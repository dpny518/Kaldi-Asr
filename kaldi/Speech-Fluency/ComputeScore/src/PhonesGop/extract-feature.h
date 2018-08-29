
/*
date:     2017-12-7
author:   dinghaojie
refer to: http://www.kaldi-asr.org
*/

#ifndef _EXTRACT_FEATURE_
#define _EXTRACT_FEATURE_

#include <iostream>
#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "gmm/am-diag-gmm.h"
#include "tree/context-dep.h"
#include "hmm/transition-model.h"
#include "fstext/fstext-lib.h"
//#include "decoder/decoder-wrappers.h"
#include "decoder-wrappers.h"
#include "gmm/decodable-am-diag-gmm.h"
#include "feat/feature-functions.h"
#include "feat/feature-mfcc.h"
#include "feat/wave-reader.h"
#include "lat/kaldi-lattice.h"
#include "lat/lattice-functions.h"
#include "hmm/hmm-utils.h"
#include "hmm/posterior.h"
#include "transform/fmllr-diag-gmm.h"
#include "lat/push-lattice.h"
#include "lat/minimize-lattice.h"
#include "matrix/kaldi-matrix.h"
#include "transform/transform-common.h"


namespace gop{

using namespace std;
using namespace kaldi;
using  fst::SymbolTable;
using  fst::Fst;
using  fst::StdArc;
using  fst::VectorFst;

class MfccExtractFeature{
public:
	//compute-mfcc
	bool ComputeMfccFeature(const string &wav_scp_filename, Matrix<BaseFloat>& feature);
	//copy-feats
	bool CopyFeats(Matrix<BaseFloat>& feature); 
	//compute-cmvn		
	bool ComputeCmvnStats(Matrix<BaseFloat>& feature, Matrix<double>& cmvn_feature);
	//apply-cmvn
	bool ApplyCmvnFeature(Matrix<BaseFloat>& feature, Matrix<double>& cmvn_feature);
	//splice-feats
	bool SpliceFeats(Matrix<BaseFloat>& feature, Matrix<BaseFloat>& splice_feature);
	//transform-feats
	bool TransFormFeats(const string &mat_file, Matrix<BaseFloat>& splice_feature, Matrix<BaseFloat>& transform_feature);
	//add-deltas
	bool AddDeltas();
	//dnn-transform-feature
	bool ComputeTransformFeature(const string &transform_name);

	bool FasterGetLat(const string &transform_name, const string &read_feature_name, const string &write_name,bool deter_lat);
	bool LatfasterGetLat(const string &transform_name, const string &read_feature_name, const string &write_name,bool deter_lat);
	bool LatticeToPost(const string &read_name, const string &write_name);
	bool WeightSilenceToPost(const string &trans_dir_name, const string &read_name, const string &write_name);
	bool PostToGpost(const string &trans_dir_name, const string &read_name, const string &read_feature_name,const string &write_name);
	bool EstFmllrGpost(const string &trans_dir_name, const string &read_name, const string &read_feature_name, const string &write_name);
	bool LatticeDeterMinize(const string &read_name, const string &write_name);
	bool EstFmllr(const string &trans_dir_name, const string &read_name, const string &read_feature_name, const string &write_name);
	bool ComposeTransform(const string &read_name, const string &read_pre_name, const string &write_name);
	
	private:
	void AccumulateForUtterance(const Matrix<BaseFloat> &feats,
                            const GaussPost &gpost,
                            const TransitionModel &trans_model,
                            const AmDiagGmm &am_gmm,
                            FmllrDiagGmmAccs *spk_stats);
							
	void AccumulateForUtterance(const Matrix<BaseFloat> &feats,
                            const Posterior &post,
                            const TransitionModel &trans_model,
                            const AmDiagGmm &am_gmm,
                            FmllrDiagGmmAccs *spk_stats) ;

	std::string  m_strUtt;

};//end extractfeature
} //end namespace gop
#endif
