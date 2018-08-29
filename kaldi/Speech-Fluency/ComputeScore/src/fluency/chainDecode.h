// fluency/chainDecode.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#ifndef KALDI_CHAIN_DECODE_H_
#define KALDI_CHAIN_DECODE_H_ 1

#include "decoder/lattice-faster-online-decoder.h"
#include "online2/online-nnet3-decoding.h"
#include "online2/online-nnet2-feature-pipeline.h"
#include <vector>
#include <string>

using namespace kaldi;
using namespace kaldi;
using namespace fst;

typedef kaldi::int32 int32;
typedef kaldi::int64 int64;

namespace fluency {
//chain模型解码     （里面包括了特征提取 ivector等操作）

class ChainDecode {
public:
  explicit ChainDecode(const std::string mdl_dir,
                       const std::string lang_dir,
                       const std::string feature_dir);
  void Init();
  ~ChainDecode();

  //chain模型解码
  //程序代码主要参考   kaldi/src/online2bin/online2-wav-nnet3-latgen-faster
   bool  FasterDecode(std::string api_name);

   void  GetAlignmentFromDecode(const std::string &utt,
                                  const fst::SymbolTable* word_syms,
                                  const CompactLattice &clat,
                                  int64 *tot_num_frames,
                                  double *tot_like) ;

   void  SetWavId(std::string& wavID);

   //回收函数（变量重新初始化）
   void  Recovery();

    //保存解码后得到的对齐信息
    std::vector<int32>  alignments;   //可以看成是transition-id的集合
    std::vector<int32>  words;        //词id的集合

    std::vector<std::string>  word_str;  //词集合



private:
    bool  CreateSpkUttFile();   //创建spk2utt文件  （配合ivector 使用）
    bool  DelSpkUttFile();      //删除spk2utt文件

    std::string    m_strWavId;   //音频id

    std::string lang_dir_;
    std::string mdl_dir_;
    std::string feature_dir_;
    std::string  fst_rxfilename_;

    OnlineNnet2FeaturePipelineConfig feature_opts_;
    nnet3::NnetSimpleLoopedComputationOptions decodable_opts_;
    LatticeFasterDecoderConfig decoder_opts_;
    OnlineEndpointConfig endpoint_opts_;
    OnlineNnet2FeaturePipelineInfo *feature_info_;
    nnet3::DecodableNnetSimpleLoopedInfo *decodable_info_;
    fst::Fst<fst::StdArc> *decode_fst_;
    fst::SymbolTable *word_syms_;

    TransitionModel trans_model_;
    nnet3::AmNnetSimple am_nnet_;
};

}

#endif  // KALDI_CHAIN_DECODE_H_
