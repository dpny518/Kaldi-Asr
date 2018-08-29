
#include <fstream>
#include "PhonesGop/compute-gop.h"
#include "common/config/config-parser.hpp"
#include "compute-center.h"
#include <boost/timer.hpp>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/timer.hpp>
#include <boost/algorithm/string.hpp>
#include <thread>

using namespace boost;

namespace compute{
    namespace spd = spdlog;

    ComputeCenter::ComputeCenter()
    {
        app_compute_gop_ = std::shared_ptr<ComputeGop>(new ComputeGop());
        app_fluency_score_ = std::shared_ptr<Score>(new Score());

        speech_compute_gop_ = std::shared_ptr<ComputeGop>(new ComputeGop());
    }

	//compute min app
	bool ComputeCenter::ComputeScore(const string &wav_file , const string &wav_text,int type)
	{
	    //cmp_type_ 控制小程序和实用英语 不同加分权重比例
	    cmp_type_ = type;
	    app_gop_jsonstr_ = "";
	    app_fluency_jsonstr_ = "";
	    speech_gop_jsonstr_ = "";
	    try{
	        //日志初始化
	        if(!app_logger_){
	        app_logger_ = spdlog::daily_logger_mt("center_app_logger", "logs/app_gop.txt", 1, 30);
            app_logger_->flush_on(spd::level::info);
	    }
	    ConfigParser config("config.ini");
	    config.get("loglevel",app_log_level_);
	    //设置全局日志输出级别
		if(app_log_level_ == 2 || app_log_level_ == 0 ) app_logger_->set_level(spd::level::info);
        else if (app_log_level_ == 1)app_logger_->set_level(spd::level::debug);
        else if (app_log_level_ == 4)app_logger_->set_level(spd::level::err);

        app_wav_file_ = wav_file;
        app_fluency_text_ = wav_text;
        app_gop_text_=wav_text;

	    //并行计算
        std::thread gop(AppPhoneGopCallback,this);
        std::thread fluency(AppFluencyCallback,this);
        //等待线程执行结束
        gop.join();
        fluency.join();

        app_logger_->info("gop result {0}",app_gop_jsonstr_);
        cout << "gop result: " << app_gop_jsonstr_ << endl;

        app_logger_->info("fluency result {0}",app_fluency_jsonstr_);
        cout << "fluency result: " << app_fluency_jsonstr_ << endl;

	    }catch(...)
	    {
	        app_logger_->info("ComputeCenter error");
	        return false;
	    }
		return true;
	}

	bool ComputeCenter::ComputeSpeechScore(const string &wav_file , const string &wav_text, int type)
	{
	    speech_gop_jsonstr_ = "";
	    try{
	        //日志初始化
	        if(!speech_logger_){
	        speech_logger_ = spdlog::daily_logger_mt("center_speech_logger", "logs/speech_gop.txt", 1, 30);
            speech_logger_->flush_on(spd::level::info);
	    }
	    ConfigParser config("config.ini");
	    config.get("loglevel",speech_log_level_);
	    //设置全局日志输出级别
		if(speech_log_level_ == 2 || speech_log_level_ == 0 ) speech_logger_->set_level(spd::level::info);
        else if (speech_log_level_ == 1)speech_logger_->set_level(spd::level::debug);
        else if (speech_log_level_ == 4)speech_logger_->set_level(spd::level::err);

        speech_wav_file_ = wav_file;
        speech_fluency_text_ = wav_text;
        speech_gop_text_=wav_text;

	    if (type == CMPFLAG::gop){
	        cout << "ComputeSpeechScore" << endl;
	        if(!speech_compute_gop_->ComputeGopScore(speech_wav_file_,speech_gop_text_,speech_gop_jsonstr_,type)){
	            speech_logger_->error("decode gop failed");
	            return false;
	        }
	    }

        speech_logger_->info("gop result {0}",speech_gop_jsonstr_);
        cout << "gop result: " << speech_gop_jsonstr_ << endl;

	    }catch(...)
	    {
	        speech_logger_->info("ComputeCenter error");
	        return false;
	    }
		return true;
	}

	void ComputeCenter::AppCompGop()
	{
	    if(false == app_compute_gop_->ComputeGopScore(this->app_wav_file_,this->app_gop_text_, app_gop_jsonstr_, cmp_type_))
	    {
	        errno_ = 1;//此时错误信息在gop_jsonstr_中
	    }
	}

    void ComputeCenter::AppCompFluency()
    {
        if(false == app_fluency_score_->GetSpeechFluencyJson(this->app_wav_file_,this->app_fluency_text_,app_fluency_jsonstr_,cmp_type_,false))
        {
            errno_ = 1;//此时错误信息在fluency_jsonstr_中
        }
    }

    //获取结果
	string ComputeCenter::GetAppJsonStrResult()
	{
	    //...构造json串
		return app_gop_jsonstr_+"|"+app_fluency_jsonstr_;
	}

	//获取结果
	string ComputeCenter::GetSpeechJsonStrResult()
	{
	    //...构造json串
		return speech_gop_jsonstr_+"|"+"";
	}
}// end namespace compute
