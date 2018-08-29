// fluency/trainParameters.cc

// Copyright 2017.12.13  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "decoder/decoder-wrappers.h"
#include "hmm/hmm-utils.h"
#include "feat/pitch-functions.h"
#include "feat/wave-reader.h"
#include "lat/word-align-lattice-lexicon.h"
#include "fluency/trainParameters.h"

namespace fluency {

TrainParams::TrainParams()
{
    train_type = type_one;
}

TrainParams::~TrainParams()
{
}

void  TrainParams::CalculateWordInfo(std::vector<WordInfo*>& vec_words_info)
{
    for(int i=0; i<vec_words_info.size(); i++)
    {
        BaseFloat sum_distance = 0;
        BaseFloat sum_energy = 0;
        BaseFloat sum_pitch = 0;

        for(int j=0; j<vec_words_info.at(i)->m_vecPhones.size(); j++)
        {
            sum_distance = sum_distance + vec_words_info.at(i)->m_vecPhones.at(j)->phone_distance;
            sum_energy = sum_energy + vec_words_info.at(i)->m_vecPhones.at(j)->phone_energy;
            sum_pitch = sum_pitch + vec_words_info.at(i)->m_vecPhones.at(j)->phone_pitch;
        }

        BaseFloat nCountPhones = vec_words_info.at(i)->m_vecPhones.size();

        vec_words_info.at(i)->word_distance = sum_distance / nCountPhones;
        vec_words_info.at(i)->word_energy = sum_energy / nCountPhones;
        vec_words_info.at(i)->word_pitch = sum_pitch / nCountPhones;
    }
}


void  TrainParams::CalculateStressScoreWithWordInfo(std::vector<WordInfo*>& vec_words_info)
{
        // y = 3*dis + 2*energy + 1*pitch   后期准备通过模型来得到y值
        for(int32 i=0; i<vec_words_info.size(); i++)
        {
            for(int32 j=0; j<vec_words_info.at(i)->m_vecPhones.size(); j++)
            {
                vec_words_info.at(i)->m_vecPhones.at(j)->phone_score = 3*vec_words_info.at(i)->m_vecPhones.at(j)->phone_distance +
                                                                                                             2*vec_words_info.at(i)->m_vecPhones.at(j)->phone_energy +
                                                                                                             1*vec_words_info.at(i)->m_vecPhones.at(j)->phone_pitch;
            }
        }

        switch(train_type)
        {
            case type_one:    //每个音节
            {
                for(int32 i=0; i<vec_words_info.size(); i++)
                {
                    CalculateEveryWordScore_One(vec_words_info.at(i));
                }
                break;
            }
            case type_two:   //每个元音
            {
                for(int32 i=0; i<vec_words_info.size(); i++)
                {
                    CalculateEveryWordScore_Two(vec_words_info.at(i));
                }
                break;
            }
            case type_three:  //最大元音
            {
                for(int32 i=0; i<vec_words_info.size(); i++)
                {
                    CalculateEveryWordScore_Three(vec_words_info.at(i));
                }
                break;
            }
            case type_four:   //最大音节
            {
                for(int32 i=0; i<vec_words_info.size(); i++)
                {
                    CalculateEveryWordScore_Four(vec_words_info.at(i));
                }
                break;
            }
        }
}

void  TrainParams::CalculateEveryWordScore_One(WordInfo*& word_info)
{
    //每个计算音节
    BaseFloat phones_sum_score = 0;
    for(int32 i=0;i<word_info->m_vecSplitPhones.size(); i++)
    {
        BaseFloat syllable_score = 0;
        for(int32 j=0; j<word_info->m_vecSplitPhones.at(i).size(); j++)
        {
            syllable_score = syllable_score + word_info->m_vecSplitPhones.at(i).at(j)->phone_score;
        }
        syllable_score = syllable_score/word_info->m_vecSplitPhones.at(i).size();

        phones_sum_score = phones_sum_score + syllable_score;
    }

    BaseFloat word_score = phones_sum_score / word_info->m_vecSplitPhones.size();

    word_info->stress_score = word_score;
}

void  TrainParams::CalculateEveryWordScore_Two(WordInfo*& word_info)
{
    //计算每个元音
    BaseFloat phones_sum_score = 0;

    int32 vowel_count = 0;    //元音个数
    for(int32 i=0; i<word_info->m_vecPhones.size(); i++)
    {
        if(CheckPhoneIsVowel(word_info->m_vecPhones.at(i)))
        {
            phones_sum_score = phones_sum_score + word_info->m_vecPhones.at(i)->phone_score;
            vowel_count ++;
        }
    }

    BaseFloat word_score = phones_sum_score / vowel_count;

    word_info->stress_score = word_score;
}

void  TrainParams::CalculateEveryWordScore_Three(WordInfo*& word_info)
{
    //最大元音
    BaseFloat phones_max_score = 0;

    int32 vowel_count = 0;    //元音个数
    for(int32 i=0; i<word_info->m_vecPhones.size(); i++)
    {
        if(CheckPhoneIsVowel(word_info->m_vecPhones.at(i)))
        {
            if(word_info->m_vecPhones.at(i)->phone_score > phones_max_score)
                phones_max_score = word_info->m_vecPhones.at(i)->phone_score;
        }
    }

    word_info->stress_score = phones_max_score;
}

void  TrainParams::CalculateEveryWordScore_Four(WordInfo*& word_info)
{
    //最大音节
    BaseFloat max_syllable_score = 0;
    for(int32 i=0;i<word_info->m_vecSplitPhones.size(); i++)
    {
        BaseFloat syllable_score = 0;
        for(int32 j=0; j<word_info->m_vecSplitPhones.at(i).size(); j++)
        {
            syllable_score = syllable_score + word_info->m_vecSplitPhones.at(i).at(j)->phone_score;
        }
        syllable_score = syllable_score/word_info->m_vecSplitPhones.size();

        if(syllable_score > max_syllable_score)
            max_syllable_score = syllable_score;
    }

    word_info->stress_score = max_syllable_score;
}

bool  TrainParams::CheckPhoneIsVowel(PhoneInfo*& phone_info)
{
    std::string str_phone = phone_info->strPhone;

    if(!str_phone.empty())
        if((str_phone[0] == 'A') || (str_phone[0] == 'E') || (str_phone[0] == 'I') || (str_phone[0] == 'O') || (str_phone[0] == 'U'))
            return true;
    return false;
}

}  // End namespace kaldi
