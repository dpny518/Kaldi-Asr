// fluency/speechStress.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0


//数据归一化时，将音素里面的帧长 能量  基因频率  取和  再做归一化处理

#ifndef KALDI_SPEECH_STRESS_H_
#define KALDI_SPEECH_STRESS_H_ 1

#include "fluency/speechPublic.h"
#include "fluency/dataExtract.h"
#include "common/json/json.h"

using namespace kaldi;

namespace fluency {

//typedef std::multimap<int32,std::vector<int32> > LexiconMap;   //一对多

#define  score_max     6

class FrameInfo{
public:
       explicit FrameInfo()
        {
            energy = 0;
            pitch = 0;
            distance = 0.01;  //s   10ms = 0.01s
        }
        ~FrameInfo() {}

         int32         transitionid;

         BaseFloat  distance;     //时长  默认0
         BaseFloat  energy;       //能量   默认 0
		 BaseFloat  pitch;          //基音频率  默认0

		 int32         transition_index;
};

class PhoneInfo{
public:
explicit PhoneInfo()
{
    phone_score = 0;
    phone_distance = 0;
    phone_energy = 0;
    phone_pitch = 0;
}
~PhoneInfo() {}

	std::string  strPhone;      // 音素名  默认空
	int32      phone_id;         //音素ID  默认-1

	BaseFloat    phone_score;     //音素评分

	BaseFloat    phone_distance;       //这里都取帧信息做和
	BaseFloat    phone_energy;
	BaseFloat    phone_pitch;

  std::vector<FrameInfo*>  m_vecFrame;
};

class  WordInfo{
    public:
        explicit  WordInfo()
        {
            str_word_name = "";
            vowelPhones_count = 0;
            stress_split_index = -1;
            stress_score = 0;

            //add
            word_distance = 0;
            word_energy = 0;
            word_pitch = 0;
        }
        ~WordInfo() {}

        int32             word_id;
        std::string      str_word_name;   //单词名称  默认为空

        std::vector<std::vector<PhoneInfo*>>  m_vecSplitPhones;   //音素分段

        int32      vowelPhones_count;          //单词中的元音音素个数
        int32      stress_split_index;        //重音所在的分段索引    默认-1  单词级的重音判断依据

        BaseFloat   stress_score;   //重音评分

        std::vector<PhoneInfo*>    m_vecPhones;

        //另一种重音检测方法，需要添加新的参数
        BaseFloat    word_distance;       //单词的时长
        BaseFloat    word_energy;         //单词的能量
        BaseFloat    word_pitch;          //单词的基音频率
};

class SpeechStress
{
public:
   SpeechStress(SpeechPublic*& pPublic);
  ~SpeechStress();

    void  InitSpeechStress(std::string &mdl_dir,std::vector<int32>& alignments, std::vector<int32>& words, std::vector<BaseFloat>& energy,std::string api_name);

   //根据保存的重音信息，选择并进行重音评分
   BaseFloat  CalculateSentenceStressScore(std::vector<std::vector<std::string> >&  forceAli,
                                                      std::vector<int32>& word_stress_indexs,
                                                      std::vector<std::string>&  m_word_str);

   //新的重音评分(三个指标 各取前N个最大的，找出相同的单词后，作为重音单词，不通过评分判断)
   BaseFloat  New_CalculateSentenceStressScore(std::vector<std::vector<std::string> >&  forceAli,
                                               std::vector<int32>& word_stress_indexs,
                                               std::vector<std::string>&  m_word_str);

   //计算单词级的重音评分
   BaseFloat  CalculateWordsStressScore();

   //获取重音的详细信息  json格式
   Json::Value   GetSpeechStressJson()  { return  Stress_Json; }

    //连读检测评分
    //bool   CheckLiaisonPossible();

    //读取连读里面的音素，并返回两个音素的transition-id 的索引（开始  结束）
   //bool  GetTransitionIndexWithPhones(std::string& strEndWord, std::string& strStartWord, std::string& index);

   //回收函数（变量重新初始化）
  void  Recovery();

protected:

  void  Init();

  //初始化map(单词和音素集的对应关系)
   void   UpdateLexiconMap(const std::vector<int32> &lexicon_entry);
   void   InitViabilityMap();

  //根据单词得到对应音素的个数
   int32   CountPhoneWithWord(int32 word_id,int32 phone_start_index);

  //将音素数据进行归一化操作
  void  NormalizedPhoneData();

  //提取基音频率，在初始化时操作,和帧对应起来
  bool  GetPitchFromWav(std::string api_name);

  //将音素集和单词对应起来
  void   PhonesMatchWord(std::vector<int32>& vec_word);

  //将单词里面的音素集进行分段
  void  SplitPhonesWithWords();

  //检测当前音素是否是元音
  bool  CheckPhoneIsVowel(int32 phone_id);

  //计算每个单词所在的分段索引
  void  CalculateWordSplitIndex(WordInfo*& wordInfo);

  BaseFloat  GetWordScoreWithStr(std::string& word_str);   //根据单词得到对应的单词分数

  //根据三个特征去除对应的重音位置
  void GetStressIndexFromSentence(std::vector<BaseFloat>& vec_distance, 
                                  std::vector<BaseFloat>& vec_energy,
                                  std::vector<BaseFloat>& vec_pitch,
                                  int nCount_compare,
                                  std::vector<int>& vec_result_stress);

  std::vector<WordInfo*>    vec_WordInfo;     //保存所有的单词信息
  std::vector<PhoneInfo*>   vec_PhoneInfo;    //保存所有的音素信息
  std::vector<FrameInfo*>   vec_FrameInfo;    //保存所有的帧序列

  //std::vector<PhoneInfo*>   vec_vowelPhones;    //保存所有的元音音素信息

  Json::Value       Stress_Json;

  SpeechPublic*   p_Speech_Public;

  //DataExtract*      p_DataExtract;

};

}  // End namespace kaldi

#endif  // KALDI_SPEECH_STRESS_H_
