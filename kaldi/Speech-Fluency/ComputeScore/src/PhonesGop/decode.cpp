
#include <math.h>
#include "decode.h"

namespace gop{

	typedef typename fst::StdArc Arc;
	typedef typename Arc::StateId StateId;
	typedef typename Arc::Weight Weight;
	
		void DecodeCompute::Init(bool is_gmm, string &tree_file, string &model_file, string &lex_file)
		{
		    is_gmm_ = is_gmm;
		    wav_time_ = 0;
			bool binary;
			Input ki(model_file, &binary);
			tm_.Read(ki.Stream(), binary);
			if (is_gmm_) am_.Read(ki.Stream(), binary);
			else am_nnet_.Read(ki.Stream(), binary);

			ReadKaldiObject(tree_file, &ctx_dep_);
			fst::VectorFst<fst::StdArc> *lex_fst = fst::ReadFstKaldi(lex_file);
			vector<int32> disambig_syms;
			TrainingGraphCompilerOptions gopts;
			gc_ = new TrainingGraphCompiler(tm_, ctx_dep_, lex_fst, disambig_syms, gopts);

			for (size_t i = 0; i < tm_.NumTransitionIds(); i++){
				pdfid_to_tid[tm_.TransitionIdToPdf(i)] = i;
			}
		}

		//dnn compute
		bool DecodeCompute::Compute(const CuMatrix<BaseFloat> &feat, const vector<int32> &transcript)
		{
		    try{
		    ClearMem();
			DecodableAmNnet ali_decodable(tm_, am_nnet_, feat, true, 1.0);
			vector<int32> align;
			vector<int32> words;
			ForceAlign(ali_decodable, transcript, align, words);

			//get phone-align
			SplitToPhones(tm_, align, &split_);
			gop_result_.resize(split_.size());
			phones_.resize(split_.size());

			int32 frame_start_idx = 0;
			for (MatrixIndexT i = 0; i < split_.size(); i++)
			{
				CuSubMatrix<BaseFloat> feats_in_phone = feat.Range(frame_start_idx, split_[i].size(), 0, feat.NumCols());
				const CuMatrix<BaseFloat> features(feats_in_phone);
				DecodableAmNnet split_decodable(tm_, am_nnet_, features, true, 1.0);

				int32 phone, phone_l, phone_r;
				GetContextFromSplit(split_, i, phone_l, phone, phone_r);

				vector<int32> align_before;
				vector<int32> align_after;
				vector<int32> words_before;
				vector<int32> words_after;
				BaseFloat gop_numerator = ComputeNumera(split_decodable, phone_l, phone, phone_r, align_before, words_before);
				BaseFloat gop_denominator = ComputeDenomin(split_decodable, phone_l, phone_r, align_after, words_after);
				gop_result_[i] = (gop_numerator - gop_denominator) / split_[i].size();
				for (int i = 0; i < align_after.size(); i++)
					total_align_after_.push_back(align_after[i]);

				gop_result_[i] = pow(10, gop_result_[i]+2);

				// decode failed
				if (gop_numerator == 0 || gop_denominator == 0) gop_result_[i] = 0;
				phones_[i] = phone;
				for (int i = 0; i < align_before.size(); i++)
						total_align_before_.push_back(align_before[i]);
				frame_start_idx += split_[i].size();
			}
		    }catch(...){
		        return false;
		    }
		    return true;
		}

		//gmm compute
		bool DecodeCompute::Compute(const Matrix<BaseFloat> &feat, const vector<int32> &transcript)
		{
		    try{
		    ClearMem();
			DecodableAmDiagGmmScaled ali_decodable(am_, tm_, feat, 1.0);
			vector<int32> align;
			vector<int32> words;
			ForceAlign(ali_decodable, transcript, align, words);
			cout << "align.size(): " << align.size() << endl;
			cout << "feat.row: " << feat.NumRows() << endl;
            if(feat.NumRows())
                wav_time_ = (feat.NumRows()-1)*0.01+0.025;
            else wav_time_ = -1;

			//get phone-align
			SplitToPhones(tm_, align, &split_);

			gop_result_.resize(split_.size());
			phones_.resize(split_.size());

			int32 frame_start_idx = 0;
			for (MatrixIndexT i = 0; i < split_.size(); i++)
			{
				SubMatrix<BaseFloat> feats_in_phone = feat.Range(frame_start_idx, split_[i].size(), 0, feat.NumCols());
				const Matrix<BaseFloat> features(feats_in_phone);
				DecodableAmDiagGmmScaled split_decodable(am_, tm_, features, 1.0);

				int32 phone, phone_l, phone_r;
				GetContextFromSplit(split_, i, phone_l, phone, phone_r);

				vector<int32> align_before;
				vector<int32> words_before;
				vector<int32> align_after;
				vector<int32> words_after;

				BaseFloat gop_numerator = ComputeNumera(split_decodable, phone_l, phone, phone_r, align_before, words_before);

				BaseFloat gop_denominator = ComputeDenomin(split_decodable, phone_l, phone_r, align_after, words_after);

				gop_result_[i] = (gop_numerator - gop_denominator) / split_[i].size();

                //cout << "***************************" << endl;
                //cout << " gop_numerator: " << gop_numerator << " gop_denominator: " << gop_denominator << endl;
                //cout << " gop_result_before: " << gop_result_[i] << endl;

				for (int i = 0; i < align_after.size(); i++)
					total_align_after_.push_back(align_after[i]);

				gop_result_[i] = pow(10, gop_result_[i]+2);
                //cout << " gop_result_after: " << gop_result_[i] << endl;
                //cout << "***************************" << endl;
				// decode failed
				if (gop_numerator == 0 || gop_denominator == 0) gop_result_[i] = 0;
				phones_[i] = phone;

				frame_start_idx += split_[i].size();
			}
		    }catch(...){
		        return false;
		    }
		    return true;
		}

		//decode gmm or dnn
		BaseFloat DecodeCompute::Decode(fst::VectorFst<fst::StdArc> &fst, DecodableInterface &decodable,
				         vector<int32> *align, vector<int32> *words)
		{
			FasterDecoderOptions decode_opts;
			decode_opts.beam = 500;
			//decode_opts.beam = 100;
			FasterDecoder decoder(fst, decode_opts);
			decoder.Decode(&decodable);
			if (!decoder.ReachedFinal()){
				KALDI_WARN << " not successed ";
				return 0;
			}

			fst::VectorFst<LatticeArc> decoded;
			decoder.GetBestPath(&decoded);
			LatticeWeight weight;
			GetLinearSymbolSequence(decoded, align, words, &weight);
			BaseFloat likelihood = -(weight.Value1()+weight.Value2());

			return likelihood;
		}
		
		BaseFloat DecodeCompute::ComputeNumera(DecodableInterface &decodable, int32 phone_l, int32 phone, int32 phone_r,
								vector<int32> &align, vector<int32> &words) 
		{
			vector<int32> phoneseq(3);
			phoneseq[0] = phone_l;
			phoneseq[1] = phone;
			phoneseq[2] = phone_r;

			fst::VectorFst<fst::StdArc> fst;
			StateId cur_state = fst.AddState();
			fst.SetStart(cur_state);
			
			for(size_t c = 0; c < tm_.GetTopo().NumPdfClasses(phone); c++){
				int32 pdf_id;
				ctx_dep_.Compute(phoneseq, c, &pdf_id);
				int tid = pdfid_to_tid[pdf_id];

				StateId next_state = fst.AddState();
				Arc arc(tid, 0, Weight::One(), next_state);
				fst.AddArc(cur_state, arc);
				cur_state = next_state;

				Arc arc_selfloop(tid, 0, Weight::One(), cur_state);
				fst.AddArc(cur_state, arc_selfloop);
			}
			fst.SetFinal(cur_state, Weight::One());

			return Decode(fst, decodable, &align, &words);
		}

		BaseFloat DecodeCompute::ComputeDenomin(DecodableInterface &decodable, int32 phone_l, int32 phone_r, 
								vector<int32> &align, vector<int32> &words)
		{
			vector<int32> phoneseq(3);
			phoneseq[0] = phone_l;
			phoneseq[2] = phone_r;

			fst::VectorFst<fst::StdArc> fst;
			StateId start_state = fst.AddState();
			fst.SetStart(start_state);
			
			const vector<int32> &phone_syms = tm_.GetPhones();
			for(size_t i = 0; i< phone_syms.size(); i++)
			{
				int32 phone = phone_syms[i];
				phoneseq[1] = phone;
				const int pdfclass_num = tm_.GetTopo().NumPdfClasses(phone);
				StateId cur_state = start_state;
				for(size_t c = 0; c < pdfclass_num; c++){
					int32 pdf_id;
					ctx_dep_.Compute(phoneseq, c, &pdf_id);
					int tid = pdfid_to_tid[pdf_id];

					StateId next_state = fst.AddState();
					Arc arc(tid, 0, Weight::One(), next_state);
					fst.AddArc(cur_state, arc);
					cur_state = next_state;

					Arc arc_selfloop(tid, 0, Weight::One(), cur_state);
					fst.AddArc(cur_state, arc_selfloop);
				}
				Arc arc(0, 0, Weight::One(), start_state);
				fst.AddArc(cur_state, arc);
			}
			fst.SetFinal(start_state, Weight::One());
			return Decode(fst, decodable, &align, &words);
		}

		void DecodeCompute::ForceAlign(DecodableInterface &decodable, const vector<int32> &transcript, vector<int32> &align,
										vector<int32> &words)
		{
			fst::VectorFst<fst::StdArc> ali_fst;
			gc_->CompileGraphFromText(transcript, &ali_fst);
			Decode(ali_fst, decodable, &align, &words);
		}

		void DecodeCompute::GetContextFromSplit(vector< vector<int32> > split, int32 index, 
												int32 &phone_l, int32 &phone, int32 &phone_r)
		{
			phone_l = (index > 0)? tm_.TransitionIdToPhone(split[index-1][0]):1;
			phone = tm_.TransitionIdToPhone(split[index][0]);
			phone_r = (index < split.size() - 1)? tm_.TransitionIdToPhone(split[index+1][0]):1;
		}

        void DecodeCompute::ClearMem()
        {
            split_.clear();
            vector <vector<int32>>().swap(split_);

            gop_result_.clear();
            vector <float>().swap(gop_result_);

            phones_.clear();
            vector <int32>().swap(phones_);

            total_align_after_.clear();
            vector <int32>().swap(total_align_after_);

            total_align_before_.clear();
            vector <int32>().swap(total_align_before_);

            align_before_.clear();
            vector <vector<int32>>().swap(align_before_);
        }

		void  DecodeCompute:: GetResult(vector<float> &gop_result, 
						vector<int32> &phones_before, vector<int32> &phones_after, vector<int32> &phone_after_num)
		{
			gop_result = gop_result_;
			phones_before = phones_;

			vector< vector<int32> > temp_split;
			SplitToPhones(tm_, total_align_after_, &temp_split);

			phone_after_num.resize(split_.size());
			int j = 0;
			for (int i = 0; i < split_.size(); i++)
			{
				int phone_frame_num = split_[i].size();

				int sum_frame = 0;
				int n = 0;
				do{
					sum_frame += temp_split[j++].size();
					n++;
				} while(sum_frame < phone_frame_num);
				// before-after_trueNum
				phone_after_num[i] = n;
			}

			//after_align to phone
			for (int i = 0; i< temp_split.size(); i++)
			{
				int32 phone, phone_l, phone_r;
				GetContextFromSplit(temp_split, i, phone_l, phone, phone_r);
				phones_after.push_back(phone);
			}
		}
		DecodeCompute::~DecodeCompute()
		{
		    if(gc_){
		        delete gc_;
		        gc_ = NULL;
		    }
		}

}//end namespace gop
