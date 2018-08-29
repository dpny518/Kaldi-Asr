// fluency/speechPause.cc

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "gmm/am-diag-gmm.h"
#include "hmm/hmm-utils.h"
#include "lat/kaldi-lattice.h"
#include "lat/word-align-lattice-lexicon.h"
#include "fluency/speechPublic.h"
#include <sstream>

namespace fluency {

SpeechPublic::SpeechPublic(string lang_dir)
{
    lang_dir_ = lang_dir;       //保存lang文件目录
    phones_syms = NULL;
    words_syms = NULL;

    Init();
}

SpeechPublic::~SpeechPublic()
{
    if(phones_syms)
        delete phones_syms;

    if(words_syms)
        delete words_syms;
}

void  SpeechPublic::Init()
{
    //加载phones.txt文件
   std::string phones_syms_filename = lang_dir_+"/phones.txt";
   phones_syms  = NULL;

    if (phones_syms_filename != "")
      if (!(phones_syms = fst::SymbolTable::ReadText(phones_syms_filename)))
        KALDI_ERR << "Could not read symbol table from file "<< phones_syms_filename;

    //读取words.txt文件
    std::string word__syms_filename = lang_dir_+"/words.txt";
    words_syms = NULL;

    if (word__syms_filename != "")
        if (!(words_syms = fst::SymbolTable::ReadText(word__syms_filename)))
            KALDI_ERR << "Could not read symbol table from file "<< word__syms_filename;


    //加载align_lexicon.int内容到map中
    InitViabilityMap();

    //加载静音音素
    std::ifstream inf(lang_dir_+"/silence.int");
    std::string  strPhoneID;
    while (getline(inf, strPhoneID))      //getline(inf,s)是逐行读取inf中的文件信息
    {
        int32 sil_phone_id = atoi(strPhoneID.c_str());
        m_vecSilPhone.push_back(sil_phone_id);
    }
    inf.close();

    //加载重音 音节索引
    InitWordSymbol();

    //加载连读音素表， 判断单词间连读可能性
    InitLiaisonPhones();
}

void  SpeechPublic::InitWordSymbol()
{
    //加载total_syllable_map  和  stress_syllable_map
    std::string stress_symbol_filename = lang_dir_+"/stress_index.txt";

    std::ifstream symbol_file(stress_symbol_filename);

    if (!symbol_file.is_open())
        KALDI_ERR << "Error open stress_index.txt";

    std::string line_data;
    while(getline(symbol_file,line_data))
    {
        string strWord,strSymbol;
        std::istringstream is(line_data);
        is>>strWord>>strSymbol;

        //去掉特殊字符
        strSymbol.erase(std::remove(strSymbol.begin(), strSymbol.end(), '['), strSymbol.end());
        strSymbol.erase(std::remove(strSymbol.begin(), strSymbol.end(), ']'), strSymbol.end());

        int nPos = strSymbol.find(std::string(":"));

        int iTotal = atoi(strSymbol.substr(0,nPos).c_str());
        int iIndex = atoi(strSymbol.substr(nPos+1, strSymbol.length()).c_str());

        total_syllable_map.insert(std::pair<std::string, int>(strWord, iTotal));
        stress_syllable_map.insert(std::pair<std::string, int>(strWord, iIndex));
    }

    symbol_file.close();

    std::cout<<"total 的个数是："<<total_syllable_map.size()<<std::endl;
    std::cout<<"stress 的个数是："<<stress_syllable_map.size()<<std::endl;

}

void  SpeechPublic::InitViabilityMap()
{
    std::string align_lexicon_rxfilename = lang_dir_+"/align_lexicon.int";
    std::vector<std::vector<int32> > lexicon;
    {
      bool binary_in;
      Input ki(align_lexicon_rxfilename, &binary_in);
      KALDI_ASSERT(!binary_in && "Not expecting binary file for lexicon");
      if (!ReadLexiconForWordAlign(ki.Stream(), &lexicon)) {
        KALDI_ERR << "Error reading alignment lexicon from "
                  << align_lexicon_rxfilename;
      }
    }

    for (size_t i = 0; i < lexicon.size(); i++)
    {
        const std::vector<int32> &lexicon_entry = lexicon[i];
        KALDI_ASSERT(lexicon_entry.size() >= 2);

        UpdateLexiconMap(lexicon_entry);
    }
}

void  SpeechPublic::UpdateLexiconMap(const std::vector<int32> &lexicon_entry)
{
      std::vector<int32> word_key;
      word_key.reserve(lexicon_entry.size() - 2);

      word_key.insert(word_key.end(), lexicon_entry.begin() + 2, lexicon_entry.end());
      int32 new_word = lexicon_entry[1]; // This will typically be the same as

      lexicon_map_.insert(std::pair<int32,std::vector<int32>>(new_word, word_key));
}

void  SpeechPublic::InitLiaisonPhones()
{
    std::ifstream liaisonfile(lang_dir_+"/liaison_phone.txt");
    //assert(liaisonfile.is_open());   //若失败,则输出错误消息,并终止程序运行

    if (!liaisonfile.is_open())
        KALDI_ERR << "Error reading liaison phones from "
                  << "liaison_phone.txt";

    std::string strLiaisonPhones;
    while(getline(liaisonfile,strLiaisonPhones))
    {
        //cout<<s<<endl;
        m_vecLiaisonPhone.push_back(strLiaisonPhones);
    }
    liaisonfile.close();             //关闭文件输入流
}

int32 SpeechPublic::CountPhoneWithWord(int32 word_id, std::vector<int32>& vec_phones_id ,int32 phone_start_index)
{
    //通过 word 和音素集  对应起来
    LexiconMap::iterator iterBeg = lexicon_map_.lower_bound(word_id);
    LexiconMap::iterator iterEnd = lexicon_map_.upper_bound(word_id);

    for(;iterBeg != iterEnd;iterBeg++)
    {
            bool  isEqual = true;

            std::vector<int32>   select_phones = iterBeg->second;
            for(int i = 0; i<select_phones.size(); i++)
            {
                if((phone_start_index + i) < vec_phones_id.size())
                {
                    if(select_phones.at(i) != vec_phones_id.at(phone_start_index + i))
                    {
                        isEqual = false;
                        continue;
                    }
                }
                else
                {
                    std::cout<<"计算音素个数时，越界了！！！" <<std::endl;
                    return 0;
                }

            }

            if(isEqual)
                return select_phones.size();
    }
    return iterEnd->second.size();
}

bool  SpeechPublic::CheckCurPhoneIsSil(int32 phone_id)
{
        bool  isNor = false;
        std::vector<int32>::iterator ret;
        ret = std::find(m_vecSilPhone.begin(), m_vecSilPhone.end(), phone_id);

        if(ret == m_vecSilPhone.end())   //是正常音素
            isNor = true;

       return isNor;
}

bool  SpeechPublic::CheckCurPhoneIsVowel(int phone_id)
{
    std::string str_phone = phones_syms->Find(phone_id);
    if(!str_phone.empty())
        if((str_phone[0] == 'A') || (str_phone[0] == 'E') || (str_phone[0] == 'I') || (str_phone[0] == 'O') || (str_phone[0] == 'U'))
            return true;
    return false;
}

 std::string  SpeechPublic::GetPhoneStrWithId(int32  phone_id)
 {
    std::string strResult = "";
    if(phones_syms)
        strResult = phones_syms->Find(phone_id);
    return strResult;
 }

 std::string  SpeechPublic::GetWordStrWithId(int32  word_id)
 {
    std::string strResult = "";
    if(words_syms)
        strResult = words_syms->Find(word_id);
    return strResult;
 }

int32    SpeechPublic::GetPhoneIDWithStr(std::string& phone_str)
{
    int32 intResult = -1;
    if(phones_syms)
        intResult = phones_syms->Find(phone_str);
    return intResult;
}

int32    SpeechPublic::GetWordIDWithStr(std::string& word_str)
{
    int32 intResult = -1;
    if(words_syms)
        intResult = words_syms->Find(word_str);
    return intResult;
}

bool    SpeechPublic::JudgePossibilityOfLiaison(std::string& strEndWord, std::string& strStartWord, std::string&  strEndPhone,  std::string& strStartPhone)
{
    if(m_vecLiaisonPhone.size() > 0)
    {
        std::string  strLiaison = strEndPhone + std::string("  ") + strStartPhone;

        std::vector<std::string>::iterator iter;
        iter=find(m_vecLiaisonPhone.begin(),m_vecLiaisonPhone.end(),strLiaison);

        if (iter!=m_vecLiaisonPhone.end())    //是否找到当前音素对
        {
            std::cout<<"exist the possibility of liaison："<<strEndWord <<"  "<<strStartWord<<std::endl;
            return  true;
        }
        else
        {
            std::cout<<"not exist the possibility of liaison："<<strEndWord <<"  "<<strStartWord<<std::endl;
            return false;
        }
    }
    return false;
}

int   SpeechPublic::GetLiaisonPhoneIndex(std::string& strEndPhone, std::string& strStartPhone)
{
    std::string strFindIndex = strEndPhone + std::string("  ") + strStartPhone;

    //std::vector<std::string>::iterator iter;
    //iter=find(m_vecLiaisonPhone.begin(),m_vecLiaisonPhone.end(),strFindIndex);

    std::vector<std::string>::iterator iter = find(m_vecLiaisonPhone.begin(),  m_vecLiaisonPhone.end(),strFindIndex);

     if( iter != m_vecLiaisonPhone.end() )
     {
          int nPosition = distance(m_vecLiaisonPhone.begin(),iter);
          return nPosition;
     }
    else
    {
        return -1;
    }
    return -1;
}

int SpeechPublic::GetTotalSyllable(std::string& str_word)
{
    //根据单词查找对应总音节数
    auto iter = total_syllable_map.find(str_word);
    if(iter!=total_syllable_map.end())
    {
        return iter->second;
    }
    return -1;
}

int SpeechPublic::GetStressSyllable(std::string& str_word)
{
    //根据单词查找重音节索引
    auto  iter = stress_syllable_map.find(str_word);
    if(iter!=stress_syllable_map.end())
    {
        return iter->second;
    }
    return -1;
}

}