// fluency/mfcc.cc

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "base/timer.h"
#include "util/common-utils.h"
#include "fstext/fstext-utils.h"
#include "fstext/fstext-lib.h"
#include "feat/feature-mfcc.h"
#include "feat/wave-reader.h"
#include "feat/feature-functions.h"  // feature reversal
#include "matrix/kaldi-matrix.h"
#include "transform/cmvn.h"
#include "transform/transform-common.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "hmm/hmm-utils.h"
#include "hmm/posterior.h"
#include "lat/kaldi-lattice.h"
#include "lat/lattice-functions.h"

#include "fluency/mfcc.h"
#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <vector>

namespace kaldi {

typedef typename fst::StdArc Arc;
typedef typename Arc::StateId StateId;
typedef typename Arc::Weight Weight;


void InitCmvnStats(int32 dim, Matrix<double> *stats) {
  KALDI_ASSERT(dim > 0);
  stats->Resize(2, dim+1);
}

bool AccCmvnStatsWrapper(std::string utt,
                         const MatrixBase<BaseFloat> &feats,
                         RandomAccessBaseFloatVectorReader *weights_reader,
                         Matrix<double> *cmvn_stats) {
  if (!weights_reader->IsOpen()) {
    AccCmvnStats(feats, NULL, cmvn_stats);
    return true;
  } else {
    if (!weights_reader->HasKey(utt)) {
      KALDI_WARN << "No weights available for utterance " << utt;
      return false;
    }
    const Vector<BaseFloat> &weights = weights_reader->Value(utt);
    if (weights.Dim() != feats.NumRows()) {
      KALDI_WARN << "Weights for utterance " << utt << " have wrong dimension "
                 << weights.Dim() << " vs. " << feats.NumRows();
      return false;
    }
    AccCmvnStats(feats, &weights, cmvn_stats);
    return true;
  }
}

void AccCmvnStats(const VectorBase<BaseFloat> &feats, BaseFloat weight, MatrixBase<double> *stats) {
  int32 dim = feats.Dim();
  KALDI_ASSERT(stats != NULL);
  KALDI_ASSERT(stats->NumRows() == 2 && stats->NumCols() == dim + 1);

   double *__restrict__ mean_ptr = stats->RowData(0),
       *__restrict__ var_ptr = stats->RowData(1),
       *__restrict__ count_ptr = mean_ptr + dim;
   const BaseFloat * __restrict__ feats_ptr = feats.Data();
  *count_ptr += weight;

  for (; mean_ptr < count_ptr; mean_ptr++, var_ptr++, feats_ptr++) {
    *mean_ptr += *feats_ptr * weight;
    *var_ptr +=  *feats_ptr * *feats_ptr * weight;
  }
}

void AccCmvnStats(const MatrixBase<BaseFloat> &feats,
                  const VectorBase<BaseFloat> *weights,
                  MatrixBase<double> *stats) {
  int32 num_frames = feats.NumRows();
  if (weights != NULL) {
    KALDI_ASSERT(weights->Dim() == num_frames);
  }
  for (int32 i = 0; i < num_frames; i++) {
    SubVector<BaseFloat> this_frame = feats.Row(i);
    BaseFloat weight = (weights == NULL ? 1.0 : (*weights)(i));
    if (weight != 0.0)
      AccCmvnStats(this_frame, weight, stats);
  }
}

void FakeStatsForSomeDims(const std::vector<int32> &dims,
                          MatrixBase<double> *stats) {
  KALDI_ASSERT(stats->NumRows() == 2 && stats->NumCols() > 1);
  int32 dim = stats->NumCols() - 1;
  double count = (*stats)(0, dim);
  for (size_t i = 0; i < dims.size(); i++) {
    int32 d = dims[i];
    KALDI_ASSERT(d >= 0 && d < dim);
    (*stats)(0, d) = 0.0;
    (*stats)(1, d) = count;
  }
}

void ApplyCmvnReverse(const MatrixBase<double> &stats,
                      bool var_norm,
                      MatrixBase<BaseFloat> *feats) {
  KALDI_ASSERT(feats != NULL);
  int32 dim = stats.NumCols() - 1;
  if (stats.NumRows() > 2 || stats.NumRows() < 1 || feats->NumCols() != dim) {
    KALDI_ERR << "Dim mismatch: cmvn "
              << stats.NumRows() << 'x' << stats.NumCols()
              << ", feats " << feats->NumRows() << 'x' << feats->NumCols();
  }
  if (stats.NumRows() == 1 && var_norm)
    KALDI_ERR << "You requested variance normalization but no variance stats "
              << "are supplied.";

  double count = stats(0, dim);
  // Do not change the threshold of 1.0 here: in the balanced-cmvn code, when
  // computing an offset and representing it as stats, we use a count of one.
  if (count < 1.0)
    KALDI_ERR << "Insufficient stats for cepstral mean and variance normalization: "
              << "count = " << count;

  Matrix<BaseFloat> norm(2, dim);  // norm(0, d) = mean offset
  // norm(1, d) = scale, e.g. x(d) <-- x(d)*norm(1, d) + norm(0, d).
  for (int32 d = 0; d < dim; d++) {
    double mean, offset, scale;
    mean = stats(0, d) / count;
    if (!var_norm) {
      scale = 1.0;
      offset = mean;
    } else {
      double var = (stats(1, d)/count) - mean*mean,
          floor = 1.0e-20;
      if (var < floor) {
        KALDI_WARN << "Flooring cepstral variance from " << var << " to "
                   << floor;
        var = floor;
      }
      // we aim to transform zero-mean, unit-variance input into data
      // with the given mean and variance.
      scale = sqrt(var);
      offset = mean;
    }
    norm(0, d) = offset;
    norm(1, d) = scale;
  }
  if (var_norm)
    feats->MulColsVec(norm.Row(1));
  feats->AddVecToRows(1.0, norm.Row(0));
}

void ApplyCmvn(const MatrixBase<double> &stats,
               bool var_norm,
               MatrixBase<BaseFloat> *feats) {
  KALDI_ASSERT(feats != NULL);
  int32 dim = stats.NumCols() - 1;
  if (stats.NumRows() > 2 || stats.NumRows() < 1 || feats->NumCols() != dim) {
    KALDI_ERR << "Dim mismatch: cmvn "
              << stats.NumRows() << 'x' << stats.NumCols()
              << ", feats " << feats->NumRows() << 'x' << feats->NumCols();
  }
  if (stats.NumRows() == 1 && var_norm)
    KALDI_ERR << "You requested variance normalization but no variance stats "
              << "are supplied.";

  double count = stats(0, dim);
  // Do not change the threshold of 1.0 here: in the balanced-cmvn code, when
  // computing an offset and representing it as stats, we use a count of one.
  if (count < 1.0)
    KALDI_ERR << "Insufficient stats for cepstral mean and variance normalization: "
              << "count = " << count;

  Matrix<BaseFloat> norm(2, dim);  // norm(0, d) = mean offset
  // norm(1, d) = scale, e.g. x(d) <-- x(d)*norm(1, d) + norm(0, d).
  for (int32 d = 0; d < dim; d++) {
    double mean, offset, scale;
    mean = stats(0, d)/count;
    if (!var_norm) {
      scale = 1.0;
      offset = -mean;
    } else {
      double var = (stats(1, d)/count) - mean*mean,
          floor = 1.0e-20;
      if (var < floor) {
        KALDI_WARN << "Flooring cepstral variance from " << var << " to "
                   << floor;
        var = floor;
      }
      scale = 1.0 / sqrt(var);
      if (scale != scale || 1/scale == 0.0)
        KALDI_ERR << "NaN or infinity in cepstral mean/variance computation";
      offset = -(mean*scale);
    }
    norm(0, d) = offset;
    norm(1, d) = scale;
  }
  // Apply the normalization.
  if (var_norm)
    feats->MulColsVec(norm.Row(1));
  feats->AddVecToRows(1.0, norm.Row(0));
}

bool  is_file_exist(const char* filename)
{
    //检测当前文件下文件是否存在，存在返回true
    if(filename == NULL)
        return false;

    if(access(filename, F_OK) == 0 )
        return true;
    return false;
}

bool del_temporary_file(const char* filename)
{
    //根据文件名删除文件
    if(filename == NULL)
        return false;

    if(remove(filename)==0)
        return true;
    return false;
}

bool MfccFeature::ComputeMfccFeature()
{
    //计算mfcc状态，调用函数ComputeMfccFeats
    //使用compute-mfcc-feats生成对应的特征文件feats.ark

	  Matrix<BaseFloat>  mfcc_features;

    bool bIsComputeMfcc = ComputeMfccFeats(mfcc_features);

    if(!bIsComputeMfcc)
    {
        KALDI_ERR << "ComputeMfccFeats failed. ";
        return false;
    }
    std::cout<<"compute-mfcc-feat完成！！"<<std::endl;

    //拷贝特征文件，并生成scp文件
    //使用copy-feats来拷贝特征文件，并创建特征的scp文件，生成feat.scp feat.ark
    bool bIsCopyFeats = CopyFeats(mfcc_features);

    if(!bIsCopyFeats)
    {
       KALDI_ERR << "CopyFeats failed. ";
        return false;
    }
    std::cout<<"copy-feats完成！！"<<std::endl;

    //计算CMVN归一化
    //使用compute-cmvn-stats计算CMVN归一化，得到cmvn.scp  cmvn.ark

    Matrix<double>  cmvn_features;

    bool bIsComputeCmvn = ComputeCMVNStats(mfcc_features, cmvn_features);

    if(!bIsComputeCmvn)
    {
        KALDI_ERR << "ComputeCMVNStats failed. ";
        return false;
    }
    std::cout<<"compute-cmvn-states完成！！"<<std::endl;

    //使用apply-cmvn，得到了applycmvn.ark文件
    bool bIsApplyCmvn = ApplyCMVN(mfcc_features, cmvn_features);

    if(!bIsApplyCmvn)
    {
        KALDI_ERR << "ApplyCMVN failed. ";
        return false;
    }
    std::cout<<"apply-cmvn完成！！"<<std::endl;

    Matrix<BaseFloat>  splice_features;
    //使用splice-feats来继续变换特征
    bool bIsSplice = SpliceFeats(mfcc_features, splice_features);

    if(!bIsSplice)
    {
        KALDI_ERR << "SpliceFeats failed. ";
        return false;
    }
    std::cout<<"splice-feats完成！！"<<std::endl;

    if(is_file_exist("final.mat"))
    {
        bool bIsTransform = Transform(splice_features);
        if(!bIsTransform)
        {
            KALDI_ERR << "Transform failed. ";
            return false;
        }
    }
    std::cout<<"trans-form完成！！"<<std::endl;

    //使用add-deltas来进行特征转换
    /*
    std::string  add_deltas_input_ark = "ark:applycmvn.ark";
    std::string  add_deltas_output_ark = "ark:addDeltasFinal.ark";
    if((is_file_exist("applycmvn.ark")))
    {
        bool bIsAddDeltas = AddDeltas(add_deltas_input_ark, add_deltas_output_ark);
        if(!bIsAddDeltas)
        {
          KALDI_ERR << "Add-deltas failed. ";
          return false;
        }
    }
    */

    return true;
}

bool  MfccFeature::ComputeMfccFeats(Matrix<BaseFloat>& kaldi_features)
{
   //首先判断wav.scp文件是否存在
    if(!is_file_exist("wav.scp"))
    {
        KALDI_ERR << "wav.scp does not exist. ";
        return false;
    }

    try {

    MfccOptions mfcc_opts;

    mfcc_opts.use_energy = false;

    bool subtract_mean = false;
    BaseFloat vtln_warp = 1.0;
    std::string vtln_map_rspecifier;
    std::string utt2spk_rspecifier;
    int32 channel = -1;
    BaseFloat min_duration = 0.0;
    // Define defaults for gobal options
    std::string output_format = "kaldi";

    std::string wav_rspecifier =  "scp:wav.scp";          //po.GetArg(1);

    std::string output_wspecifier =  "ark:feats.ark";  //po.GetArg(2);

    Mfcc mfcc(mfcc_opts);

    SequentialTableReader<WaveHolder> reader(wav_rspecifier);
    BaseFloatMatrixWriter kaldi_writer;  // typedef to TableWriter<something>.
    TableWriter<HtkMatrixHolder> htk_writer;

    if (utt2spk_rspecifier != "")
      KALDI_ASSERT(vtln_map_rspecifier != "" && "the utt2spk option is only "
                   "needed if the vtln-map option is used.");
    RandomAccessBaseFloatReaderMapped vtln_map_reader(vtln_map_rspecifier,
                                                      utt2spk_rspecifier);

    int32 num_utts = 0, num_success = 0;
    for (; !reader.Done(); reader.Next()) {
      num_utts++;
      std::string utt = reader.Key();
      const WaveData &wave_data = reader.Value();
      if (wave_data.Duration() < min_duration) {
        KALDI_WARN << "File: " << utt << " is too short ("
                   << wave_data.Duration() << " sec): producing no output.";
        continue;
      }
      int32 num_chan = wave_data.Data().NumRows(), this_chan = channel;
      {  // This block works out the channel (0=left, 1=right...)
        KALDI_ASSERT(num_chan > 0);  // should have been caught in
        // reading code if no channels.
        if (channel == -1) {
          this_chan = 0;
          if (num_chan != 1)
            KALDI_WARN << "Channel not specified but you have data with "
                       << num_chan  << " channels; defaulting to zero";
        } else {
          if (this_chan >= num_chan) {
            KALDI_WARN << "File with id " << utt << " has "
                       << num_chan << " channels but you specified channel "
                       << channel << ", producing no output.";
            continue;
          }
        }
      }
      BaseFloat vtln_warp_local;  // Work out VTLN warp factor.
      if (vtln_map_rspecifier != "") {
        if (!vtln_map_reader.HasKey(utt)) {
          KALDI_WARN << "No vtln-map entry for utterance-id (or speaker-id) "
                     << utt;
          continue;
        }
        vtln_warp_local = vtln_map_reader.Value(utt);
      } else {
        vtln_warp_local = vtln_warp;
      }

      SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);
      //Matrix<BaseFloat> features;

      BaseFloat wavFreq = 1.0; //wave_data.SampFreq();

      try {
        mfcc.Compute(waveform, vtln_warp_local, &kaldi_features);
      } catch (...) {
        KALDI_WARN << "Failed to compute kaldi_features for utterance "
                   << utt;
        continue;
      }

      //13维特征向量取出第一列（能量）
      bool bEnergy = GetLogEnergyFromFeature(kaldi_features);
      if(!bEnergy)
        KALDI_WARN << "Failed to get frame energy form kaldi_features. ";
    
      //原来的代码，这里是准备写入文件，utt 和features,这里通过变量来进行替换 

      m_strUtt = utt;

    }

    return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::CopyFeats(Matrix<BaseFloat>& kaldi_features)
{
    try {

    bool binary = true;
    bool htk_in = false;
    bool sphinx_in = false;
    bool compress = false;
    int32 compression_method_in = 1;
    std::string num_frames_wspecifier;

    int32 num_done = 0;

    CompressionMethod compression_method = static_cast<CompressionMethod>(
        compression_method_in);

    //这里不判断文件里面的fier ，直接将文件写入到新文件中， 所以最外层循环去掉
    
    Int32Writer num_frames_writer(num_frames_wspecifier);

    if (!compress) {
      //BaseFloatMatrixWriter kaldi_writer(wspecifier);
      //SequentialBaseFloatMatrixReader kaldi_reader(rspecifier);
      
  	}	

  	std::cout<<"复制特征文件完成!"<<std::endl;

  	return true;
  	
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::ComputeCMVNStats(Matrix<BaseFloat>& kaldi_features, Matrix<double>& cmvn_features)
{
    try {

    std::string spk2utt_rspecifier, weights_rspecifier;
    bool binary = true;

    int32 num_done = 0, num_err = 0;

    RandomAccessBaseFloatVectorReader weights_reader(weights_rspecifier);

    InitCmvnStats(kaldi_features.NumCols(), &cmvn_features);

    if (!AccCmvnStatsWrapper(m_strUtt, kaldi_features, &weights_reader, &cmvn_features)) {
      return false;
    }

    return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::ApplyCMVN(Matrix<BaseFloat>& kaldi_features, Matrix<double>& cmvn_features)
{
    try {
    std::string utt2spk_rspecifier;
    bool norm_vars = false;
    bool norm_means = true;
    bool reverse = false;
    std::string skip_dims_str;

    if (norm_vars && !norm_means)
      KALDI_ERR << "You cannot normalize the variance but not the mean.";

    std::vector<int32> skip_dims;  // optionally use "fake"
                                   // (zero-mean/unit-variance) stats for some
                                   // dims to disable normalization.
    if (!SplitStringToIntegers(skip_dims_str, ":", false, &skip_dims)) {
      KALDI_ERR << "Bad --skip-dims option (should be colon-separated list of "
                <<  "integers)";
    }

    kaldi::int32 num_done = 0, num_err = 0;

    if (norm_means) {    
      
      if (!skip_dims.empty())
        FakeStatsForSomeDims(skip_dims, &cmvn_features);

      if (reverse) {
        ApplyCmvnReverse(cmvn_features, norm_vars, &kaldi_features);
      } else {
        ApplyCmvn(cmvn_features, norm_vars, &kaldi_features);
      }

    } 

    return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::SpliceFeats(Matrix<BaseFloat>& kaldi_features, Matrix<BaseFloat>& splice_feature)
{
    try {

    int32 left_context = 3, right_context = 3;

    //std::string wspecifier = std::string("ark:splice.ark");    //po.GetArg(2);

    //BaseFloatMatrixWriter kaldi_writer(wspecifier);

    SpliceFrames(kaldi_features, left_context, right_context, &splice_feature);
    //kaldi_writer.Write(m_strUtt, splice_feature);

    return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::Transform(Matrix<BaseFloat>& splice_feature)
{
    try {
    using namespace kaldi;

    std::string utt2spk_rspecifier;

    std::string transform_rspecifier_or_rxfilename = "final.mat";  //po.GetArg(1);
    std::string feat_wspecifier = "ark:final.ark"; //po.GetArg(3);

    BaseFloatMatrixWriter feat_writer(feat_wspecifier);

    RandomAccessBaseFloatMatrixReaderMapped transform_reader;
    bool use_global_transform;
    Matrix<BaseFloat> global_transform;

    if (ClassifyRspecifier(transform_rspecifier_or_rxfilename, NULL, NULL) == kNoRspecifier) {
          // not an rspecifier -> interpret as rxfilename....
          use_global_transform = true;
          ReadKaldiObject(transform_rspecifier_or_rxfilename, &global_transform);
    } 

    enum { Unknown, Logdet, PseudoLogdet, DimIncrease };
    int32 logdet_type = Unknown;
    double tot_t = 0.0, tot_logdet = 0.0;  // to compute average logdet weighted by time...
    int32 num_done = 0, num_error = 0;
    BaseFloat cached_logdet = -1;

    if (!use_global_transform && !transform_reader.HasKey(m_strUtt)) {
      KALDI_WARN << "No fMLLR transform available for utterance "<< m_strUtt << ", producing no output for this utterance";
      num_error++;
      return false;
    }

    const Matrix<BaseFloat> &trans = (use_global_transform ? global_transform : transform_reader.Value(m_strUtt));
    int32 transform_rows = trans.NumRows(),
              transform_cols = trans.NumCols(),
              feat_dim = splice_feature.NumCols();

    Matrix<BaseFloat> feat_out(splice_feature.NumRows(), transform_rows);

    if (transform_cols == feat_dim) {
      feat_out.AddMatMat(1.0, splice_feature, kNoTrans, trans, kTrans, 0.0);
    } else if (transform_cols == feat_dim + 1) {
      // append the implicit 1.0 to the input features.
      SubMatrix<BaseFloat> linear_part(trans, 0, transform_rows, 0, feat_dim);
      feat_out.AddMatMat(1.0, splice_feature, kNoTrans, linear_part, kTrans, 0.0);
      Vector<BaseFloat> offset(transform_rows);
      offset.CopyColFromMat(trans, feat_dim);
      feat_out.AddVecToRows(1.0, offset);
    } else {
      return false;
    }

    feat_writer.Write(m_strUtt, feat_out);

    return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::AddDeltas(std::string& input_Ark, std::string& output_Ark)
{
  try {
    using namespace kaldi;

    DeltaFeaturesOptions opts;
    int32 truncate = 0;

    std::string rspecifier = input_Ark;
    std::string wspecifier = output_Ark;

    BaseFloatMatrixWriter feat_writer(wspecifier);
    SequentialBaseFloatMatrixReader feat_reader(rspecifier);
    for (; !feat_reader.Done(); feat_reader.Next()) {
      std::string key = feat_reader.Key();
      const Matrix<BaseFloat> &feats  = feat_reader.Value();

      if (feats.NumRows() == 0) {
        KALDI_WARN << "Empty feature matrix for key " << key;
        continue;
      }
      Matrix<BaseFloat> new_feats;
      if (truncate != 0) {
        if (truncate > feats.NumCols())
          KALDI_ERR << "Cannot truncate features as dimension " << feats.NumCols()
                    << " is smaller than truncation dimension.";
        SubMatrix<BaseFloat> feats_sub(feats, 0, feats.NumRows(), 0, truncate);
        ComputeDeltas(opts, feats_sub, &new_feats);
      } else
        ComputeDeltas(opts, feats, &new_feats);
      feat_writer.Write(key, new_feats);
    }
    return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
}

bool  MfccFeature::GetLogEnergyFromFeature(Matrix<BaseFloat>& features)
{
    if(features.NumCols() > 0)
    {
        for(int i=0; i<features.NumRows(); i++)
        {
            BaseFloat energy = (*(features.Data() + i*features.Stride()));
            //std::cout<<"第"<<i<<"帧的能量值是："<<energy<<std::endl;
            m_vecEnergy.push_back(energy);
        }
        return true;
    }
    return false;
}

void  MfccFeature::Recovery()
{
     //收回uut vector空间
     m_vecEnergy.clear();
     std::vector <BaseFloat>().swap(m_vecEnergy);
}

}  // End namespace kaldi
