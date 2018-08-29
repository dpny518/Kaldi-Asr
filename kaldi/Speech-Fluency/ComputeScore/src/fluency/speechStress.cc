// fluency/speechStress.cc

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "decoder/decoder-wrappers.h"
#include "hmm/hmm-utils.h"
#include "feat/pitch-functions.h"
#include "feat/wave-reader.h"
#include "lat/word-align-lattice-lexicon.h"
#include "fluency/sentenceParse.h"
#include "fluency/trainParameters.h"
#include "fluency/speechStress.h"

namespace fluency {

typedef typename fst::StdArc Arc;
typedef typename Arc::StateId StateId;
typedef typename Arc::Weight Weight;


SpeechStress::SpeechStress(SpeechPublic*& pPublic)
{
    p_Speech_Public = pPublic;

    //p_DataExtract = new DataExtract();
}

SpeechStress::~SpeechStress()
{
   /* if(p_Speech_Public)
    {
        delete p_Speech_Public;
        p_Speech_Public = NULL;
    }*/

    //if(p_DataExtract)
    //{
    //    delete p_DataExtract;
    //    p_DataExtract = NULL;
    //}
}


int32   SpeechStress::CountPhoneWithWord(int32 word_id, int32 phone_start_index)
{
    std::vector<int32>  vec_phones_id;

    for(int32 i=0; i<vec_PhoneInfo.size(); i++)
    {
        vec_phones_id.push_back(vec_PhoneInfo.at(i)->phone_id);
    }

    int32  count_phone = 0;

    count_phone = p_Speech_Public->CountPhoneWithWord(word_id, vec_phones_id, phone_start_index);

    return count_phone;
}

void  SpeechStress::InitSpeechStress(std::string &mdl_dir,std::vector<int32>& alignments , std::vector<int32>& words, std::vector<BaseFloat>& energy,std::string api_name)
{

    //根据解码得到的信息创建帧对象指针
    int frame_len = alignments.size();
    for(int i = 0; i< frame_len; i++)
    {
        FrameInfo*  pFrameInfo = new FrameInfo();
        pFrameInfo->transitionid = alignments.at(i);
        pFrameInfo->transition_index = i;
        vec_FrameInfo.push_back(pFrameInfo);
    }
    //将能量和帧对应起来
    if(vec_FrameInfo.size() ==  energy.size())
    {
        for(int i=0; i<frame_len; i++)
        {
            vec_FrameInfo.at(i)->energy = energy.at(i);
        }
    }

    //将基音频率和帧对应起来
    bool bGetPitch = GetPitchFromWav(api_name);
    if(!bGetPitch)
    {
        KALDI_ERR << "Get pitch from wav failed. ";
    }


    //根据对齐信息，得到音素对应的帧集合，这里加载final.mdl文件
    std::string model_in_filename = mdl_dir+"/final.mdl";
    TransitionModel trans_model;
    {
      bool binary;
      Input ki(model_in_filename, &binary);
      trans_model.Read(ki.Stream(), binary);
    }

    std::vector<std::vector<int32> > split;
    SplitToPhones(trans_model, alignments, &split);

    //将上面的帧集合和音素类对应起来
    int32 frame_index = 0;

    for(int32 i=0; i < split.size(); i++)
    {
        int32  front_transition_id = split.at(i).front();
        int32  phone_id = trans_model.TransitionIdToPhone(front_transition_id);

        bool  isNor = p_Speech_Public->CheckCurPhoneIsSil(phone_id);

        if(isNor)   //是正常音素
        {
                PhoneInfo* pPhone = new PhoneInfo();
                for(int32 j=0; j<split.at(i).size(); j++)
                {
                    pPhone->m_vecFrame.push_back(vec_FrameInfo.at(frame_index));
                    frame_index++;
                }

                std::string str_phone = p_Speech_Public->GetPhoneStrWithId(phone_id);

                pPhone->phone_id = phone_id;
                pPhone->strPhone = str_phone;

                vec_PhoneInfo.push_back(pPhone);

        }
        else
        {
                frame_index = frame_index + split.at(i).size();
        }
    }


    //音素数据归一化
    NormalizedPhoneData();

    //得到正常的音素集后，根据map，将音素集和单词进行匹配
    PhonesMatchWord(words);

    //音素和单词匹配后，对每个单词里面的音素集，进行分段处理（为了后期的音标重音）
    SplitPhonesWithWords();

    //调用训练参数类，得到单词的重音评分
    TrainParams* trainParm = new TrainParams();
    //trainParm->CalculateStressScoreWithWordInfo(vec_WordInfo);
    trainParm->CalculateWordInfo(vec_WordInfo);

    if(trainParm){
        delete trainParm;
        trainParm = NULL;
    }
}

void  SpeechStress::PhonesMatchWord(std::vector<int32>& vec_word)
{
    int32 phone_index = 0;
    for(int32 i=0; i<vec_word.size(); i++)
    {
        int32 phone_count = CountPhoneWithWord(vec_word.at(i), phone_index);

        WordInfo*  new_word = new WordInfo();

        for(int32 j=0; j<phone_count; j++)
        {
            new_word->m_vecPhones.push_back(vec_PhoneInfo.at(phone_index+j));
        }

        new_word->word_id = vec_word.at(i);
        new_word->str_word_name = p_Speech_Public->GetWordStrWithId(vec_word.at(i));

        vec_WordInfo.push_back(new_word);

        phone_index = phone_index + phone_count;
    }
}

void  SpeechStress::NormalizedPhoneData()
{
    KALDI_ASSERT(vec_PhoneInfo.size() > 0);

    BaseFloat max = 1; //归一化数据范围
    BaseFloat min = 0;

    std::vector<BaseFloat> features_dis; //临时特征向量
    std::vector<BaseFloat> features_energy;
    std::vector<BaseFloat> features_pitch;

    for(int32 i=0; i<vec_PhoneInfo.size(); i++)
    {
        BaseFloat   dis = 0,energy=0,pitch=0;
        for(int32 j=0;j<vec_PhoneInfo.at(i)->m_vecFrame.size(); j++)
        {
            dis = dis + vec_PhoneInfo.at(i)->m_vecFrame.at(j)->distance;
            energy = energy + vec_PhoneInfo.at(i)->m_vecFrame.at(j)->energy;
            pitch = pitch + vec_PhoneInfo.at(i)->m_vecFrame.at(j)->pitch;
        }

        vec_PhoneInfo.at(i)->phone_distance = dis;
        vec_PhoneInfo.at(i)->phone_energy = energy;
        vec_PhoneInfo.at(i)->phone_pitch = pitch;

        features_dis.push_back(dis);
        features_energy.push_back(energy);
        features_pitch.push_back(pitch);
    }

    //特征归一化
    BaseFloat MaxDisValue = *max_element(features_dis.begin(),features_dis.end());  //求时长最大值
    BaseFloat MinDisValue = *min_element(features_dis.begin(),features_dis.end());  //求时长最小值

    BaseFloat MaxEnergyValue = *max_element(features_energy.begin(),features_energy.end());
    BaseFloat MinEnergyValue = *min_element(features_energy.begin(),features_energy.end());

    BaseFloat MaxPitchValue = *max_element(features_pitch.begin(),features_pitch.end());
    BaseFloat MinPitchValue = *min_element(features_pitch.begin(),features_pitch.end());

    for(int32 i=0; i<vec_PhoneInfo.size(); i++)
    {
        vec_PhoneInfo.at(i)->phone_distance = (vec_PhoneInfo.at(i)->phone_distance - MinDisValue)/(MaxDisValue - MinDisValue);

        vec_PhoneInfo.at(i)->phone_energy = (vec_PhoneInfo.at(i)->phone_energy - MinEnergyValue)/(MaxEnergyValue - MinEnergyValue);

        vec_PhoneInfo.at(i)->phone_pitch = (vec_PhoneInfo.at(i)->phone_pitch - MinPitchValue)/(MaxPitchValue - MinPitchValue);

    }

}

bool  SpeechStress::GetPitchFromWav(std::string api_name)
{
    //提取基音频率
    if(vec_FrameInfo.size() <=0)
    {
        KALDI_ERR << "No alignment information in SpeechStress: ";
        return false;
    }

    //读取wav.scp文件，计算基音频率
    try {
    const char *usage =
        "Apply Kaldi pitch extractor, starting from wav input.  Output is 2-dimensional\n"
        "features consisting of (NCCF, pitch in Hz), where NCCF is between -1 and 1, and\n"
        "higher for voiced frames.  You will typically pipe this into\n"
        "process-kaldi-pitch-feats.\n"
        "Usage: compute-kaldi-pitch-feats [options...] <wav-rspecifier> <feats-wspecifier>\n"
        "e.g.\n"
        "compute-kaldi-pitch-feats --sample-frequency=8000 scp:wav.scp ark:- \n"
        "\n"
        "See also: process-kaldi-pitch-feats, compute-and-process-kaldi-pitch-feats\n";

        ParseOptions po(usage);
        PitchExtractionOptions pitch_opts;
        int32 channel = -1; // Note: this isn't configurable because it's not a very
                            // good idea to control it this way: better to extract the
                            // on the command line (in the .scp file) using sox or
                            // similar.
        pitch_opts.Register(&po);

        std::string wav_rspecifier = std::string("scp:"+api_name+"wav.scp");//po.GetArg(1)

        SequentialTableReader<WaveHolder> wav_reader(wav_rspecifier);

        int32 num_done = 0, num_err = 0;
        for (; !wav_reader.Done(); wav_reader.Next()) {
          std::string utt = wav_reader.Key();
          const WaveData &wave_data = wav_reader.Value();

          int32 num_chan = wave_data.Data().NumRows(), this_chan = channel;
          {
            KALDI_ASSERT(num_chan > 0);
            // reading code if no channels.
            if (channel == -1) {
              this_chan = 0;
              if (num_chan != 1)
                KALDI_WARN << "Channel not specified but you have data with "
                           << num_chan  << " channels; defaulting to zero";
            } else {
              if (this_chan >= num_chan) {
                KALDI_WARN << "File with id " << utt << " has "
                           << num_chan << " channels but you specified channel "
                           << channel << ", producing no output.";
                continue;
              }
            }
          }

          if (pitch_opts.samp_freq != wave_data.SampFreq())
            KALDI_ERR << "Sample frequency mismatch: you specified "
                      << pitch_opts.samp_freq << " but data has "
                      << wave_data.SampFreq() << " (use --sample-frequency "
                      << "option).  Utterance is " << utt;


          SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);
          Matrix<BaseFloat> features;
          try {
            ComputeKaldiPitch(pitch_opts, waveform, &features);
          } catch (...) {
            KALDI_WARN << "Failed to compute pitch for utterance "
                       << utt;
            num_err++;
            continue;
          }
          //这里根据得到的矩阵取出对应的基因频率信息

          if(vec_FrameInfo.size() == features.NumRows())
          {
                for(int i=0; i< features.NumRows(); i++)
                {
                    BaseFloat  framePitch = (*((features.Data() + i*features.Stride()) +1 ));

                    vec_FrameInfo.at(i)->pitch = framePitch;
                }
          }

          //feat_writer.Write(utt, features);
          if (num_done % 50 == 0 && num_done != 0)
            KALDI_VLOG(2) << "Processed " << num_done << " utterances";
          num_done++;
        }
        //KALDI_LOG << "Done " << num_done << " utterances, " << num_err
        //          << " with errors.";
        return true;
      } catch(const std::exception &e) {
        std::cerr << e.what();
        return false;
      }
}

void  SpeechStress::SplitPhonesWithWords()
{
    for(int32 i=0; i<vec_WordInfo.size();i++)
    {
        //得到元音音素的个数
        std::vector<int32>  vec_vowel_phone_index;

        for(int32 j=0; j<vec_WordInfo.at(i)->m_vecPhones.size(); j++)
        {
            if(CheckPhoneIsVowel(vec_WordInfo.at(i)->m_vecPhones.at(j)->phone_id))
            {
                vec_vowel_phone_index.push_back(j);
            }
        }

        if(vec_vowel_phone_index.size() <= 1)
        {
                std::vector<PhoneInfo*>   vec_split_phones;
                for(int32 j=0; j<vec_WordInfo.at(i)->m_vecPhones.size(); j++)
                {
                    vec_split_phones.push_back(vec_WordInfo.at(i)->m_vecPhones.at(j));
                }
                vec_WordInfo.at(i)->m_vecSplitPhones.push_back(vec_split_phones);
        }
        else
        {
                //通过计算得到分离出音素的索引
                std::vector<int32>  vec_split_index;

                for(int32 j=0; j<vec_vowel_phone_index.size(); j++)
                {

                   if((j+1) < vec_vowel_phone_index.size())
                   {
                         //判断每两个元音之间的辅音个数
                         int32 consonant_count = vec_vowel_phone_index.at(j+1) - vec_vowel_phone_index.at(j);

                         if(consonant_count == 1)
                                vec_split_index.push_back(vec_vowel_phone_index.at(j));
                         else   if(consonant_count == 2)
                                vec_split_index.push_back(vec_vowel_phone_index.at(j));
                         else if(consonant_count == 3)
                                vec_split_index.push_back(vec_vowel_phone_index.at(j) + 1);
                         else if(consonant_count == 4)
                                vec_split_index.push_back(vec_vowel_phone_index.at(j) + 1);
                         else if(consonant_count == 5)
                                vec_split_index.push_back(vec_vowel_phone_index.at(j) + 2);
                         else
                                KALDI_VLOG(2) << "两个元音音素中间的辅音音素个数大于4个.";
                   }
                }

                //通过得到分段节点位置对音素集进行分段
                int32  phone_start_index =0;
                for(int32 j=0; j<vec_split_index.size(); j++)
                {
                        std::vector<PhoneInfo*>   vec_split_phones;
                        for(;phone_start_index<=vec_split_index.at(j); phone_start_index++)
                        {
                            vec_split_phones.push_back(vec_WordInfo.at(i)->m_vecPhones.at(phone_start_index));
                        }

                        vec_WordInfo.at(i)->m_vecSplitPhones.push_back(vec_split_phones);
                        phone_start_index++;
                }
        }
    }
}

bool  SpeechStress::CheckPhoneIsVowel(int32 phone_id)
{
    std::string str_phone = p_Speech_Public->GetPhoneStrWithId(phone_id);
    if(!str_phone.empty())
        if((str_phone[0] == 'A') || (str_phone[0] == 'E') || (str_phone[0] == 'I') || (str_phone[0] == 'O') || (str_phone[0] == 'U'))
            return true;
    return false;
}

BaseFloat  SpeechStress::CalculateSentenceStressScore(std::vector<std::vector<std::string> >&  forceAli,
                                                                          std::vector<int32>& word_stress_indexs,
                                                                          std::vector<std::string>&  m_word_str)
{
    //计算句子重音评分
    //重音评分是计算每一项得平均值
    //从语句解析类里可以得到单词的合并分段  和具体的重音位置
    //根据分段信息，得到匹配个数，进而得到匹配率（重音评分）

    std::vector<BaseFloat>   vec_stress_score;
    std::vector<BaseFloat>   vecSelScore;

    int32 word_index = 0;
    for(int32 i=0;i<forceAli.size(); i++)
    {
        BaseFloat  sum_score = 0;
        for(int32 j=0; j<forceAli.at(i).size(); j++)
        {
            if(forceAli.at(i).at(j).compare(" ") !=0 )   //判断当前单词不为空
            {
                sum_score = sum_score + vec_WordInfo.at(word_index)->stress_score;
                word_index ++;
            }
        }

        BaseFloat  averageScore = sum_score/forceAli.at(i).size();

        vec_stress_score.push_back(averageScore);
        vecSelScore.push_back(averageScore);
    }
    BaseFloat  compare_count = word_stress_indexs.size();

    sort(vecSelScore.begin(), vecSelScore.end(),std::greater<int32>());//降序

    std::vector<int32>  stress_indexs;

    for(int32 i=0; i<(compare_count); i++)
    {
        int32  word_index = -1;

        if(i >= vecSelScore.size())
            break;

        for(int32 j=0; j<vec_stress_score.size(); j++)
        {
                if(vec_stress_score.at(j) == vecSelScore.at(i))
                {
                    stress_indexs.push_back(j);
                    break;
                }
        }
    }

    //这里分数通过单词的索引来判断重音标记的位置

    std::vector<int32>  aaaa;

    //if(stress_indexs.size() != compare_count )
    //    return -1;

    BaseFloat success_count = 0.0;
    for(int32 i=0; i<compare_count; i++)
    {
        for(int32 j=0;j<stress_indexs.size(); j++)
        {
            if(stress_indexs.at(j) == word_stress_indexs.at(i))
            {
                success_count = success_count + 1;
                aaaa.push_back(word_stress_indexs.at(i));
                break;
            }
        }
    }

    for(int32 i=0; i<aaaa.size(); i++)
    {
         Json::Value data;

        data["index"] = aaaa.at(i);
        data["word"] = m_word_str.at(aaaa.at(i));
        data["stressScore"] = vec_stress_score.at(aaaa.at(i)) / score_max;
        std::cout<<"重音成功匹配单词位置："<<aaaa.at(i)<<std::endl;
        std::cout<<"重音成功匹配单词："<<m_word_str.at(aaaa.at(i))<<std::endl;

        Stress_Json.append(data);
    }

    if( compare_count == 0 )
        return 0.0;

    BaseFloat  sentenceWordScore = 0.00;
    sentenceWordScore = success_count/compare_count;

    return  sentenceWordScore;
}

BaseFloat  SpeechStress::CalculateWordsStressScore()
{
    //计算分段索引
    if(vec_WordInfo.size() > 0)
    {
        for(int i=0; i<vec_WordInfo.size(); i++)
        {
            //计算每个单词重音所在的分段索引
            CalculateWordSplitIndex(vec_WordInfo.at(i));
        }
    }

    //开始计算重音评分
    //主要思路：通过判断重音音节和表中给出的重音音节的索引是否相同的个数
    //除以 单词的个数作为单词级重音评分的标准
    BaseFloat word_stress_succeed = 0;

    for(int i=0; i<vec_WordInfo.size(); i++)
    {
        //首先计算音节数是否相同
        int syllable_count = p_Speech_Public->GetTotalSyllable(vec_WordInfo.at(i)->str_word_name);
        int stress_index = p_Speech_Public->GetStressSyllable(vec_WordInfo.at(i)->str_word_name);

        //std::cout<<"重音表中"<<vec_WordInfo.at(i)->str_word_name<<"重音索引是："<<stress_index<<std::endl;

        if((vec_WordInfo.at(i)->vowelPhones_count == syllable_count) && (vec_WordInfo.at(i)->stress_split_index == stress_index))
            word_stress_succeed = word_stress_succeed + 1;

        //如果在词表中找不到单词，默认为查找成功
        //if((syllable_count == (-1)) && (stress_index == (-1)))
        //    word_stress_succeed = word_stress_succeed + 1;
    }

    BaseFloat  WordsScore = 0.00;

    std::cout<<"成功匹配的个数是:"<<word_stress_succeed<<std::endl;
    std::cout<<"总的单词个数是:"<<vec_WordInfo.size()<<std::endl;

    BaseFloat totalSize = vec_WordInfo.size();
    WordsScore = word_stress_succeed / totalSize;

    return WordsScore;
}

void  SpeechStress::CalculateWordSplitIndex(WordInfo*& wordInfo)
{
    if(p_Speech_Public == NULL)
        return;

    std::vector<PhoneInfo*>  vowel_Phones;

    //std::vector<FrameInfo*>  vec_Frames;
    //std;:vector<PhoneInfo*>  vec_vowelPhones;

    //找出单词中的所有元音音素
    if(wordInfo)
    {
        for(int i=0; i<wordInfo->m_vecPhones.size(); i++)
        {
            if(p_Speech_Public->CheckCurPhoneIsVowel(wordInfo->m_vecPhones.at(i)->phone_id))  //检测到当前的音素是元音音素
            {
                vowel_Phones.push_back(wordInfo->m_vecPhones.at(i));
            }
        }
    }

    wordInfo->vowelPhones_count = vowel_Phones.size();  //

    //根据找到的元音音素，计算哪个元音分数大，当作是重音部分
    int stress_index = -1;
    float start_stress_score = 0;

    for(int i=0; i<vowel_Phones.size(); i++)
    {
        float cur_score = 3*vowel_Phones.at(i)->phone_distance + 2*vowel_Phones.at(i)->phone_energy + 1*vowel_Phones.at(i)->phone_pitch;
        if(cur_score > start_stress_score)
        {
            stress_index = i;
            start_stress_score = cur_score;
        }
    }

    wordInfo->stress_split_index = stress_index;

}

BaseFloat  SpeechStress::New_CalculateSentenceStressScore(std::vector<std::vector<std::string> >&  forceAli,
                                                                          std::vector<int32>& word_stress_indexs,
                                                                          std::vector<std::string>&  m_word_str)
{
    //计算句子重音评分
    //思路：计算句子中的前N个时长 前N个能量  前N个基音频率
    //将三个集合取交集，得到的单词就是重音的单词，

    std::vector<BaseFloat>   vec_stress_score;
    std::vector<BaseFloat>   vecSelScore;

    //下面三个容器保存所有单词的 时长 能量 基音频率
    std::vector<BaseFloat>   vecWord_Distance;
    std::vector<BaseFloat>   vecWord_Energy;
    std::vector<BaseFloat>   vecWord_Pitch;

    int32 word_index = 0;
    for(int32 i=0;i<forceAli.size(); i++)
    {
        BaseFloat  sum_distance = 0;
        BaseFloat  sum_energy = 0;
        BaseFloat  sum_pitch = 0;

        //因为这里涉及到分词，可能把两三个单词当一个单词，所以要计算整体的平均值，当成整个单词的分数
        for(int32 j=0; j<forceAli.at(i).size(); j++)
        {
            if(forceAli.at(i).at(j).compare(" ") !=0 )   //判断当前单词不为空
            {

                sum_distance = sum_distance + vec_WordInfo.at(word_index)->word_distance;
                sum_energy = sum_energy + vec_WordInfo.at(word_index)->word_energy;
                sum_pitch = sum_pitch + vec_WordInfo.at(word_index)->word_pitch;

                word_index ++;
            }
        }

        BaseFloat  result_distance = 0;
        BaseFloat  result_energy = 0;
        BaseFloat  result_pitch = 0;

        if(forceAli.at(i).size() > 0)
        {
           result_distance = sum_distance / forceAli.at(i).size();
           result_energy = sum_energy / forceAli.at(i).size();
           result_pitch = sum_pitch / forceAli.at(i).size();
        }

        vecWord_Distance.push_back(result_distance);
        vecWord_Energy.push_back(result_energy);
        vecWord_Pitch.push_back(result_pitch);
    }

    //统计完所有单词的 三个指标的分数后，现在要取出各个指标的前N名
    int  compare_count = word_stress_indexs.size();

    std::vector<int> vec_analyse_stress;   //里面保存的是通过时长 能量  基音频率  三个特征分析出来的重音单词位置
    GetStressIndexFromSentence(vecWord_Distance, vecWord_Energy, vecWord_Pitch, compare_count, vec_analyse_stress);

    //下面计算匹配成功的个数
    BaseFloat success_count = 0.0;
    for(int32 i=0; i<compare_count; i++)
    {
        for(int32 j=0;j<vec_analyse_stress.size(); j++)
        {
            if(vec_analyse_stress.at(j) == word_stress_indexs.at(i))
            {
                success_count = success_count + 1;
                //aaaa.push_back(word_stress_indexs.at(i));
                break;
            }
        }
    }

    if( compare_count == 0 )
        return 0.0;

    std::cout<<"句子重音成功个数是："<<success_count<<std::endl;
    std::cout<<"句子重音比较个数是："<<compare_count<<std::endl;

    BaseFloat  sentenceWordScore = 0.00;
    sentenceWordScore = success_count/compare_count;

    return  sentenceWordScore;
}

void SpeechStress::GetStressIndexFromSentence(std::vector<BaseFloat>& vec_distance, 
                                  std::vector<BaseFloat>& vec_energy,
                                  std::vector<BaseFloat>& vec_pitch,
                                  int nCount_compare,
                                  std::vector<int>& vec_result_stress)
{
    //函数 思路， 找出前三个容器中的最大的前N项，将对应的集合做交集后，交集的结果就是重音的结果
    //时长
    std::vector<int> v_distance;

    for(int i=0; i<nCount_compare; i++)
    {
        std::vector<BaseFloat>::iterator biggest = std::max_element(std::begin(vec_distance), std::end(vec_distance));
        int biggest_index = std::distance(std::begin(vec_distance), biggest);
        v_distance.push_back(biggest_index);

        if(biggest != vec_distance.end())
            *biggest = 0;
            //biggest->second = 0;
            //vec_distance.erase(biggest);
    }

    //能量
    std::vector<int> v_energy;
    for(int i=0; i<nCount_compare; i++)
    {
        std::vector<BaseFloat>::iterator biggest = std::max_element(std::begin(vec_energy), std::end(vec_energy));
        int biggest_index = std::distance(std::begin(vec_energy), biggest);
        v_energy.push_back(biggest_index);

        if(biggest != vec_energy.end())
            *biggest = 0;
            //biggest->second = 0;
            //vec_energy.erase(biggest);
    }

    //基音频率
    std::vector<int> v_pitch;
    for(int i=0; i<nCount_compare; i++)
    {
        std::vector<BaseFloat>::iterator biggest = std::max_element(std::begin(vec_pitch), std::end(vec_pitch));
        int biggest_index = std::distance(std::begin(vec_pitch), biggest);
        v_pitch.push_back(biggest_index);

        if(biggest != vec_pitch.end())
            *biggest = 0;
            //biggest->second = 0;
            //vec_pitch.erase(biggest);
    }

    //找出三个容器中的并集
    std::vector<int> find_one;
    std::sort(v_distance.begin(), v_distance.end());
    std::sort(v_energy.begin(), v_energy.end());
    std::set_union(v_distance.begin(), v_distance.end(), v_energy.begin(), v_energy.end(), back_inserter(find_one));

    
    std::vector<int> find_result;
    std::sort(find_one.begin(), find_one.end());
    std::sort(v_pitch.begin(), v_pitch.end());
    std::set_union(find_one.begin(), find_one.end(), v_pitch.begin(), v_pitch.end(), back_inserter(find_result));

    //将一个容器的值赋给另一个容器
    vec_result_stress.assign(find_result.begin(), find_result.end());

    sort(find_result.begin(), find_result.end());
    find_result.erase(unique(find_result.begin(), find_result.end()), find_result.end());
}

void  SpeechStress::Recovery()
{
   //收回vec_WordInfo空间
     for(std::vector <WordInfo*>::iterator s = vec_WordInfo.begin(),
                                                  e = vec_WordInfo.end(); s!=e; ++s)
     {
        delete *s;
        *s = NULL;
     }
     vec_WordInfo.clear();

    for(std::vector <PhoneInfo*>::iterator s = vec_PhoneInfo.begin(),
                                                  e = vec_PhoneInfo.end(); s!=e; ++s)
     {
        delete *s;
        *s = NULL;
     }
     vec_PhoneInfo.clear();

    for(std::vector <FrameInfo*>::iterator s = vec_FrameInfo.begin(),
                                                  e = vec_FrameInfo.end(); s!=e; ++s)
     {
        delete *s;
        *s = NULL;
     }
     vec_FrameInfo.clear();

     //for(std::vector <PhoneInfo*>::iterator s = vec_vowelPhones.begin(),
     //                                             e = vec_vowelPhones.end(); s!=e; ++s)
    // {
    //    delete *s;
    //    *s = NULL;
    // }
    // vec_vowelPhones.clear();

     Stress_Json = "";
}

/*
bool  SpeechStress::CheckLiaisonPossible()
{
    std::cout<<"output transition-ids："<<std::endl;

    for(int32 i=0; i<vec_FrameInfo.size(); i++)
    {
        std::cout<<vec_FrameInfo.at(i)->transitionid<< " ";
    }
    std::cout<<std::endl;

    //判断识别结果一句中的两两单词连读性
    int32 nWordsCount = vec_WordInfo.size();

    if( nWordsCount > 0)
    {
        for(int32 i=0; i<nWordsCount; i++)
        {
            if((i+1)<nWordsCount)
            {
                std::string  strEndWord = vec_WordInfo.at(i)->str_word_name;
                std::string  strStartWord = vec_WordInfo.at(i + 1)->str_word_name;

                std::string  strEndPhone = vec_WordInfo.at(i)->m_vecPhones.back()->strPhone;
                std::string  strStartPhone = vec_WordInfo.at(i + 1)->m_vecPhones.front()->strPhone;

                if(p_Speech_Public->JudgePossibilityOfLiaison(strEndWord, strStartWord ,strEndPhone, strStartPhone))
                {
                    int32 index = p_Speech_Public->GetLiaisonPhoneIndex(strEndPhone, strStartPhone);
                    std::string strIndex = std::to_string(index);
                    std::cout<<"find phone index is："<<strIndex<<std::endl;
                    bool bSuccess = GetTransitionIndexWithPhones( strEndWord, strStartWord,strIndex);
                    if(!bSuccess)
                        KALDI_WARN << "Failed to extract feature : " <<strEndWord << "   " <<strStartWord;
                }
            }
        }
    }

    if(p_DataExtract)
        p_DataExtract->SaveMatrixToTxtFile();

    return true;

}

bool  SpeechStress::GetTransitionIndexWithPhones(std::string& strEndWord, std::string& strStartWord, std::string& index)
{
    int32 one_start_index = -1;
    int32 one_end_index = -1;
    int32 two_start_index = -1;
    int32 two_end_index = -1;

    //std::string  word_one = "ONE";
    //std::string  word_two = "OF";

    for(int32 i=0; i<vec_WordInfo.size(); i++)
    {
        if(vec_WordInfo.at(i)->str_word_name.compare(strStartWord) == 0 )
        {
            if(vec_WordInfo.at(i - 1 )->str_word_name.compare(strEndWord) == 0 )
            {
                one_start_index = vec_WordInfo.at(i - 1)->m_vecPhones.back()->m_vecFrame.front()->transition_index;
                one_end_index = vec_WordInfo.at(i - 1)->m_vecPhones.back()->m_vecFrame.back()->transition_index;

                two_start_index = vec_WordInfo.at(i)->m_vecPhones.front()->m_vecFrame.front()->transition_index;
                two_end_index = vec_WordInfo.at(i)->m_vecPhones.front()->m_vecFrame.back()->transition_index;

                bool   b_rtn = p_DataExtract->MatrixExtractWithTransitionIndex(index, strEndWord, strStartWord, one_start_index, one_end_index, two_start_index, two_end_index);

                std::string front_phones = std::string("");
                std::string front_transitions = std::string("");
                for(int32 j=0; j<vec_WordInfo.at(i - 1)->m_vecPhones.size(); j++)
                    front_phones = front_phones + std::string(" ") + vec_WordInfo.at(i - 1)->m_vecPhones.at(j)->strPhone;
                for(int32 j =0; j<vec_WordInfo.at(i - 1)->m_vecPhones.back()->m_vecFrame.size(); j++)
                    front_transitions = front_transitions + std::string(" ") + std::to_string(vec_WordInfo.at(i - 1)->m_vecPhones.back()->m_vecFrame.at(j)->transitionid);
                std::cout<<"front   word name："<<vec_WordInfo.at(i - 1)->str_word_name<<std::endl;
                std::cout<<"front   phones："<<front_phones<<std::endl;
                std::cout<<"front   transition-ids："<<front_transitions<<std::endl;

                std::string next_phones = std::string("");
                std::string next_transitions = std::string("");
                for(int32 j=0; j<vec_WordInfo.at(i)->m_vecPhones.size(); j++)
                    next_phones = next_phones + std::string(" ") + vec_WordInfo.at(i)->m_vecPhones.at(j)->strPhone;
                for(int32 j =0; j<vec_WordInfo.at(i)->m_vecPhones.front()->m_vecFrame.size(); j++)
                    next_transitions = next_transitions + std::string(" ") + std::to_string(vec_WordInfo.at(i)->m_vecPhones.front()->m_vecFrame.at(j)->transitionid);
                std::cout<<"next   word name："<<vec_WordInfo.at(i)->str_word_name<<std::endl;
                std::cout<<"next   phones："<<next_phones<<std::endl;
                std::cout<<"next   transition-ids："<<next_transitions<<std::endl;

                return b_rtn;
            }
        }
    }

    return true;
}
*/

}  // End namespace kaldi
