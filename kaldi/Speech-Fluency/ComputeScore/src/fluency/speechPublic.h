// fluency/speechPublic.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#ifndef KALDI_SPEECH_PUBLIC_H_
#define KALDI_SPEECH_PUBLIC_H_ 1

using namespace kaldi;

namespace fluency {

typedef std::multimap<int32,std::vector<int32> > LexiconMap;   //一对多

typedef std::map<std::string, int>    StressSymbolMap;   //重音的标志映射

//这个类是语音公共类, 加载停顿和重音需要的公共信息

class SpeechPublic{
public:

  explicit SpeechPublic(std::string lang_dir);    // 构造函数传起始索引
  ~SpeechPublic();

  int32 CountPhoneWithWord(int32 word_id, std::vector<int32>& vec_phones_id ,int32 phone_start_index);    //根据单词查找匹配的音素个数

  bool  CheckCurPhoneIsSil(int32 phone_id);               //检查当前音素是否是静音音素  ,true 表示正常音素

  bool  CheckCurPhoneIsVowel(int phone_id);     //检查当前音素是否是元音音素，true 表示是元音音素
  std::string  lang_dir_;

  std::string  GetPhoneStrWithId(int32  phone_id);          //根据音素ID获取音素名称
  std::string  GetWordStrWithId(int32  word_id);            //根据单词ID获取单词名称

  int32    GetPhoneIDWithStr(std::string& phone_str);       //根据音素名称得到音素ID
  int32    GetWordIDWithStr(std::string& word_str);          //根据单词名称得到单词ID

  int  GetTotalSyllable(std::string& str_word);          //根据单词查找对应总音节数
  int  GetStressSyllable(std::string& str_word);         //根据单词查找重音节索引

  //根据音素 判断是否存在连读
  bool    JudgePossibilityOfLiaison(std::string& strEndWord, std::string& strStartWord, std::string&  strEndPhone,  std::string& strStartPhone);

  //根据音素获得音素表的索引
  int   GetLiaisonPhoneIndex(std::string& strEndPhone, std::string& strStartPhone);

protected:
    void  Init();

    void  InitViabilityMap();

    //初始化连读音素表，用于判断单词的连读性
    void  InitLiaisonPhones();

    void  UpdateLexiconMap(const std::vector<int32> &lexicon_entry);

    //加载单词对应音节表
    void  InitWordSymbol();

    fst::SymbolTable *phones_syms;
    fst::SymbolTable *words_syms;

    LexiconMap  lexicon_map_;    //主要保存align_lexicon.int  文件内容

    StressSymbolMap   total_syllable_map;    //保存单词和对应的所有音节数
    StressSymbolMap   stress_syllable_map;   //保存单词和重音音节的索引

    std::vector<int32>    m_vecSilPhone;

    std::vector<std::string>   m_vecLiaisonPhone;    //保存连读音素表  （一行是一个元素）
};

}  // End namespace kaldi

#endif  // KALDI_SPEECH_PUBLIC_H_
