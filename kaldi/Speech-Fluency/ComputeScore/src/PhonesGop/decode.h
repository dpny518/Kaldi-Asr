
/*
date:     2017-12-7
author:   dinghaojie
refer to: https://github.com/jimbozhang/kaldi-gop
*/

#ifndef _DECODE_
#define _DECODE_

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "gmm/am-diag-gmm.h"
#include "nnet2/am-nnet.h"
#include "decoder/training-graph-compiler.h"
#include "gmm/decodable-am-diag-gmm.h"
#include "hmm/transition-model.h"
#include "decoder/faster-decoder.h"
#include "fstext/fstext-utils.h"
#include "nnet2/decodable-am-nnet.h"
#include "hmm/hmm-utils.h"

namespace gop{
	
	using namespace kaldi;
	using namespace kaldi::nnet2;
	using namespace std;
	using fst::Fst;
	using fst::StdArc;

class DecodeCompute{
	public:
		DecodeCompute(){}
		~DecodeCompute();
		void Init(bool is_gmm, string &tree_filename, string &model_filename, string &lex_filename);
		//dnn compute
		bool Compute(const CuMatrix<BaseFloat> &feat, const vector<int32> &transcript);
		//gmm compute
		bool Compute(const Matrix<BaseFloat> &feat, const vector<int32> &transcript);
		
		void GetResult(vector<float> &gop_result, vector<int32> &phones_before, vector<int32> &phones_after, vector<int32> &phone_after_num);
	    float wav_time_;
	private:
		bool is_gmm_;
		AmNnet am_nnet_;
		AmDiagGmm am_;
		TransitionModel tm_;
		ContextDependency ctx_dep_;
		TrainingGraphCompiler *gc_;
		map<int32, int32> pdfid_to_tid;
		vector<float> gop_result_;
		vector<int32> phones_;
		vector<int32> total_align_after_;
		vector<int32> total_align_before_;
		// force align split 
		vector< vector<int32> > split_;
		// align before
		vector< vector<int32> > align_before_;


		//decode gmm or dnn
		BaseFloat Decode(fst::VectorFst<fst::StdArc> &fst, DecodableInterface &decodable,
				         vector<int32> *align, vector<int32> *words);
		
		BaseFloat ComputeNumera(DecodableInterface &decodable, int32 phone_l, int32 phone, int32 phone_r,
								vector<int32> &align, vector<int32> &words); 
		BaseFloat ComputeDenomin(DecodableInterface &decodable, int32 phone_l, int32 phone_r, 
								vector<int32> &align, vector<int32> &words);
        void ClearMem();
		//text decode
		void ForceAlign(DecodableInterface &decodable, const vector<int32> &transcript, vector<int32> &align, vector<int32> &words);

		void GetContextFromSplit(vector< vector<int32> > split, int32 index, int32 &phone_l, int32 &phone, int32 &phone_r);

};//end class DecodeCompute;

}//end namespace gop
#endif
