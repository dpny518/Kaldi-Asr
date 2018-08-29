/*
date:     2017-12-7
author:   dinghaojie
refer to: http://www.kaldi-asr.org
*/

#ifndef _COMPUTE_GOP_
#define _COMPUTE_GOP_

#include <iostream>
#include "lat/word-align-lattice-lexicon.h"
#include "prepare-data.h"
#include "extract-feature.h"
#include "decode.h"
#include "common/json/json.h"
#include "common/spdlog/spdlog.h"
#include "common/spdlog/logger.h"

namespace gop{
	using namespace std;
	namespace spd = spdlog;
	class ComputeGop{
		public:
			ComputeGop(){Init();}
			//初始化 模型读取 相关数据文件读取保存(如音素 单词 单词关联音素)
			void Init(const string &config_name = "config.ini");

			//计算特征相关函数
			bool ComputeCommonFeature(const string &wav_filename);
			bool ComputeLdaFeature(const string &wav_filename, Matrix<BaseFloat>& transform_feature);
			bool ComputeRawFeature(const string &wav_filename);
			bool ComputeTransformFeature();
			//主计算函数
			bool ComputeGopScore(const string &wav_file, const string &wav_txt, string &json_rs, int cmp_type);

			//python调用接口
			string GetJsonStrResult();
			~ComputeGop();
			
			
		private:
			//计算两个音素串的编辑距离
			int ComputeEditDistence(vector<int> &phone_before, vector<int> &phone_after);
			//以单词切分音素串
			bool FilterPhoneForWord(vector<int32> &phone_before, vector<int32> &phone_after, vector<int32> &phone_after_num, vector<float> &gop_result);
			//根据音素分数计算单词分数
			bool ComputeWordScore();
			//过滤停顿 重音字符
			void FiltTag(string &text,string tag);
			//获取文本对应的int串
			bool GetIntListText(const string &text);
			bool InitIntListText();
			bool ReadFile(string file_name);
			//构造json串
			void MakeJsonStr();
			bool DeleteTempFile();
			//清空vector
			bool ClearVector();
			
			//处理特征类
			MfccExtractFeature mfcc_;
			//准备数据类
			PrepareData prepare_data_;
			//解码类
			DecodeCompute *decode_computer_;
			//存储每个uut对应的单词int序列
			vector<int> utt_int_;
			map<string, int> word2int_;
			
			//存放以_B开始_E结束的音素序列，对应强制对齐前
			vector< vector<int> > real_phone_;
			//存放每个音素的分数
			vector<float> real_result_;
			//存放以_B开始_E结束的音素序列
			vector< vector<int> > real_phone_after_;
			
			//存放为了计算编辑距离的识别前后音素序列
			vector<int> sigle_phone_after_;
			vector<int> sigle_phone_before_;
			
			//按顺序存储每个单词的分数
			vector<int> words_score_;
			//音素/单词的整形和string对应
			map<int32, string > phones_map_;
			map<int32, string > words_map_;
			
			//模块文件路径信息
			string mdl_dir_name_;
			string tree_dir_name_;
			string lang_dir_name_;
			string lfst_dir_name_;
			string mat_dir_name_;
			string transform_dir_name_;
			
			//生成的wav_scp文件
			string wav_scp_;
			string utt_;
			//返回的json格式串
			string jsonstr_;
			//编辑距离分数
			int edit_score_;
            //元音得分，辅音得分
			int vowel_score_;
			int consonant_score_;
			//句子分数
			int avrage_score_;
			//标识模型是GMM/DNN
			bool is_gmm_;
			//表示是否提取向量用lda
			bool is_lda_;
			//评分配置
			int score_level_;
			//log level
			int log_level_;
			int cmp_type_;
			std::shared_ptr<spdlog::logger> daily_logger_;
			
			//错误信息相关
			int error_no_;
			string error_infor_;



	};
}//end namespace gop







#endif
