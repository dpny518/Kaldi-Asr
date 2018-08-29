// fluency/trainParameters.h

// Copyright 2017.12.13  Zhang weiyu
// Version 1.0

#ifndef KALDI_TRAIN_PARAMETERS_H_
#define KALDI_TRAIN_PARAMETERS_H_ 1

#include "fluency/speechStress.h"
using namespace kaldi;

namespace fluency {

enum train_type{
    type_one=0,      //每个音节
    type_two,           //每个元音
    type_three,          //最大元音
    type_four             //最大音节
};

//训练参数类
class TrainParams{
public:
  explicit TrainParams();
  ~TrainParams();

  void  CalculateStressScoreWithWordInfo(std::vector<WordInfo*>& vec_words_info);   //根据单词信息计算重音评分

  void  CalculateWordInfo(std::vector<WordInfo*>& vec_words_info);     //计算每个单词中的时长 能量 基音频率

protected:

    void  CalculateEveryWordScore_One(WordInfo*& word_info);   //计算每个单词的重音评分

    void  CalculateEveryWordScore_Two(WordInfo*& word_info);   //计算每个单词的重音评分

    void  CalculateEveryWordScore_Three(WordInfo*& word_info);   //计算每个单词的重音评分

    void  CalculateEveryWordScore_Four(WordInfo*& word_info);   //计算每个单词的重音评分

    bool  CheckPhoneIsVowel(PhoneInfo*& phone_info);    //检测当前音素是否是元音

    int32   train_type;
};

}  // End namespace kaldi

#endif  // KALDI_TRAIN_PARAMETERS_H_
