// fluency/score.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#ifndef KALDI_SCORE_H_
#define KALDI_SCORE_H_ 1

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-utils.h"
#include "decoder/decoder-wrappers.h"
#include "gmm/decodable-am-diag-gmm.h"
#include "lat/kaldi-lattice.h"
#include "hmm/hmm-utils.h"
#include "fluency/mfcc.h"
#include "fluency/chainDecode.h"
#include "fluency/sentenceParse.h"
#include "fluency/speechPause.h"
#include "fluency/speechStress.h"
#include "fluency/wordScore.h"
#include "common/json/json.h"
#include "common/config/config-parser.hpp"
#include "common/spdlog/spdlog.h"
#include "common/spdlog/logger.h"

using namespace kaldi;
namespace spd = spdlog;

namespace fluency {

using namespace std;

class Score {
public:
  explicit Score(std::string api_name="");
  ~Score();

  //计算语音流利度评分
  bool CalculateFluencyScore(string& str_wav_file_name, std::string str_compare);

  //根据wav路径创建wav.scp文件
  void   CreateWavScpFile(string&  str_wav_path);      

  //默认获取简单信息
  bool GetSpeechFluencyJson(string str_wav_file_name, string str_compare, string &result, int cmp_type, bool  bIsDetail = false);

  //基本分数的json字符串
  std::string    str_Json;

  //详细分数的json字符串
  std::string    str_DetailJson;

  //获取解码单词序列
  std::string   GetDecodeWords();

  std::string api_name_;

  int  cmp_type_;

  private:
    bool DeleteTempFile();

    //初始化log模块
    void  InitLogger();

    //制作错误的json
    void  MakeErrorJson();

    //制作所有分数的json
    void  MakeScoreJson(BaseFloat wordScore, BaseFloat pauseScore, BaseFloat sentenceStressScore, BaseFloat wordsStressScore);

    //制作详细分数的json
    void  MakeDetailScoreJson(Json::Value& pauseJson, BaseFloat pauseScore, Json::Value& stressJson, BaseFloat sentenceStressScore, BaseFloat wordScore);

    MfccFeature* m_pMfcc;
    SpeechPublic* m_pSpeechPublic;
    ChainDecode* m_pChainDecode;

    std::string    m_strDecodeWords;      //解码单词连起来的字符串

    //保存模型相关路径
    std::string mdl_dir_name_;
    std::string lang_dir_name_;
    std::string feature_dir_name_;

    //log 等级
    int fluency_log_level_;
	  std::shared_ptr<spdlog::logger> fluency_logger_;
};


}  // End namespace kaldi

#endif  // KALDI_SCORE_H_