// fluency/score.cc

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0


#include "fluency/score.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <sstream>
#include <boost/timer.hpp>
using namespace boost;

namespace fluency {

namespace spd = spdlog;
using namespace Json;
using namespace kaldi;

typedef typename fst::StdArc Arc;
typedef typename Arc::StateId StateId;
typedef typename Arc::Weight Weight;

bool del_file(const char* filename)
{
    //根据文件名删除文件
    if(filename == NULL)
        return false;

    if(remove(filename)==0)
        return true;
    return false;
}

float Random(int start, int end){
    float dis = end - start;
    //printf("dis: %d\n", dis);
    return start + dis * (rand() / (RAND_MAX + 1.0));
}

Score::Score(std::string api_name)
{
    str_Json = "";
    str_DetailJson = "";
    api_name_ = api_name;

    //初始化读取模型相关路径
    timer t;
	ConfigParser config("config.ini");
	//读取fluency用到的模型路径和相关语言文件路径
	config.get("FluencyMoudle",mdl_dir_name_);
	config.get("FluencyLang",lang_dir_name_);
	config.get("FeatureConfig",feature_dir_name_);

    m_pMfcc = new MfccFeature();
    m_pSpeechPublic = new SpeechPublic(lang_dir_name_);
    m_pChainDecode = new ChainDecode(mdl_dir_name_,lang_dir_name_,feature_dir_name_);
    m_pChainDecode->Init();
    cout << "score init use time:" << t.elapsed()<< "s" << endl;
}

Score::~Score()
{
    if(m_pMfcc)
    {
        delete m_pMfcc;
        m_pMfcc = NULL;
    }

   if(m_pSpeechPublic)
    {
        delete m_pSpeechPublic;
        m_pSpeechPublic = NULL;
    }

    if(m_pChainDecode)
    {
        delete m_pChainDecode;
        m_pChainDecode = NULL;
    }
}

void  Score::InitLogger()
{
    if(!fluency_logger_){
       fluency_logger_ = spdlog::daily_logger_mt("fluency_logger", "logs/"+api_name_+"gop.txt", 1, 30);
       fluency_logger_->flush_on(spd::level::info);
    }

    ConfigParser config("config.ini");
    config.get("loglevel",fluency_log_level_);

    //设置全局日志输出级别
    if(fluency_log_level_ == 2) fluency_logger_->set_level(spd::level::info);
    else if (fluency_log_level_ == 1)fluency_logger_->set_level(spd::level::debug);
    else if (fluency_log_level_ == 4)fluency_logger_->set_level(spd::level::err);
}

bool  Score::GetSpeechFluencyJson(string  str_wav_file_name, string  str_compare, string &result, int cmp_type, bool  bIsDetail)
{
    cmp_type_ = cmp_type;
    if(CalculateFluencyScore(str_wav_file_name, str_compare))
    {
        if(bIsDetail)
            result = str_DetailJson;
        else
            result = str_Json;
        return true;
    }
    return false;
}

std::string   Score::GetDecodeWords()
{
    std::string strWordsRtn = std::string("");
    if(m_pChainDecode->word_str.size() > 0)
    {
        for(int32 i=0; i<m_pChainDecode->word_str.size(); i++)
        {
            strWordsRtn = strWordsRtn + m_pChainDecode->word_str.at(i) + std::string(" ");
        }
    }
    return strWordsRtn;
}

bool  Score::CalculateFluencyScore(string& str_wav_file_name, std::string str_compare)
{

    std::cout << "Score::CalculateFluencyScore" << std::endl;
    m_pChainDecode->Recovery();
    m_pMfcc->Recovery();

    //InitLogger();

    std::string strLog = std::string("");

    //特殊处理原wav文件打开失败问题
    if(str_wav_file_name == "" || access(str_wav_file_name.c_str(), F_OK) != 0)
    {
         MakeErrorJson();
         return true;
    }

    //创建wav.scp文件（使用文件流创建）
    CreateWavScpFile(str_wav_file_name);

    //特征提取   用于模型读取   分类
    if(m_pMfcc)
    {
        bool bMfcc = m_pMfcc->ComputeMfccFeature();
        if(!bMfcc)
        {
            KALDI_ERR << "MfccFeature failed. ";
            //fluency_logger_->error("MfccFeature failed.");
            MakeErrorJson();
            return true;
        }
    }

    //chain模型解码
    if(m_pChainDecode)
    {
        m_pChainDecode->SetWavId(str_wav_file_name);
        bool bChainDecode = m_pChainDecode->FasterDecode(api_name_);
        if(!bChainDecode)
        {
            KALDI_ERR << "ChainDecode failed. ";
            //fluency_logger_->error("ChainDecode failed.");
            MakeErrorJson();
            return true;
        }
    }

    //拼接日志
    string str="";
    for(auto s:m_pChainDecode->word_str)
        str += s+" ";
    strLog = std::string("Chain model decoding results: ")+str;
    //fluency_logger_->info(strLog);
    std::cout << "chain解码结果: " << str << std::endl;
    //如果没解码出单词直接返回错误json串
    if(m_pChainDecode->word_str.size() == 0)
    {
        MakeErrorJson();
        return true;
    }


    SentenceParse* pSentenceParse = new SentenceParse(m_pSpeechPublic, m_pChainDecode->word_str, str_compare);

    //计算词级分数
    WordScore ws;
    BaseFloat wordScore=-1,dis=0;
    dis = ws.EditDistance(m_pChainDecode->word_str,pSentenceParse->m_vec_word_str);

    if(pSentenceParse->m_vec_word_str.size()>0 && (m_pChainDecode->word_str.size() - dis)>0 )
        wordScore = (m_pChainDecode->word_str.size() - dis)/pSentenceParse->m_vec_word_str.size();
    wordScore *=100;
    if(wordScore<0 || wordScore>100) wordScore = -1;

    if (cmp_type_ == 2){
        //基本分数的json
        if(!DeleteTempFile()) //打印日志
        pSentenceParse->Recovery();
        delete pSentenceParse;
        pSentenceParse = NULL;
        MakeScoreJson(wordScore, -1, -1, -1);
        return true;
    }


    //停顿类计算评分
    std::cout<<"停顿评分开始" <<std::endl;

    BaseFloat fPauseScore = -1;
    Json::Value  pauseJson;

    if((m_pMfcc->m_vecEnergy.size() > 0) && (pSentenceParse->m_vecWord_pause_index.size()>0))
    {
        SpeechPause* m_pPause = new SpeechPause(m_pSpeechPublic);
        //std::shared_ptr<SpeechPause> m_pPause(new SpeechPause(m_pSpeechPublic));
        m_pPause->InitSpeechPause(mdl_dir_name_,m_pChainDecode->alignments);

        //fPauseScore = m_pPause->CalculatePauseScore(m_pChainDecode->words,
          //                                          m_pChainDecode->word_str,
            //                                        pSentenceParse->vec_lcs_ali,
              //                                      pSentenceParse->m_vecWord_pause_index);

        fPauseScore = m_pPause->New_CalculatePauseScore(m_pChainDecode->words,
                                                    m_pChainDecode->word_str,
                                                    pSentenceParse->vec_lcs_ali,
                                                    pSentenceParse->m_vecWord_notpossible_pause_index);

        std::cout<<"停顿分数是: "<<fPauseScore<<std::endl;
        if(fPauseScore <= 0.2)
        {
            fPauseScore = fPauseScore + 0.4;
            fPauseScore = fPauseScore * 100;
            fPauseScore = Random((fPauseScore-5), (fPauseScore +5));
            std::cout<<"停顿分数是: "<<fPauseScore<<std::endl;
        }
        else
            fPauseScore = fPauseScore * 100;

        pauseJson = m_pPause->GetPauseInfoJson();

        m_pPause->Recovery();
        if(m_pPause){
            delete m_pPause;
            m_pPause = NULL;
        }
    }

    std::cout<<"停顿评分结束" <<std::endl;

   //重音类计算评分
   BaseFloat fStressScore = -1;
   BaseFloat fWordsStressScore = -1;
   Json::Value   stressJson;

   std::cout<<"重音评分开始" <<std::endl;

   if((m_pMfcc->m_vecEnergy.size() > 0) && (pSentenceParse->m_vecWord_stress_indexs.size()>0))
   {
        SpeechStress *m_pStress = new SpeechStress(m_pSpeechPublic);

        std::cout<<"重音类对象初始化完成,开始初始化重音数据!!!!" <<std::endl;

        m_pStress->InitSpeechStress(mdl_dir_name_,m_pChainDecode->alignments, m_pChainDecode->words, m_pMfcc->m_vecEnergy,api_name_);

        std::cout<<"开始计算重音评分!!!!" <<std::endl;

        //fStressScore = m_pStress->CalculateSentenceStressScore(pSentenceParse->vec_lcs_ali,
          //                                             pSentenceParse->m_vecWord_stress_indexs,
            //                                           pSentenceParse->m_vec_word_str);

        fStressScore = m_pStress->New_CalculateSentenceStressScore(pSentenceParse->vec_lcs_ali,
                                                       pSentenceParse->m_vecWord_stress_indexs,
                                                       pSentenceParse->m_vec_word_str);

        std::cout << "fStressScore: " << fStressScore << std::endl;
        if(fStressScore <= 0.4)
        {
            fStressScore = fStressScore + 0.2;
            fStressScore = fStressScore * 100;
            fStressScore = Random((fStressScore-10), (fStressScore +5));
            fStressScore = fStressScore / 100;
        }
        fStressScore *= 100;

        //单词级重音分数计算
        fWordsStressScore = m_pStress->CalculateWordsStressScore();
        if(fWordsStressScore <= 0.4)
        {
            fWordsStressScore = fWordsStressScore + 0.2;
            fWordsStressScore = fWordsStressScore * 100;
            fWordsStressScore = Random((fWordsStressScore-10), (fWordsStressScore +5));
            fWordsStressScore = fWordsStressScore / 100;
        }

        fWordsStressScore *=100;

        stressJson = m_pStress->GetSpeechStressJson();
        //m_pStress->CheckLiaisonPossible();
        m_pStress->Recovery();
        if(m_pStress){
            delete m_pStress;
            m_pStress = NULL;
        }
   }

    std::cout<<"重音评分结束" <<std::endl;

    //基本分数的json
    MakeScoreJson(wordScore, fPauseScore, fStressScore, fWordsStressScore);

    //详细分数的json
    MakeDetailScoreJson(pauseJson, fPauseScore, stressJson, fStressScore, wordScore);

    //log打印
    strLog = std::string("重音连读json：")+str_Json;
    //fluency_logger_->info(strLog);

    strLog = std::string("重音连读详细json：")+str_DetailJson;
    //fluency_logger_->info(strLog);

    //删除产生的临时文件


    if(!DeleteTempFile()) //打印日志
    pSentenceParse->Recovery();
    delete pSentenceParse;
    pSentenceParse = NULL;

    return true;
}

void  Score::MakeScoreJson(BaseFloat wordScore, BaseFloat pauseScore, BaseFloat sentenceStressScore, BaseFloat wordsStressScore)
{
    //计算综合评分
    Json::Value data;
    Json::Value frame;
    Json::FastWriter fastWriter;

    frame["wordScore"] = wordScore;
    frame["pauseScore"] = pauseScore;
    frame["SentenceStressScore"] = sentenceStressScore;
    frame["WordsStressScore"] = wordsStressScore;

    str_Json = fastWriter.write(frame);    //简便信息
}

void  Score::MakeDetailScoreJson(Json::Value& pauseJson, BaseFloat pauseScore, Json::Value& stressJson, BaseFloat sentenceStressScore, BaseFloat wordScore)
{
    Json::FastWriter fastWriter;

    Json::Value pauseInfo;
    if(!pauseJson.isNull())
        pauseInfo["pauseInfo"] = pauseJson;
    pauseInfo["pauseScore"] = pauseScore;

    Json::Value stressInfo;

    if(!stressJson.isNull())
        stressInfo["stressInfo"] = stressJson;
    stressInfo["SentenceStressScore"] = sentenceStressScore;

    Json::Value   frameDetail;

    frameDetail["Pause"] = pauseInfo;
    frameDetail["Stress"] = stressInfo;
    frameDetail["wordScore"] = wordScore;
    str_DetailJson = fastWriter.write(frameDetail);
}

void  Score::CreateWavScpFile(string&  str_wav_path)
{
        std::ofstream location_out;

        location_out.open((api_name_+"wav.scp").c_str(),  std::ios::out | std::ios::trunc );  //以写入和在文件末尾添加的方式打开.txt文件，没有的话就创建该文件。
        if (!location_out.is_open())
            return ;

        location_out << "fulency_wav_id" << " " << str_wav_path <<std::endl;
        location_out.close();
}

void  Score::MakeErrorJson()
{
    Json::Value frame;
    Json::FastWriter fastWriter;
    frame["wordScore"] = -1;
    frame["pauseScore"] = -1;
    frame["SentenceStressScore"] = -1;
    frame["WordsStressScore"] = -1;
    str_Json = fastWriter.write(frame);    //简便信息

    Json::Value pauseInfo;
    pauseInfo["pauseScore"] = -1;
    Json::Value stressInfo;
    stressInfo["stressScore"] = -1;
    Json::Value SentenceStressInfo;
    SentenceStressInfo["SentenceStressScore"] = -1;
    Json::Value WordsStressInfo;
    WordsStressInfo["WordsStressScore"] = -1;

    Json::Value   frameDetail;
    frameDetail["Pause"] = pauseInfo;
    frameDetail["Stress"] = SentenceStressInfo;
    frameDetail["WordScore"] = -1;
    str_DetailJson = fastWriter.write(frameDetail);
}

bool Score::DeleteTempFile()
{
        try{
		remove((api_name_+"addDeltasFinal.ark").c_str());
		remove((api_name_+"wav.scp").c_str());
		remove((api_name_+"lattice").c_str());
        }
        catch (...){
			KALDI_WARN << "Failed to delete file " << endl;
			return false;
		}
		return true;
}

}  // End namespace kaldi
