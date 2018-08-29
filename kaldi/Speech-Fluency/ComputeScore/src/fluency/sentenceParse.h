// fluency/sentenceParse.h

// Copyright 2017.12.13  Zhang weiyu
// Version 1.0

#ifndef KALDI_SENTENCE_PARSE_H_
#define KALDI_SENTENCE_PARSE_H_ 1

#include "fluency/speechPublic.h"

using namespace kaldi;

namespace fluency {

class SentenceParse{
public:
  explicit SentenceParse(SpeechPublic*& pPublic,std::vector<std::string>& vec_decode_words ,std::string&  str_sentence);
  ~SentenceParse();
   void  Recovery();
  //std::vector<int32>   GetStressIndexs()   { return  m_vecWord_stress_indexs; }

  std::vector<std::vector<std::string> >   vec_lcs_ali;     //保存强制对齐后单词信息

  std::vector<int32>          m_vecWord_stress_indexs;       //保存重音的单词索引
  std::vector<int32>          m_vecWord_pause_index;        //保存停顿的索引

  //add by zhangweiyu   2018.8.17  新的停顿评分方法
  std::vector<int>          m_vecWord_notpossible_pause_index;   //保存不可停顿的位置

  std::vector<std::string>  m_vec_word_str;        //保存读取的单词信息

    void  PrintParseWords();
protected:
    //从文件中解析正确的语句
    void  ParseSentenceFromFile(std::string&  str_sentence);    //从文件中解析语句

    void  SplitWordFromSentence(std::string std_words);         //从语句中分离单词

    void  ParseWordsFromSentence(std::string  str_sentence);      //从语句中得到所有的单词信息

    void  ParsePauseInfoFromSentence(std::string  str_sentence);  //从语句中得到停顿信息

    void  ParseStressInfoFromSentence(std::string  str_sentence);         //从语句中得到重音信息

    void  DelSubStrFromSentence(std::string& str_sentence, std::string& substr);       //从语句中删除所有子字符串

    void   ForceAlignment();           //强制对齐

    void   SplitDecodeWord();        //根据动态规划结果  对单词做分割处理

    int32  Length_LCS(int32 **&count,int32 **&dir,int32 row,int32 col);  //输出LCS的长度
    void   Print_LCS(int32 **dir, int32 row, int32 col);

    //生成非停顿的索引， 保存在 m_vecWord_notpossible_pause_index 中
    void   GenerateNonPause();

    //判断元素是否在容器中
    bool is_element_in_vector(std::vector<int> v,int element);

    std::vector<std::string>   m_vec_decode_words;     //这里比较字符串 解码的字符串

    std::vector<int32>           m_vecWordIndex;           //保存动态规划后的匹配单词索引
    std::vector<int32>           m_vecDecodeWordIndex;   //保存动态规划后的解码单词索引

    SpeechPublic*   m_pSpeechPublic;
};

}

#endif  // KALDI_SENTENCE_PARSE_H_
