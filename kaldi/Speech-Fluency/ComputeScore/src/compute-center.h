/*
date:     2017-12-7
author:   dinghaojie
refer to: http://www.kaldi-asr.org
*/

#ifndef _COMPUTE_CENTER_
#define _COMPUTE_CENTER_

#include <iostream>

#include "PhonesGop/compute-gop.h"
#include "fluency/score.h"
#include "common/json/json.h"
#include "common/spdlog/spdlog.h"
#include "common/spdlog/logger.h"

namespace compute{
	using namespace std;
	namespace spd = spdlog;
	using namespace gop;
	using namespace fluency;

    enum CMPFLAG { all = 1, gop, fluency,serial};

	class ComputeCenter{
		public:
			ComputeCenter();
			~ComputeCenter(){}

			//主计算函数，入参是wav文件名和其对应的文本
			bool ComputeScore(const string &wav_file, const string &wav_txt,int type);
			bool ComputeSpeechScore(const string &wav_file, const string &wav_txt, int type);
			string GetAppJsonStrResult();
			string GetSpeechJsonStrResult();


		    //线程函数
            void AppCompGop(void);
            void AppCompFluency(void);

             //线程函数
            void SpeechCompGop(void);
            void SpeechCompFluency(void);

            //test
            void TestPrint(int type){ cout << "print " << type << endl; }

		private:
            //评分模块对象
		    std::shared_ptr<Score> app_fluency_score_;
		    std::shared_ptr<ComputeGop> app_compute_gop_;

		    //评分模块对象
		    std::shared_ptr<ComputeGop> speech_compute_gop_;


			//传入的文件路径，文本信息
			string app_wav_file_;
			string app_fluency_text_;
			string app_gop_text_;

			string speech_wav_file_;
			string speech_fluency_text_;
			string speech_gop_text_;
			//模块评分json串
			string app_gop_jsonstr_;
			string app_fluency_jsonstr_;
			//模块评分json串
			string speech_gop_jsonstr_;
			string speech_fluency_jsonstr_;
            //错误码
            int errno_;
            //app log level
			int app_log_level_;
			std::shared_ptr<spdlog::logger> app_logger_;

			//speech log level
			int speech_log_level_;
			std::shared_ptr<spdlog::logger> speech_logger_;
			//compute_flag
			int cmp_speech_flag_;
			int cmp_app_flag_;
			int cmp_type_;
	};

    //线程函数
	static void* AppPhoneGopCallback(void *arg){
        ((ComputeCenter*)arg)->AppCompGop();//调用成员函数
        return NULL;
    }
    static void* AppFluencyCallback(void *arg){
        ((ComputeCenter*)arg)->AppCompFluency();//调用成员函数
        return NULL;
    }

}//end namespace compute

#endif
