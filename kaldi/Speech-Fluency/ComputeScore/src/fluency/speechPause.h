// fluency/speechPause.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#ifndef KALDI_SPEECH_PAUSE_H_
#define KALDI_SPEECH_PAUSE_H_ 1

#include "fluency/speechPublic.h"
#include "common/json/json.h"

using namespace kaldi;

namespace fluency {

const int kTemporaryEpsilon = -2;
typedef unordered_map<int32, std::vector<int32> > PauseLexiconMap;

//这个类主要记录停顿信息
class SpeechPauseInfo{
public:

  explicit SpeechPauseInfo() { phoneStartIndex = -1; }    // 构造函数传起始索引
  ~SpeechPauseInfo() {}

  int32  phoneStartIndex;    //停顿音素起始索引（相对与对齐序列）默�?1

  std::vector<int32>   m_vecPhone;  //保存停顿里面的音素集
  std::vector<int32>   m_vecFrame;   //保存停顿里面的帧集合
};

//这个类主要用来识别停顿
class SpeechPause {
public:
  explicit SpeechPause(SpeechPublic*& pPublic);
  ~SpeechPause();

 //停顿初始化，主要得到语音对应得音素集合和模型读取得静音音素集合
 void  InitSpeechPause(std::string &mdl_dir,std::vector<int32>& alignments);

 //对停顿信息进行评分
 BaseFloat  CalculatePauseScore(std::vector<int32>& words, std::vector<std::string>& words_str, std::vector<std::vector<std::string> >& forceAli, std::vector<int32>& pauseIndexs );

 BaseFloat  New_CalculatePauseScore(std::vector<int32>& words, 
                                    std::vector<std::string>& words_str, 
                                    std::vector<std::vector<std::string> >& forceAli,
                                    std::vector<int32>& non_pauseIndexs);

 Json::Value   GetPauseInfoJson()   { return m_PauseJson; }

 //回收函数（变量重新初始化）
  void  Recovery();

protected:

   //根据音素集合和静音音素集合比较，得到停顿信息
  bool  GeneratePauseInfo(std::vector<std::vector<int32>>& split,TransitionModel& transModel);

  //取出模型文件中的静音音素ID集合
  void  GetSilPhoneSet(TransitionModel& transModel,std::vector<int32>& silPhone);

 //根据对齐文件取出停顿对应的帧信息
  void  GetFrameSetFromSplit(int32 phoneid, std::vector<std::vector<int32>>& split, TransitionModel& transModel, SpeechPauseInfo*& pPause);

  //打印最后的结果，保存到文件中
  bool   PrintResultToTxt(std::string& strResult);

  //生成区间的随机数 BaseFloat
  BaseFloat Random(int start, int end);

  //保存所有的停顿信息
  std::vector<SpeechPauseInfo*>  m_vecPause;

  //音素个数
  std::vector<int32>     m_vecPhones;

  Json::Value        m_PauseJson;

  SpeechPublic*   m_pSpeechPublic;

};

}

#endif  // KALDI_SPEECH_PAUSE_H_
