// fluency/chainDecode.cc

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#include "feat/wave-reader.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include "online2/onlinebin-util.h"
#include "online2/online-timing.h"
#include "online2/online-endpoint.h"
#include "fstext/fstext-lib.h"
#include "lat/lattice-functions.h"
#include "util/kaldi-thread.h"
#include "nnet3/nnet-utils.h"

#include "fluency/chainDecode.h"

#include <boost/timer.hpp>
using namespace boost;


namespace fluency {

void GetDiagnosticsAndPrintOutput(const std::string &utt,
                                  const fst::SymbolTable *word_syms,
                                  const CompactLattice &clat,
                                  int64 *tot_num_frames,
                                  double *tot_like)
{
  if (clat.NumStates() == 0) {
    KALDI_WARN << "Empty lattice.";
    return;
  }
  CompactLattice best_path_clat;
  CompactLatticeShortestPath(clat, &best_path_clat);

  Lattice best_path_lat;
  ConvertLattice(best_path_clat, &best_path_lat);

  double likelihood;
  LatticeWeight weight;
  int32 num_frames;
  std::vector<int32> alignment;
  std::vector<int32> words;
  GetLinearSymbolSequence(best_path_lat, &alignment, &words, &weight);
  num_frames = alignment.size();
  likelihood = -(weight.Value1() + weight.Value2());
  *tot_num_frames += num_frames;
  *tot_like += likelihood;
  KALDI_VLOG(2) << "Likelihood per frame for utterance " << utt << " is "
                << (likelihood / num_frames) << " over " << num_frames
                << " frames.";

  if (word_syms != NULL) {
    std::cerr << utt << ' ';
    for (size_t i = 0; i < words.size(); i++) {
      std::string s = word_syms->Find(words[i]);
      if (s == "")
        KALDI_ERR << "Word-id " << words[i] << " not in symbol table.";
      std::cerr << s << ' ';
    }
    std::cerr << std::endl;
  }
}

ChainDecode::ChainDecode(const std::string mdl_dir,
                       const std::string lang_dir,
                       const std::string feature_dir)
{
    m_strWavId = "";
    lang_dir_ = lang_dir;
    mdl_dir_ = mdl_dir;
    feature_dir_ = feature_dir;
}

void ChainDecode::Init()
{
    //  特征配置
    feature_opts_.feature_type = std::string("mfcc");
    feature_opts_.mfcc_config = std::string(feature_dir_+"/mfcc.conf");
    feature_opts_.ivector_extraction_config = std::string(feature_dir_+"/ivector_extractor.conf");

    //解码器配置
    decodable_opts_.acoustic_scale = 1;
    decodable_opts_.extra_left_context_initial = 0;
    decodable_opts_.frame_subsampling_factor = 1;
    decodable_opts_.frames_per_chunk = 10;

    decoder_opts_.max_active = 200;
    decoder_opts_.beam = 8.0;
    decoder_opts_.lattice_beam = 10.0;
    decoder_opts_.prune_interval = 50;

    feature_info_ = new OnlineNnet2FeaturePipelineInfo(feature_opts_);


    //端点配置（静音音素id）
    endpoint_opts_.silence_phones = std::string("1:2:3:4:5:6:7:8:9:10");


    std::string  nnet3_rxfilename = mdl_dir_+"/final.mdl";   //po.GetArg(1),
    fst_rxfilename_ = lang_dir_+"/HCLG.fst";   //po.GetArg(2),

    bool binary;
    Input ki(nnet3_rxfilename, &binary);
    trans_model_.Read(ki.Stream(), binary);
    am_nnet_.Read(ki.Stream(), binary);
    SetBatchnormTestMode(true, &(am_nnet_.GetNnet()));
    SetDropoutTestMode(true, &(am_nnet_.GetNnet()));
    nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet_.GetNnet()));

    decodable_info_ = new nnet3::DecodableNnetSimpleLoopedInfo(decodable_opts_,&am_nnet_);

    decode_fst_ = ReadFstKaldiGeneric(fst_rxfilename_);

    std::string word_syms_rxfilename = std::string(lang_dir_+"/words.txt");
    word_syms_ = NULL;
    if (word_syms_rxfilename != "")
      if (!(word_syms_ = fst::SymbolTable::ReadText(word_syms_rxfilename)))
        KALDI_ERR << "Could not read symbol table from file "
                  << word_syms_rxfilename;

}

ChainDecode::~ChainDecode()
{
    delete decode_fst_;
    delete word_syms_;
    delete feature_info_;
    delete decodable_info_;
}

bool  ChainDecode::FasterDecode(std::string api_name)
{
    //创建spk2utt文件
    CreateSpkUttFile();

    try {

    // feature_opts includes configuration for the iVector adaptation,
    // as well as the basic features.
    /*timer t;
    OnlineNnet2FeaturePipelineConfig feature_opts;
    nnet3::NnetSimpleLoopedComputationOptions decodable_opts;
    LatticeFasterDecoderConfig decoder_opts;
    OnlineEndpointConfig endpoint_opts;
    std::cout << "chain init time " << t.elapsed() << std::endl;*/

    BaseFloat chunk_length_secs = 0.18;
    bool do_endpointing = false;
    bool online = false;

    /*
    //  特征配置
    feature_opts.feature_type = std::string("mfcc");
    feature_opts.mfcc_config = std::string(feature_dir_+"/mfcc.conf");
    feature_opts.ivector_extraction_config = std::string(feature_dir_+"/ivector_extractor.conf");

    //解码器配置
    decodable_opts.acoustic_scale = 1;
    decodable_opts.extra_left_context_initial = 0;
    decodable_opts.frame_subsampling_factor = 3;
    decodable_opts.frames_per_chunk = 10;

    decoder_opts.max_active = 200;
    decoder_opts.beam = 8.0;
    decoder_opts.lattice_beam = 10.0;
    decoder_opts.prune_interval = 50;

    //端点配置（静音音素id）
    endpoint_opts.silence_phones = std::string("1:2:3:4:5:6:7:8:9:10");

    std::string  nnet3_rxfilename = mdl_dir_+"/final.mdl";   //po.GetArg(1),
    std::string  fst_rxfilename = lang_dir_+"/HCLG.fst";   //po.GetArg(2),
    std::string  spk2utt_rspecifier = "ark:spk2utt";   //po.GetArg(3),
    std::string  wav_rspecifier = "scp:"+api_name+"wav.scp";   //po.GetArg(4),
    std::string  clat_wspecifier = "ark:"+api_name+"lattice";   //po.GetArg(5);
    */
    std::string  wav_rspecifier = "scp:"+api_name+"wav.scp";
    std::string  spk2utt_rspecifier = "ark:spk2utt";
    std::string  clat_wspecifier = "ark:"+api_name+"lattice";

    /*timer t;
    OnlineNnet2FeaturePipelineInfo feature_info(feature_opts_);
    std::cout << "chain init feature_info " << t.elapsed() << std::endl;
    */
    if (!online) {
      feature_info_->ivector_extractor_info.use_most_recent_ivector = true;
      feature_info_->ivector_extractor_info.greedy_ivector_extractor = true;
      chunk_length_secs = -1.0;
    }

    /*
    TransitionModel trans_model;
    nnet3::AmNnetSimple am_nnet;
    {
      bool binary;
      Input ki(nnet3_rxfilename, &binary);
      trans_model.Read(ki.Stream(), binary);
      am_nnet.Read(ki.Stream(), binary);
      SetBatchnormTestMode(true, &(am_nnet.GetNnet()));
      SetDropoutTestMode(true, &(am_nnet.GetNnet()));
      nnet3::CollapseModel(nnet3::CollapseModelConfig(), &(am_nnet.GetNnet()));
    }
    */

    // this object contains precomputed stuff that is used by all decodable
    // objects.  It takes a pointer to am_nnet because if it has iVectors it has
    // to modify the nnet to accept iVectors at intervals.
    /*
    timer t1;
    nnet3::DecodableNnetSimpleLoopedInfo decodable_info(decodable_opts_,
                                                        &am_nnet_);
    std::cout << "chain init decodable_info " << t1.elapsed() << std::endl;
    */

    /*
    timer t2;
    fst::Fst<fst::StdArc> *decode_fst = ReadFstKaldiGeneric(fst_rxfilename_);
    std::cout << "chain init decode_fst " << t2.elapsed() << std::endl;
    */

    /*
    timer t3;
    fst::SymbolTable *word_syms = NULL;
    if (word_syms_rxfilename != "")
      if (!(word_syms = fst::SymbolTable::ReadText(word_syms_rxfilename)))
        KALDI_ERR << "Could not read symbol table from file "
                  << word_syms_rxfilename;
    std::cout << "chain init word_syms " << t3.elapsed() << std::endl;
    */

    int32 num_done = 0, num_err = 0;
    double tot_like = 0.0;
    int64 num_frames = 0;

    SequentialTokenVectorReader spk2utt_reader(spk2utt_rspecifier);
    RandomAccessTableReader<WaveHolder> wav_reader(wav_rspecifier);
    CompactLatticeWriter clat_writer(clat_wspecifier);

    OnlineTimingStats timing_stats;

    /*for (; !spk2utt_reader.Done(); spk2utt_reader.Next()) {
      std::string spk = spk2utt_reader.Key();
      const std::vector<std::string> &uttlist = spk2utt_reader.Value();
      OnlineIvectorExtractorAdaptationState adaptation_state(
          feature_info.ivector_extractor_info);
      for (size_t i = 0; i < uttlist.size(); i++) {
        std::string utt = uttlist[i];
        if (!wav_reader.HasKey(utt)) {
          KALDI_WARN << "Did not find audio for utterance " << utt;
          num_err++;
          continue;
        }*/
        std::string spk = spk2utt_reader.Key();
        const std::vector<std::string> &uttlist = spk2utt_reader.Value();
        std::string utt = uttlist[0];
        timer t;
        OnlineIvectorExtractorAdaptationState adaptation_state(
          feature_info_->ivector_extractor_info);
        std::cout << "adaptation_state init" << t.elapsed() << std::endl;

        const WaveData &wave_data = wav_reader.Value(utt);
        // get the data for channel zero (if the signal is not mono, we only
        // take the first channel).
        SubVector<BaseFloat> data(wave_data.Data(), 0);

        timer t1;
        OnlineNnet2FeaturePipeline feature_pipeline(*feature_info_);
        feature_pipeline.SetAdaptationState(adaptation_state);
        std::cout << "feature_pipeline init" << t1.elapsed() << std::endl;

        timer t2;
        OnlineSilenceWeighting silence_weighting(
            trans_model_,
            feature_info_->silence_weighting_config,
	    decodable_opts_.frame_subsampling_factor);
        std::cout << "silence_weighting init" << t2.elapsed() << std::endl;

        timer t3;
        SingleUtteranceNnet3Decoder decoder(decoder_opts_, trans_model_,
                                            *decodable_info_,
                                            *decode_fst_, &feature_pipeline);
        std::cout << "decoder init" << t3.elapsed() << std::endl;
        OnlineTimer decoding_timer(utt);

        BaseFloat samp_freq = wave_data.SampFreq();
        int32 chunk_length;
        if (chunk_length_secs > 0) {
          chunk_length = int32(samp_freq * chunk_length_secs);
          if (chunk_length == 0) chunk_length = 1;
        } else {
          chunk_length = std::numeric_limits<int32>::max();
        }

        int32 samp_offset = 0;
        std::vector<std::pair<int32, BaseFloat> > delta_weights;

        while (samp_offset < data.Dim()) {
          int32 samp_remaining = data.Dim() - samp_offset;
          int32 num_samp = chunk_length < samp_remaining ? chunk_length
                                                         : samp_remaining;

          SubVector<BaseFloat> wave_part(data, samp_offset, num_samp);
          feature_pipeline.AcceptWaveform(samp_freq, wave_part);

          samp_offset += num_samp;
          decoding_timer.WaitUntil(samp_offset / samp_freq);
          if (samp_offset == data.Dim()) {
            // no more input. flush out last frames
            feature_pipeline.InputFinished();
          }

          if (silence_weighting.Active() &&
              feature_pipeline.IvectorFeature() != NULL) {
            silence_weighting.ComputeCurrentTraceback(decoder.Decoder());
            silence_weighting.GetDeltaWeights(feature_pipeline.NumFramesReady(),
                                              &delta_weights);
            feature_pipeline.IvectorFeature()->UpdateFrameWeights(delta_weights);
          }
          timer t2;
          decoder.AdvanceDecoding();
          std::cout << "chain decode " << t2.elapsed() << std::endl;

          if (do_endpointing && decoder.EndpointDetected(endpoint_opts_))
            break;
        }

        decoder.FinalizeDecoding();

        CompactLattice clat;
        bool end_of_utterance = true;
        decoder.GetLattice(end_of_utterance, &clat);

        GetDiagnosticsAndPrintOutput(utt, word_syms_, clat,
                                     &num_frames, &tot_like);

        //获取解码信息
        GetAlignmentFromDecode(utt, word_syms_, clat,
                                     &num_frames, &tot_like);

        decoding_timer.OutputStats(&timing_stats);

        // In an application you might avoid updating the adaptation state if
        // you felt the utterance had low confidence.  See lat/confidence.h
        feature_pipeline.GetAdaptationState(&adaptation_state);

        // we want to output the lattice with un-scaled acoustics.
        BaseFloat inv_acoustic_scale =
            1.0 / decodable_opts_.acoustic_scale;
        ScaleLattice(AcousticLatticeScale(inv_acoustic_scale), &clat);

        clat_writer.Write(utt, clat);
        KALDI_LOG << "Decoded utterance " << utt;
        num_done++;
    timing_stats.Print(online);

    KALDI_LOG << "Decoded " << num_done << " utterances, "
              << num_err << " with errors.";
    KALDI_LOG << "Overall likelihood per frame was " << (tot_like / num_frames)
              << " per frame over " << num_frames << " frames.";
    //delete decode_fst;
    //delete word_syms; // will delete if non-NULL.

    //删除spk2utt文件
    DelSpkUttFile();

    return true;

  } catch(const std::exception& e) {
    std::cerr << e.what();
    //删除spk2utt文件
    DelSpkUttFile();
    return false;
  }

}

void  ChainDecode::GetAlignmentFromDecode(const std::string &utt,
                                  const fst::SymbolTable* word_syms,
                                  const CompactLattice &clat,
                                  int64 *tot_num_frames,
                                  double *tot_like) {
  if (clat.NumStates() == 0) {
    KALDI_WARN << "Empty lattice.";
    return;
  }
  CompactLattice best_path_clat;
  CompactLatticeShortestPath(clat, &best_path_clat);

  Lattice best_path_lat;
  ConvertLattice(best_path_clat, &best_path_lat);

  LatticeWeight weight;

  GetLinearSymbolSequence(best_path_lat, &alignments, &words, &weight);

  if(word_syms != NULL)
  {
        for(int32 i = 0; i<words.size(); i++)
            {
                std::string strword = word_syms->Find(words[i]);
                word_str.push_back(strword);
            }
  }
}

bool  ChainDecode::CreateSpkUttFile()
{
    std::ofstream spk2utt_ofs;

    spk2utt_ofs.open("spk2utt",  std::ios::out | std::ios::trunc );  //以写入和在文件末尾添加的方式打开.txt文件，没有的话就创建该文件。
    if (!spk2utt_ofs.is_open())
        return false;

    std::string  str_speaker_id = std::string("speaker_id");
    //std::string  str_wav_id = m_strWavId; //std::string("fulency_wav_id");
    std::string  str_wav_id = std::string("fulency_wav_id");
    spk2utt_ofs << str_speaker_id << " " << str_wav_id <<std::endl;

    spk2utt_ofs.close();
    return true;
}

bool  ChainDecode::DelSpkUttFile()
{
    std::string del_spk_file = std::string("spk2utt");

    if(remove(del_spk_file.c_str()) == 0)
         return true;
     else
         return false;
}

void  ChainDecode::SetWavId(std::string& wavID)
{
    m_strWavId = wavID;
}

void  ChainDecode::Recovery()
{
    //收回alignments空间
     alignments.clear();
     std::vector <int32>().swap(alignments);

     //收回words空间
     words.clear();
     std::vector <int32>().swap(words);

     //收回word_str空间
     word_str.clear();
     std::vector <std::string>().swap(word_str);

     m_strWavId = "";
}

}  // End namespace kaldi
