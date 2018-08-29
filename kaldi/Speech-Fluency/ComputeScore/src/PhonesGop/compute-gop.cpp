
#include <fstream>
#include "compute-gop.h"
#include "common/config/config-parser.hpp"
#include <boost/timer.hpp>
#include <boost/python.hpp>
#include <boost/python/module.hpp>
#include <boost/timer.hpp>
#include <algorithm>
#include <cctype>
#include <unistd.h>

using namespace boost;

#define min(a,b) ((a<b)?a:b)

namespace gop{
    namespace spd = spdlog;
	void ComputeGop::Init(const string &config_name)
	{
	    avrage_score_ = -1;
	    //初始化读取模型相关路径
		ConfigParser config(config_name);
		config.get("moudle",mdl_dir_name_);
		config.get("lang",lang_dir_name_);
		config.get("gmm",is_gmm_);
		config.get("lda",is_lda_);
		config.get("transform",transform_dir_name_);
		config.get("loglevel",log_level_);

        //如果没有获取到配置文件信息失败
        if (mdl_dir_name_ == "")
        {
            cout << "mdl文件目录有误,检查配置文件信息" << endl;
            //初始化失败直接终止
            exit(0);
        }
        //lda变换需要保存mat转换文件
		if (is_lda_) mat_dir_name_ = mdl_dir_name_ + "/final.mat";
		tree_dir_name_ = mdl_dir_name_ + "/tree";
		mdl_dir_name_ += "/final.mdl";
		lfst_dir_name_ = lang_dir_name_ + "/L.fst";

        //保存word对应的int信息,失败->返回
        if (!InitIntListText()) exit(0);
		//保存单词对应的音素信息
		if(!prepare_data_.FormatPhonesFileData(lang_dir_name_+"/align_lexicon.int")) exit(0);

        decode_computer_ = new DecodeCompute();
        decode_computer_->Init(is_gmm_,tree_dir_name_, mdl_dir_name_, lfst_dir_name_);

        if(!ReadFile("phones.txt") || !ReadFile("words.txt")) exit(0);
	}

    //保存单词对应的int信息
    bool ComputeGop::InitIntListText()
    {
        char line[1024] = {0};
		string str;
		int str_int;

		string word_filename = lang_dir_name_+"/words.txt";
		ifstream fword(word_filename, ios::in);
		if (!fword.is_open())
		{
		    cout << "打开words.txt失败" << endl;
		    return false;
		}
		while (fword.getline(line, sizeof(line)))
		{
				stringstream word(line);
				word >> str;
				word >> str_int;
				word2int_[str] = str_int;
		}
		return true;
    }

    bool ComputeGop::ReadFile(string file_name)
    {
        ifstream file;
		file.open(lang_dir_name_+"/"+file_name, ios::in);
		if (!file.is_open())
		{
			cout << "打开音素文件失败！" << endl;
			file.close();
			return  false;
		}

		bool is_phone = false;
		if (file_name == "phones.txt")
		is_phone = true;

		string s;
		int i = 0;
		while (getline(file, s))
		{
			int pos = s.find(" ");
			string pi = s.substr(0,pos);
			if(is_phone) phones_map_[i++] = pi;
			else words_map_[i++] = pi;
		}
		file.close();
		return true;
    }
    bool ComputeGop::GetIntListText(const string &text)
    {
        if(text.compare("")==0)
        {
            return false;
        }
        //读取文本信息
        istringstream is(text);
        string s;
        vector<string> str_vec;
        while (is>>s) str_vec.push_back(s);
        map<string,int>::iterator iter;

        for(auto str:str_vec)
        {
            iter = word2int_.find(str);
			if (iter != word2int_.end())
			{
			    utt_int_.push_back(word2int_[str]);
			}

        }
        return true;
    }

    bool ComputeGop::ComputeCommonFeature(const string &wav_filename)
    {
        //get mfcc feat.ark
    	Matrix<BaseFloat> gop_features;

		if(!mfcc_.ComputeMfccFeature(wav_filename, gop_features))
		{
            jsonstr_ = "make gop_feat.ark failed";
            //daily_logger_->error("{0} make gop_feat.ark failed",utt_);
            return false;
		}

		//feat.ark->raw.scp raw.ark
		if(!mfcc_.CopyFeats(gop_features))
		{
		    jsonstr_ = "make gop_raw.scp failed";
		    //daily_logger_->error("{0} make gop_raw.scp failed",utt_);
		    return false;
		}

		Matrix<double> cmvn_feature;
		//raw.scp->cmvn.ark cmvn.scp
		if(!mfcc_.ComputeCmvnStats(gop_features, cmvn_feature))
		{
		    jsonstr_ = "make gop_cmvn.scp failed";
		    //daily_logger_->error("{0} make gop_cmvn.scp failed",utt_);
		    return false;
		}

		//cmvn.scp->feats.scp apply_cmvn.ark
		if(!mfcc_.ApplyCmvnFeature(gop_features, cmvn_feature))
		{
		    jsonstr_ = "make gop_apply_cmvn.scp failed";
		    //daily_logger_->error("{0} make gop_apply_cmvn.scp failed",utt_);
		    return false;
		}
		return true;
    }

	bool ComputeGop::ComputeLdaFeature(const string &wav_filename, Matrix<BaseFloat>& transform_feature)
	{
	    //if(!ComputeCommonFeature(wav_filename)) return false;

		//get mfcc feat.ark
    	Matrix<BaseFloat> gop_features;

		if(!mfcc_.ComputeMfccFeature(wav_filename, gop_features))
		{
            jsonstr_ = "make gop_feat.ark failed";
            //daily_logger_->error("{0} make gop_feat.ark failed",utt_);
            return false;
		}

		//feat.ark->raw.scp raw.ark
		if(!mfcc_.CopyFeats(gop_features))
		{
		    jsonstr_ = "make gop_raw.scp failed";
		    //daily_logger_->error("{0} make gop_raw.scp failed",utt_);
		    return false;
		}

		Matrix<double> cmvn_feature;
		//raw.scp->cmvn.ark cmvn.scp
		if(!mfcc_.ComputeCmvnStats(gop_features, cmvn_feature))
		{
		    jsonstr_ = "make gop_cmvn.scp failed";
		    //daily_logger_->error("{0} make gop_cmvn.scp failed",utt_);
		    return false;
		}

		//cmvn.scp->feats.scp apply_cmvn.ark
		if(!mfcc_.ApplyCmvnFeature(gop_features, cmvn_feature))
		{
		    jsonstr_ = "make gop_apply_cmvn.scp failed";
		    //daily_logger_->error("{0} make gop_apply_cmvn.scp failed",utt_);
		    return false;
		}

		//apply_cmvn.ark->splice.ark
		Matrix<BaseFloat> splice_features;
		if(!mfcc_.SpliceFeats(gop_features, splice_features))
		{
		    jsonstr_ = "make gop_splice.ark failed";
		    //daily_logger_->error("{0} make gop_splice.ark failed",utt_);
		    return false;
		}

		//splice.ark->transform.ark
		if(!mfcc_.TransFormFeats(mat_dir_name_, splice_features, transform_feature))
		{
		    jsonstr_ = "make gop_transform.ark failed";
		   // daily_logger_->error("{0} make gop_transform.ark failed",utt_);
		    return false;
		}
		return true;
	}//end ComputeLdaFeature

	bool ComputeGop::ComputeRawFeature(const string &wav_filename)
	{
	    if(!ComputeCommonFeature(wav_filename)) return false;

		//apply_cmvn.ark->add_deltas.ark
		if(!mfcc_.AddDeltas())
		{
		    jsonstr_ = "make gop_add_deltas.ark failed";
		    //daily_logger_->error("{0} make gop_add_deltas.ark failed",utt_);
		    return false;
		}
		return true;

	}//end computeGmm

	bool ComputeGop::ComputeTransformFeature()
	{
		/*
		if(!mfcc_.TransFormFeats(transform_dir_name_+"/final.mat", "ark:gop_splice.ark", "ark:gop_transform_fmllr.ark"))
		{
		    jsonstr_ = "make transform_fmllr.ark failed";
		    daily_logger_->error("{0} make gop_transform_fmllr.ark failed",utt_);
		    return false;
		}
		if(!mfcc_.ComputeTransformFeature(transform_dir_name_))
		{
		    jsonstr_ = "make ComputeTransformFeature failed";
		    daily_logger_->error("{0} make ComputeTransformFeature failed",utt_);
		    return false;
		}
		*/
		return true;
	}
	
	//compute
	bool ComputeGop::ComputeGopScore(const string &wav_file , const string &text, string &json_rs, int cmp_type)
	{
	    cmp_type_ = cmp_type;
	    try
	    {
        //清空每次使用的容器空间
	    ClearVector();
	    /*if(!daily_logger_){
	        daily_logger_ = spdlog::daily_logger_mt("gop_logger", "logs/gop.txt", 1, 30);
            daily_logger_->flush_on(spd::level::info);
	    }*/
	    ConfigParser config("config.ini");
	    config.get("loglevel",log_level_);
	    config.get("scorelevel",score_level_);
	    //设置全局日志输出级别
		/*if(log_level_ == 2) daily_logger_->set_level(spd::level::info);
        else if (log_level_ == 1)daily_logger_->set_level(spd::level::debug);
        else if (log_level_ == 4)daily_logger_->set_level(spd::level::err);*/

	    ifstream file;
	    if(wav_file == "" || access(wav_file.c_str(), F_OK) != 0)
	    {
	        //daily_logger_->error("打开文件失败！");
			MakeJsonStr();
			json_rs = jsonstr_;
			return false;
	    }
        //截取wav文件名作为utt
		size_t found = wav_file.find(".");
		if(found)  utt_ = wav_file.substr(0,found);

		//生成wav.scp文件
		if(!prepare_data_.WavDataPrepare(wav_file,wav_scp_)){
		    error_no_ = 1;
		    //daily_logger_->error("make {0} gop_wav.scp failed",utt_);
		    MakeJsonStr();
		    json_rs = jsonstr_;
		    return false;
		}
		string wav_text = text;
		//过滤重音停顿标志
        FiltTag(wav_text, string("[-]"));
        FiltTag(wav_text, string("[s]"));
        FiltTag(wav_text, string("[/s]"));
        FiltTag(wav_text, string("-"));
        transform(wav_text.begin(),wav_text.end(),wav_text.begin(),::toupper);
		//获取文本对应的int编号
		if(!GetIntListText(wav_text)){
		    error_no_ = 1;
		    //daily_logger_->error("get {0} int_list failed",utt_);
		    MakeJsonStr();
		    json_rs = jsonstr_;
		    return false;
		}
        //准备特征向量
		string feat;
		string wav_scp_file = "scp:"+wav_scp_;

		Matrix<BaseFloat> transform_feature;

		if (is_lda_){
		   	//feat = "ark,s,cs:gop_transform.ark";
			if(!ComputeLdaFeature(wav_scp_file, transform_feature)) {
			    error_no_ = 1;
			    //daily_logger_->error("compute lda failed");
			    MakeJsonStr();
		        json_rs = jsonstr_;
			    return false;
			}
			//dnn多了transform步骤
			/*
			if(!is_gmm_)
			{
				feat = "ark,s,cs:gop_transformfinal.ark";
				if(!ComputeTransformFeature()) {
				    error_no_ = 1;
				    daily_logger_->error("ComputeTransformFeature failed");
				    MakeJsonStr();
		            json_rs = jsonstr_;
				    return false;
				}
				if(!mfcc_.TransFormFeats("ark:gop_trans.1","ark,s,cs:gop_transform.ark","ark:gop_transformfinal.ark")) {
				    error_no_ = 1;
				    daily_logger_->error("TransFormFeats failed");
				    MakeJsonStr();
		            json_rs = jsonstr_;
				    return false;
				}
			}
			*/
		}
		else{
		   	feat = "ark,s,cs:gop_add_deltas.ark";
			if(!ComputeRawFeature(wav_scp_file)) {
			    error_no_ = 1;
			    //daily_logger_->error("ComputeRawFeature failed");
			    MakeJsonStr();
		        json_rs = jsonstr_;
			    return false;
			}
		}

		//gmm和dnn不同的矩阵初始结构
		if (is_gmm_)
		{
			//SequentialBaseFloatMatrixReader feature_reader(feat);
			//const Matrix<BaseFloat> &features(feature_reader.Value());

			if(!decode_computer_->Compute(transform_feature, utt_int_)){
			    error_no_ = 1;
			    //daily_logger_->error("decode {0} failed",utt_);
		        MakeJsonStr();
		        json_rs = jsonstr_;
		        return false;
			}
			cout << "compute end!" << endl;
		}else{
			//SequentialBaseFloatCuMatrixReader feature_reader(feat);
			//const CuMatrix<BaseFloat> &features(feature_reader.Value());

			if(!decode_computer_->Compute(transform_feature, utt_int_)){
			    error_no_ = 1;
			    //daily_logger_->error("decode {0} failed",utt_);
		        MakeJsonStr();
		        json_rs = jsonstr_;
		        return false;
			}
		}
        //daily_logger_->info("decode {0} succeed",utt_);
		//音素识别对应的分数
		vector<float> gop_result;
		//强制对齐的音素 自由识别的音素 强制对齐每个音素对应的自由识别音素的个数
		vector<int32> phone_before,phone_after,phone_after_num;
		//获取解码结果
		decode_computer_->GetResult(gop_result, phone_before, phone_after, phone_after_num);

        /*
        cout << "gop_result: " << gop_result.size();
		cout << " phone_before: " << phone_before.size();
		cout << " phone_after: " << phone_after.size();
		cout << " phone_after_num: " << phone_after_num.size();
		cout << " gop_result:";
		for(auto score:gop_result)
		    cout <<" " << score;
		cout << endl;

        cout << " phone_before:";
		for(auto score:phone_before)
		    cout <<" " << phones_map_[score];
		cout << endl;

        cout << " phone_after:";
		for(auto score:phone_after)
		    cout <<" " << phones_map_[score];
		cout << endl;

		cout << " phone_after_num:";
		for(auto score:phone_after_num)
		    cout <<" " << score;
		cout << endl;
        */

		//日志拼接
		string loginfo = "";
		for(int i = 0,j=0; i<gop_result.size()&&j<phone_before.size(); i++,j++){
		    loginfo += phones_map_[phone_before[j]] + ":" + std::to_string(gop_result[i])+" ";
		}
		//daily_logger_->debug("decode phone score: {0}",loginfo);
		//根据解码结果按单词划分音素
		bool is_right = FilterPhoneForWord(phone_before, phone_after, phone_after_num, gop_result);

		if (is_right && phone_after.size())
		{
		    //计算每个单词的分数
		    ComputeWordScore();
		}
	    //构造json返回串
		MakeJsonStr();
		//daily_logger_->debug("result score: {0}",jsonstr_);
		json_rs = jsonstr_;
		//删除生成的中间文件
		DeleteTempFile();
		return true;
	    }catch(...)
	    {
	        //daily_logger_->error("未解码成功！");
			MakeJsonStr();
			json_rs = jsonstr_;
			//删除生成的中间文件
		    DeleteTempFile();
			return false;
	    }

	}

	bool ComputeGop::ComputeWordScore()
	{
	    //评分机制
	    int base_score = 0;
	    float weight=0,midle_weight=0;
	    if(score_level_ == 1 && cmp_type_ == 2){
	        base_score = 50;
            weight = 2;
            midle_weight = 1.8;
	    }
	    else if(score_level_ == 2 && cmp_type_ == 2){
	        base_score = 30;
	        weight = 1.5;
	        midle_weight = 1.3;
	    }
	    else {
	        base_score = 10;
	        weight = 1;
	        midle_weight = 1;
	    }
		vector<int> words = utt_int_;
		int pj = 0;
		int vowel_total_num = 0,consonant_total_num = 0;

		for (int i = 0; i< words.size(); i++)
		{
		    float vowel_word_score = 0; //单词中元音的分数
		    int vowel_num = 0;       //单词中元音个数
		    float consonant_word_score = 0; //单词中辅音的分数
		    int consonant_num = 0;       //单词中辅音的个数
		    float origin_score = 0;      //初始分数

		    int num_100 = 0,num_0 = 0,num_1 = 0;
		    //统计每个单词对应的音素的分数
			double score = 0;
			//单词对应的音素信息
			vector<int> phones = real_phone_[i];
			//cout << "***********start*************" << endl;
			//cout << " " << words_map_[words[i]] << ": ";
			for(int j = 0; j< phones.size(); j++)
			{
			    int temp_score = 0;

			    //cout << " " << phones_map_[phones[j]] << "-" << real_result_[pj+j];
			    if(real_result_[pj+j] < 25)//奇异值  暂设为40
			    {
			        temp_score = base_score;
			    }
			    else if(real_result_[pj+j] < 50) //加 拉长
			    {
			        temp_score = (real_result_[pj+j]*midle_weight+10);
			    }
			    else if(real_result_[pj+j] >= 50)
			    {
                    temp_score = (real_result_[pj+j]*weight)-1;
			    }
			    score += temp_score;
			    
			     //元音处理
			    if(phones_map_[phones[j]][0] == 'A' || phones_map_[phones[j]][0] == 'E'
			       || phones_map_[phones[j]][0] == 'I' || phones_map_[phones[j]][0] == 'O' || phones_map_[phones[j]][0] == 'U')
			    {
                     vowel_num++; //元音个数相加
                     if (temp_score>100) temp_score=100;
                     vowel_word_score += temp_score; //元音分数叠加
			    }
			    else{
			        if (temp_score>100) temp_score=100;
			        consonant_word_score += temp_score;
			    }
			    origin_score += temp_score;
				vector<int> phones_after = real_phone_after_[pj+j];
				
				//构造识别前后一一对应音素串
				if (real_result_[pj+j] == 100)
				{
					sigle_phone_before_.push_back(phones[j]);
					sigle_phone_after_.push_back(phones[j]);
				}
				else
				{
					sigle_phone_before_.push_back(phones[j]);
					sigle_phone_after_.push_back(phones_after[0]);
				}

			}
			pj += phones.size();
            if(phones.size())
            {
                score = (score)/phones.size();
                if(score>100) score = 100;
            }

			words_score_.push_back(score);
			avrage_score_ += score;
			vowel_score_ += vowel_word_score;
			consonant_score_ += consonant_word_score;
			vowel_total_num += vowel_num;
			consonant_total_num += (phones.size()-vowel_num);

            /*
            cout << endl;
            cout << " 总音素个数: " << phones.size() << " 元音个数: " << vowel_num << " 辅音个数: " << phones.size()-vowel_num << endl;
            cout << " 总音素分数: " << origin_score << " 元音分数: " << vowel_word_score << " 辅音分数: " << consonant_word_score << endl;
            cout << "单词分数: " << origin_score/phones.size() << endl;
            cout << "元音分数：" << vowel_word_score/vowel_num << endl;
            cout << "辐音分数：" << consonant_word_score/(phones.size()-vowel_num) << endl;
            cout << endl;
			cout << "***********end*************" << endl;
			*/
		}
		if(words.size()){
		    avrage_score_ /= words.size();
		    if(avrage_score_ == 10){//这里一般可以认为是乱说 或者识别环境非常差
		        avrage_score_ = 0;
		        for(auto &a:words_score_) a=0;
		    }
		}

		//元音分数
		if(vowel_total_num)
		{
		    vowel_score_ /= vowel_total_num;
		    vowel_score_ += 5;
		    if(vowel_score_ > 100) vowel_score_ = 100;
		}

		//辅音分数
		if(consonant_total_num)
		{
		    consonant_score_ /= consonant_total_num;
		    consonant_score_ += 5;
		    if(consonant_score_ > 100) consonant_score_ = 100;
		}

		//cout << " 句子元音得分: " << vowel_score_ << " 辅音得分: " << consonant_score_ << endl;
		//daily_logger_->info("utt {0} score: {1} vowel_score: {2} consonant_score: {3}",utt_, avrage_score_,vowel_score_,consonant_score_);
		return true;
	}

	//切分某一单词的音素序列
	//用vector<vector<int>> 存储单词对应的音素
	//用vector<float>存储整个单词序列对应的音素序列的音素评分
	//另一个vector< vector<int> > 存储自由识别后的音素序列 注：一个音素自由识别可能对应好几个音素
	bool ComputeGop::FilterPhoneForWord(vector<int32> &phone_before, vector<int32> &phone_after, vector<int32> &phone_after_num, vector<float> &gop_result)
	{
		//根据_B _E获取单词对应的起始音素
		int pi = 0;//按单词追踪自由识别后的音素序列
		for(int i = 0; i< phone_before.size(); i++)
		{
			string p = phones_map_[phone_before[i]];
			//以_B开始
		    if ( (p.find("SIL") == string::npos && p.find("SPN") == string::npos) && p.find("_B") != string::npos)
			{
			    //存储以_B开始_E结束的音素
				vector<int> subphone;
				do{
				    //存储一个单词对应的音素序列
				    //subphone_after 存储的是自由识别某一音素后的音素序列，
				    //某一音素可能识别出不止一个音素这个用vector存储起来
				    //phone_after_num存储的是强制对齐的一个音素识别出的对应的音素个数
				    //phone_after存储的是识别的所有音素序列
				    //subphone存储的是一个单词对应的音素序列
					vector<int> subphone_after;
					subphone.push_back(phone_before[i]);	
					for (int j = 0; j< phone_after_num[i]; j++) subphone_after.push_back(phone_after[pi+j]);
					real_phone_after_.push_back(subphone_after);
					real_result_.push_back(gop_result[i]);
					pi += phone_after_num[i];

					i++;
					p = phones_map_[phone_before[i]];
				} while(p.find("_E") == string::npos);

				//存储最后_E结束的音素和分数
				subphone.push_back(phone_before[i]);
				real_result_.push_back(gop_result[i]);

				real_phone_.push_back(subphone);

				vector<int> subphone_after;
				for (int j = 0; j< phone_after_num[i]; j++) subphone_after.push_back(phone_after[pi+j]);
				pi += phone_after_num[i];
				real_phone_after_.push_back(subphone_after);
			}
			// 单音素词
			else if (p.find("_S") != string::npos)
			{
				vector<int> subphone(1,phone_before[i]);
				real_result_.push_back(gop_result[i]);
				real_phone_.push_back(subphone);

				vector<int> subphone_after;
				for (int j = 0; j< phone_after_num[i]; j++) subphone_after.push_back(phone_after[pi+j]);
				pi += phone_after_num[i];
				real_phone_after_.push_back(subphone_after);
			}
			else pi += phone_after_num[i];
		}

		if (real_phone_.size() != 0 && real_result_.size() != 0) return true;
		return false;
	}
	
	//计算识别前后音素的编辑距离
	int ComputeGop::ComputeEditDistence(vector<int> &phone_before, vector<int> &phone_after)
	{
		int m = phone_before.size();
		int n = phone_after.size();
		
		if (m == 0) return n;
		if (n == 0) return m;
		
		typedef vector< vector<int> > tmatrix;
		tmatrix matrix(n+1);
		for (int i = 0; i <= n; i++)  matrix[i].resize(m + 1); 
		
		for(int i = 1; i<=n; i++) matrix[i][0] = i;
		for(int i = 1; i<=m; i++) matrix[0][i] = i;
		
		for (int i = 1; i<= n; i++)
		{
			const int si = phone_before[i-1];
			for (int j = 1; j<=m; j++)
			{
				 const int dj = phone_after[j - 1];  
				//step 5  
				int cost;  
				if (si == dj){  
					cost = 0;  
				}  
				else{  
					cost = 1;  
				}  
                //step 6  
				const int above = matrix[i - 1][j] + 1;  
				const int left  = matrix[i][j - 1] + 1;  
				const int diag  = matrix[i - 1][j - 1] + cost;  
				matrix[i][j] = min(above, min(left, diag));  
			}
		}
		
		return matrix[n][m];	
	}
	
	//构造返回给python的json格式串
	void ComputeGop::MakeJsonStr()
	{
		Json::FastWriter writerinfo;
		Json::Value  valueinfo;
		Json::Value  subvalue;
		Json::Value  subinfo;

		vector<int> words = utt_int_;

        if(words_map_.size() && words_score_.size())
        {
            for (int i = 1; i<= words.size(); i++)
		    {
			    subinfo["word"]= words_map_[words[i-1]];
			    subinfo["score"] = words_score_[i-1];
			    subvalue.append(subinfo);
		    }
        }
		valueinfo["words_info"] = subvalue;
		valueinfo["sentence_score"] = avrage_score_;
		valueinfo["vowel_score"] = vowel_score_;
		valueinfo["consonant_score"] = consonant_score_;
		valueinfo["SentenceStressScore"] = vowel_score_;
		valueinfo["WordsStressScore"] = consonant_score_;
		valueinfo["wordScore"] = consonant_score_;
		valueinfo["pauseScore"] = consonant_score_;
		valueinfo["wav_time"] = decode_computer_->wav_time_;
		valueinfo["error_no"] = 0;
		jsonstr_ = writerinfo.write(valueinfo);
	}
	
	string ComputeGop::GetJsonStrResult()
	{
		return jsonstr_;
	}

    void ComputeGop::FiltTag(string &text, string tag)
    {
        int num=tag.length();
        while(1)
        {
            std::size_t found = text.find(tag);
            if(found == std::string::npos) break;
            text.replace(found,num," ");
        }
    }

    bool ComputeGop::DeleteTempFile()
    {
        try{
             remove(wav_scp_.c_str());
		remove("gop_feats.ark");
		remove("gop_raw_mfcc.ark");
		remove("gop_raw_mfcc.scp");
		remove("gop_cmvn.scp");
		remove("gop_cmvn.ark");
		remove("gop_apply_cmvn.ark");
		if (is_lda_)
		{
			remove("gop_splice.ark");
			remove("gop_transform.ark");
			if (!is_gmm_) {
				remove("gop_transform_fmllr.ark");
				remove("lat.ark");
				remove("lattopost1.ark");
				remove("weighttopost1.ark");
				remove("gmmgpost1.ark");
				remove("pre_trans.1");
				remove("transform_feats2.ark");
				remove("lat2.ark");
				remove("latprune.ark");
				remove("lattopost2.ark");
				remove("weighttopost2.ark");
				remove("transtemp.1");
				remove("transformfinal.ark");
				remove("trans.1");
			}
		}else remove("gop_add_deltas.ark");
        }
        catch (...){
			KALDI_WARN << "Failed to delete file " << endl;
			return false;
		}
		return true;
    }
    bool ComputeGop::ClearVector()
    {
        edit_score_ = 0;
        avrage_score_ = -1;
        vowel_score_ = -1;
        consonant_score_ = -1;
        jsonstr_ = "";
        //收回uut vector空间
        utt_int_.clear();
        vector <int>().swap(utt_int_);

        //收回 real_gop
        words_score_.clear();
        vector <int>().swap(words_score_);

        real_phone_.clear();
        vector <vector<int>>().swap(real_phone_);

        real_result_.clear();
        vector <float>().swap(real_result_);

        real_phone_after_.clear();
        vector <vector<int>>().swap(real_phone_after_);

        sigle_phone_after_.clear();
        vector <int>().swap(sigle_phone_after_);

        sigle_phone_before_.clear();
        vector <int>().swap(sigle_phone_before_);

    }
	ComputeGop::~ComputeGop()
	{
	    if(decode_computer_)
	    {
	        delete decode_computer_;
	        decode_computer_ = NULL;
	    }

	}//end ~ComputeGop
}// end namespace gop
