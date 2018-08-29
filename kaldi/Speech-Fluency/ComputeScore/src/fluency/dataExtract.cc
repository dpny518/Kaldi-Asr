// dataExtract.cc

// Copyright 2018.03.27  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "feat/feature-mfcc.h"
#include "feat/wave-reader.h"
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "matrix/kaldi-matrix.h"
#include "Python.h"
#include "fluency/dataExtract.h"

namespace fluency {

// returns true if successfully appended.
bool AppendFeats(const std::vector<Matrix<BaseFloat> > &in,
                 std::string utt,
                 int32 tolerance,
                 Matrix<BaseFloat> *out) {
  // Check the lengths
  int32 min_len = in[0].NumRows(),
      max_len = in[0].NumRows(),
      tot_dim = in[0].NumCols();
  for (int32 i = 1; i < in.size(); i++) {
    int32 len = in[i].NumRows(), dim = in[i].NumCols();
    tot_dim += dim;
    if(len < min_len) min_len = len;
    if(len > max_len) max_len = len;
  }
  if (max_len - min_len > tolerance || min_len == 0) {
    KALDI_WARN << "Length mismatch " << max_len << " vs. " << min_len
               << (utt.empty() ? "" : " for utt ") << utt
               << " exceeds tolerance " << tolerance;
    out->Resize(0, 0);
    return false;
  }
  if (max_len - min_len > 0) {
    KALDI_VLOG(2) << "Length mismatch " << max_len << " vs. " << min_len
                  << (utt.empty() ? "" : " for utt ") << utt
                  << " within tolerance " << tolerance;
  }
  out->Resize(min_len, tot_dim);
  int32 dim_offset = 0;
  for (int32 i = 0; i < in.size(); i++) {
    int32 this_dim = in[i].NumCols();
    out->Range(0, min_len, dim_offset, this_dim).CopyFromMat(
        in[i].Range(0, min_len, 0, this_dim));
    dim_offset += this_dim;
  }
  return true;
}


DataExtract::DataExtract()
{
}

DataExtract::~DataExtract()
{
}

bool DataExtract::MatrixExtractWithTransitionIndex(std::string& strIndex, std::string& strEndWord, std::string& strStartWord,
                                                                     int32 front_phone_start_transition_index,
                                                                     int32 front_phone_end_transition_index,
                                                                     int32 next_phone_start_transition_index,
                                                                     int32 next_phone_end_transition_index)
{
    if((front_phone_start_transition_index<0) || (front_phone_end_transition_index<0) || (next_phone_start_transition_index<0) ||(next_phone_end_transition_index<0) )
        return false;

    try {
    bool binary = true;
    bool htk_in = false;
    bool sphinx_in = false;
    bool compress = false;
    int32 compression_method_in = 1;
    std::string num_frames_wspecifier;

    std::cout<<"front_phone_start_transition_index："<<front_phone_start_transition_index<<std::endl;
    std::cout<<"front_phone_end_transition_index："<<front_phone_end_transition_index<<std::endl;
    std::cout<<"next_phone_start_transition_index："<<next_phone_start_transition_index<<std::endl;
    std::cout<<"next_phone_end_transition_index："<<next_phone_end_transition_index<<std::endl;

    int32 num_done = 0;

    CompressionMethod compression_method = static_cast<CompressionMethod>(
        compression_method_in);

    std::string   lda_rspecifier = std::string("ark:final.ark");    //po.GetArg(1);
    std::string   delta_rspecifier = std::string("ark:addDeltasFinal.ark");

    SequentialBaseFloatMatrixReader lda_kaldi_reader(lda_rspecifier);

    SequentialBaseFloatMatrixReader delta_kaldi_reader(delta_rspecifier);

    Int32Writer num_frames_writer(num_frames_wspecifier);

    if (!compress) {

        for (; !lda_kaldi_reader.Done(); lda_kaldi_reader.Next(), num_done++) {
            Matrix<BaseFloat> feat(lda_kaldi_reader.Value());

            Matrix<BaseFloat>  appendFeat(delta_kaldi_reader.Value());

            //final.ark
            SubMatrix<BaseFloat> front_feats_sub(feat, front_phone_start_transition_index, (front_phone_end_transition_index - front_phone_start_transition_index + 1), 0, feat.NumCols());
            Matrix<BaseFloat> front_projection_dbl(front_feats_sub);

            SubMatrix<BaseFloat> next_feats_sub(feat, next_phone_start_transition_index, (next_phone_end_transition_index - next_phone_start_transition_index + 1), 0, feat.NumCols());
            Matrix<BaseFloat> next_projection_dbl(next_feats_sub);

            //矩阵拼接
            Matrix<BaseFloat> Lda_output(front_projection_dbl.NumRows() +next_projection_dbl.NumRows() , front_projection_dbl.NumCols());
            int32 n_Front_count = front_projection_dbl.NumRows();
            int32 next_index = 0;

            for (int32 i = 0; i < Lda_output.NumRows(); i++)
            {
                if(i < n_Front_count)
                    Lda_output.Row(i).CopyFromVec(front_projection_dbl.Row(i));
                else
                {
                    Lda_output.Row(i).CopyFromVec(next_projection_dbl.Row(next_index));
                     next_index = next_index + 1;
                }
            }

            //addDeltasFinal.ark
            SubMatrix<BaseFloat> append_front_feats_sub(appendFeat, front_phone_start_transition_index, (front_phone_end_transition_index - front_phone_start_transition_index + 1), 0, appendFeat.NumCols());
            Matrix<BaseFloat> append_front_projection_dbl(append_front_feats_sub);

            SubMatrix<BaseFloat> append_next_feats_sub(appendFeat, next_phone_start_transition_index, (next_phone_end_transition_index - next_phone_start_transition_index + 1), 0, appendFeat.NumCols());
            Matrix<BaseFloat> append_next_projection_dbl(append_next_feats_sub);

            //矩阵拼接
            Matrix<BaseFloat> append_output(append_front_projection_dbl.NumRows() +append_next_projection_dbl.NumRows() , append_front_projection_dbl.NumCols());
            int32 n_append_Front_count = append_front_projection_dbl.NumRows();
            int32 append_next_index = 0;

            for (int32 i = 0; i < append_output.NumRows(); i++)
            {
                if(i < n_append_Front_count)
                    append_output.Row(i).CopyFromVec(append_front_projection_dbl.Row(i));
                else
                {
                    append_output.Row(i).CopyFromVec(append_next_projection_dbl.Row(append_next_index));
                     append_next_index = append_next_index + 1;
                }
            }

            std::vector<Matrix<BaseFloat> > vec_appendfeats;
            vec_appendfeats.push_back(Lda_output);
            vec_appendfeats.push_back(append_output);

            Matrix<BaseFloat> allOutput;
            int32 length_tolerance = 0;
            std::string utt = lda_kaldi_reader.Key();

            if (!AppendFeats(vec_appendfeats, utt, length_tolerance, &allOutput)) {
                std::cout<<"Failed merge matrix!!!!"<<std::endl;
                break;
            }

            //矩阵转二维数组，用于检测分类结果
            /*
            int32 num_cols = output.NumCols();
            int32 num_rows = output.NumRows();

            double **array;
            array = new double*[num_rows];

            for (int32 i = 0; i < num_rows; i++) {
                //SubVector<BaseFloat> feat(output, i);
                array[i] = new double[num_cols];
                BaseFloat *src = output.RowData(i);
                for (int32 j = 0; j < num_cols; j++) {
                    array[i][j] = (double)src[j];
                }
            }

            GetClassifyResultWithPython(array,num_rows ,num_cols);
            */

           std::string strSpeaker = std::to_string(n_Front_count) + std::string("_") + strEndWord + std::string("_") + strStartWord + std::string("_") + strIndex;

           //kaldi_writer.Write(strSpeaker, output);

           m_vecMatrix.push_back(allOutput);
           m_vecSpeaker.push_back(strSpeaker);
          }
         }

      return true;
  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
    }
}

bool   DataExtract::SaveMatrixToTxtFile()
{
    std::string  arg_two = std::string("ark,t:") +  std::string("liaison") + std::string(".txt");  //+ std::string("txt/")
    std::string wspecifier = arg_two; //po.GetArg(2);
    BaseFloatMatrixWriter kaldi_writer(wspecifier);

    //std::cout<<"m_vecMatrix size:"<<std::to_string(m_vecMatrix.size());
    //std::cout<<"m_vecSpeaker size:"<<std::to_string(m_vecSpeaker.size());

    if((m_vecMatrix.size() > 0) && (m_vecSpeaker.size() > 0) )
    {
        if(m_vecMatrix.size() == m_vecSpeaker.size())
        {
              for(int32 i=0; i<m_vecMatrix.size(); i++)
              {
                    std::cout<<"write matrix to file"<<std::endl;
                    kaldi_writer.Write(m_vecSpeaker.at(i), m_vecMatrix.at(i));
              }
        }
    }
}

}