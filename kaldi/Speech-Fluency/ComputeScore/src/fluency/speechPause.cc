// fluency/speechPause.cc

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "gmm/am-diag-gmm.h"
#include "hmm/hmm-utils.h"
#include "fluency/speechPause.h"
#include "lat/kaldi-lattice.h"
#include "lat/word-align-lattice-lexicon.h"
#include <fstream>
#include <string>
#include <stdlib.h>

namespace fluency {

typedef typename fst::StdArc Arc;
typedef typename Arc::StateId StateId;
typedef typename Arc::Weight Weight;

SpeechPause::SpeechPause(SpeechPublic*& pPublic)
{
    m_pSpeechPublic = pPublic;
}

SpeechPause::~SpeechPause()
{
   /* if(m_pSpeechPublic)
    {
        delete m_pSpeechPublic;
        m_pSpeechPublic = NULL;
    }*/
}

void  SpeechPause::InitSpeechPause(std::string& mdl_dir,std::vector<int32>& alignments)
{
    //初始化停顿信息

    //加载模型文件，得到transition-id的分离信息
    std::string model_in_filename = mdl_dir+"/final.mdl";
    TransitionModel trans_model;
    {
      bool binary;
      Input ki(model_in_filename, &binary);
      trans_model.Read(ki.Stream(), binary);
    }

    std::vector<std::vector<int32> > split;
    SplitToPhones(trans_model, alignments, &split);

    //根据分离信息得到音素集合（音素ID集合）

    for(int i=0; i<split.size(); i++)
    {
        //这里取每个集合的头元素，根据头元素计算音素ID
        int32  front_transition_id = split.at(i).front();
        int32  phoneID = trans_model.TransitionIdToPhone(front_transition_id);

        m_vecPhones.push_back(phoneID);
    }

    //根据两个音素集合来生成停顿类指针
    GeneratePauseInfo(split, trans_model);
}

bool  SpeechPause::GeneratePauseInfo(std::vector<std::vector<int32>>& split,TransitionModel& transModel)
{
    //根据音素集合和静音音素集合比较，得到停顿信息

    std::vector<std::vector<int32>>  vecPauseIndex;
    std::vector<int32>  vecPhoneIndex;

    if((m_vecPhones.size() > 0 ) )
    {
        for(int32 i=0; i<m_vecPhones.size(); i++)   //所有的停顿音素都保存,  不保存结尾静音音素
        {
            if(!m_pSpeechPublic->CheckCurPhoneIsSil(m_vecPhones.at(i)) )
            {
                vecPhoneIndex.push_back(i);
            }
            else
            {
                if(vecPhoneIndex.size() >0 )
                {
                    vecPauseIndex.push_back(vecPhoneIndex);
                    //vecPhoneIndex.clear();  不使用clear  swap可以清空内存
                    std::vector<int32> ().swap(vecPhoneIndex);
                }
            }
        }

        //根据得到的索引容器，生成相应的停顿类信息

        for(int32 i=0; i<vecPauseIndex.size(); i++)
        {
            SpeechPauseInfo* pSpeechPause = new SpeechPauseInfo();

            pSpeechPause->phoneStartIndex = vecPauseIndex.at(i).front();

            for(int32 j=0;j<vecPauseIndex.at(i).size(); j++)
            {
                int32 phoneid = m_vecPhones.at(vecPauseIndex.at(i).at(j));
                pSpeechPause->m_vecPhone.push_back(phoneid);

                //通过phoneID找到对应的帧序列
                GetFrameSetFromSplit(phoneid, split, transModel, pSpeechPause);
            }
            m_vecPause.push_back(pSpeechPause);
        }

        return true;
    }
    return false;
}

void  SpeechPause::GetFrameSetFromSplit(int32 phoneid, std::vector<std::vector<int32>>& split, TransitionModel& transModel, SpeechPauseInfo*& pPause)
{
    for(int32 i=0; i<split.size(); i++)
    {
        int32  trans_id = split.at(i).front();
        if(phoneid == (transModel.TransitionIdToPhone(trans_id)))
        {
             //插入帧序列
             for(int32 j=0; j<split.at(i).size(); j++)
             {
                 pPause->m_vecFrame.push_back(split.at(i).at(j));
             }
        }
    }
}

BaseFloat  SpeechPause::CalculatePauseScore(std::vector<int32>& words,
                                                                         std::vector<std::string>& words_str,
                                                                         std::vector<std::vector<std::string> >& forceAli,
                                                                         std::vector<int32>& pauseIndexs)
{
    //根据生成的停顿类信息 计算停顿评分
    int32 word_index = 0;
    int32 phone_index = 0;

    std::cout << "开始计算停顿信息 ：  CalculatePauseScore" << std::endl;

    std::vector<std::string>  word_pause;    //保存停顿前的单词ID
    std::cout << "m_vecPause.size: " << m_vecPause.size() << std::endl;
    for(int32 i=0; i<m_vecPause.size(); i++)
    {
        //去掉头停顿
        if(m_vecPause.at(i)->phoneStartIndex == 0)
        {
            phone_index = m_vecPause.at(i)->m_vecPhone.size();
            continue;
        }

        for(; word_index < words.size(); word_index++)
        {
            std::vector<int32> phones_id;

            int32 add_index = m_pSpeechPublic->CountPhoneWithWord(words.at(word_index), m_vecPhones, phone_index);

            phone_index = phone_index + add_index;

            if(phone_index > m_vecPause.at(i)->phoneStartIndex)
            {
                std::vector<int32> ().swap(phones_id);

                std::string str_word = m_pSpeechPublic->GetWordStrWithId(words.at(word_index-1));

                if(!str_word.empty())
                    word_pause.push_back(str_word);

                break;
            }

        }
    }

   std::vector<int32>  vec__decode_pause_index;

   std::cout << "得到停顿位置（索引）" << std::endl;

   for(int32 i=0; i<forceAli.size(); i++)
   {
        if(!forceAli.at(i).back().empty())
        {
                for(int32 j = 0; j<word_pause.size(); j++)
                {
                    if(!forceAli.at(i).back().compare(word_pause.at(j)))    //判断停顿前的单词和分词后的最后一个单词是否是一样的
                        vec__decode_pause_index.push_back(i);
                }
        }
   }

   BaseFloat  pause_success = 0;
   std::vector<int32>  success_cmp_index;

   std::cout << "匹配停顿位置" << std::endl;

   for(int32 i=0;i<pauseIndexs.size(); i++)
   {
        for(int32 j=0; j<vec__decode_pause_index.size(); j++)
        {
            if(pauseIndexs.at(i) ==vec__decode_pause_index.at(j) )
            {
                pause_success ++;
                success_cmp_index.push_back(pauseIndexs.at(i));
                break;
            }
        }
   }
    std::cout << "停顿拼json" << std::endl;

    std::cout<< "成功匹配的个数是："<<success_cmp_index.size()<<std::endl;

    std::cout<< "单词个数是:"<<words_str.size()<<std::endl;

    for(int32 i=0; i<success_cmp_index.size(); i++)
    {
         Json::Value data;

        data["index"] = success_cmp_index.at(i);  //单词索引

        if(success_cmp_index.at(i) < words_str.size())
        {
            std::cout<< "停顿前的单词是:"<<words_str.at(success_cmp_index.at(i))<<std::endl;
            data["word"] = words_str.at(success_cmp_index.at(i));   //停顿前的单词
        }
        

        //通过匹配的单词找到对应的停顿时长
        std::cout<< "停顿的单词 索引和时长";

        data["distance"] = 5;     //停顿时长

        m_PauseJson.append(data);
    }

    if(pauseIndexs.size() == 0)
        return 0;

    BaseFloat  rtnScore = pause_success/pauseIndexs.size();

    return rtnScore;
}

BaseFloat  SpeechPause::New_CalculatePauseScore(std::vector<int32>& words,
                                                                         std::vector<std::string>& words_str,
                                                                         std::vector<std::vector<std::string> >& forceAli,
                                                                         std::vector<int32>& non_pauseIndexs)
{
    //根据生成的停顿类信息 计算停顿评分
    int32 word_index = 0;
    int32 phone_index = 0;

    std::vector<std::string>  word_pause;    //保存停顿前的单词ID

    for(int32 i=0; i<m_vecPause.size(); i++)
    {
        //去掉头停顿
        if(m_vecPause.at(i)->phoneStartIndex == 0)
        {
            phone_index = m_vecPause.at(i)->m_vecPhone.size();
            continue;
        }

        for(; word_index < words.size(); word_index++)
        {
            std::vector<int32> phones_id;

            int32 add_index = m_pSpeechPublic->CountPhoneWithWord(words.at(word_index), m_vecPhones, phone_index);

            phone_index = phone_index + add_index;

            if(phone_index > m_vecPause.at(i)->phoneStartIndex)
            {
                std::vector<int32> ().swap(phones_id);

                std::string str_word = m_pSpeechPublic->GetWordStrWithId(words.at(word_index-1));

                if(!str_word.empty())
                    word_pause.push_back(str_word);

                break;
            }

        }
    }

   std::vector<int32>  vec__decode_pause_index;

   for(int32 i=0; i<forceAli.size(); i++)
   {
        if(!forceAli.at(i).back().empty())
        {
                for(int32 j = 0; j<word_pause.size(); j++)
                {
                    if(!forceAli.at(i).back().compare(word_pause.at(j)))    //判断停顿前的单词和分词后的最后一个单词是否是一样的
                        vec__decode_pause_index.push_back(i);
                }
        }
   }

   BaseFloat  pause_fail = 0;

   for(int32 i=0;i<non_pauseIndexs.size(); i++)
   {
        for(int32 j=0; j<vec__decode_pause_index.size(); j++)
        {
            if(non_pauseIndexs.at(i) ==vec__decode_pause_index.at(j) )
            {
                pause_fail ++;
                break;
            }
        }
   }

    //在90 到 100之间减分
    BaseFloat basic_score = Random(85, 95);
    BaseFloat rtnScore = basic_score - pause_fail*Random(5, 6);

    //BaseFloat  rtnScore = pause_success/pauseIndexs.size();

    return rtnScore;
}

BaseFloat SpeechPause::Random(int start, int end)
{
    int dis = end - start;
    //printf("dis: %d\n", dis);
    return start + dis * (rand() / (RAND_MAX + 1.0));
}

bool  SpeechPause::PrintResultToTxt(std::string& strResult)
{
    if(!strResult.empty())
    {
        std::ofstream location_out;

        location_out.open("speechPause.txt", std::ios::out | std::ios::app);  //以写入和在文件末尾添加的方式打开.txt文件，没有的话就创建该文件。
        if (!location_out.is_open())
            return false;

        location_out << strResult << std::endl;
        location_out << "检测到" << m_vecPause.size() <<"停顿"<<std::endl;

        for(int32 i=0; i<m_vecPause.size(); i++)
        {
            location_out << "停顿" << (i+1) <<"时长："<<m_vecPause.at(i)->m_vecFrame.size() * 0.01<<" ms"<<std::endl;
        }

        location_out.close();
        return true;
    }
    return false;
}

void  SpeechPause::Recovery()
{
    //收回m_vecPause空间
     //m_vecPause.clear();
     //std::vector <SpeechPauseInfo*>().swap(m_vecPause);

     for(std::vector <SpeechPauseInfo*>::iterator s = m_vecPause.begin(),
                                                  e = m_vecPause.end(); s!=e; ++s)
     {
        delete *s;
        *s = NULL;
     }
     m_vecPause.clear();

     //收回m_vecPause空间
     m_vecPhones.clear();
     std::vector <int32>().swap(m_vecPhones);

     m_PauseJson = "";
}

}  // End namespace kaldi
