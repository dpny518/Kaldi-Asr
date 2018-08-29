
#include "extract-feature.h"
#include "transform/cmvn.h"
#include "decoder/faster-decoder.h"
#include <boost/timer.hpp>
using namespace boost;

using namespace std;

namespace gop{

	bool MfccExtractFeature::ComputeMfccFeature(const string &wav_rspecifier, Matrix<BaseFloat>& feature)
	{
		try{
            //some default option
		    bool subtract_mean = false;
		    BaseFloat vtln_warp = 1.0;
		    string vtln_map_rspecifier;
		    string utt2spk_rspecifier;
		    int32 channel = -1;
		    BaseFloat min_duration = 0.0;

		    //init reader_file,write_file
		    SequentialTableReader<WaveHolder> reader(wav_rspecifier);
		    
		    m_strUtt = reader.Key();

		    const WaveData &wave_data = reader.Value();
		    if (wave_data.Duration() < min_duration)
		    {
			    KALDI_WARN << "File: " << m_strUtt << "is too short(" << wave_data.Duration() << " sec):producing no output.";
		    }
			//get chan
		    int32 num_chan = wave_data.Data().NumRows();
		    int this_chan = channel;
		    {
			    KALDI_ASSERT(num_chan > 0);
			    if (channel == -1)
			    {
				    //default chan=0, this can option
				    this_chan = 0;
				    if (num_chan != 1)
					    KALDI_WARN << "Channel not specified but you have data with " << num_chan << " channels; defaulting to zero";
			    }
		    }

		    SubVector<BaseFloat> waveform(wave_data.Data(), this_chan);

		    MfccOptions mfcc_opts;
		    mfcc_opts.use_energy = false;
		    Mfcc mfcc(mfcc_opts);
			mfcc.Compute(waveform, vtln_warp, &feature);

		}catch (...){
			KALDI_WARN << "Failed to compute features for utterance ";
			return false;
		}
		return true;
	}//end ComputeMfccFeature

	bool MfccExtractFeature::CopyFeats(Matrix<BaseFloat>& feature)
	{
		try {
		//default init
		bool binary = true;
		bool sphinx_in = false;
		bool compress = true;
		int32 compression_method_in = 1;

		CompressionMethod compression_method = static_cast<CompressionMethod>(compression_method_in);
		
		//string wspecifier = "ark,scp:gop_raw_mfcc.ark,gop_raw_mfcc.scp";
		
		//copy-feats
		//CompressedMatrixWriter kaldi_writer(wspecifier);

		//kaldi_writer.Write(m_strUtt,CompressedMatrix(feature,compression_method));
		} catch(const exception &e){
			cerr << e.what();
			return false;
		}
	    return true;
	}//end CopyFeats()

	//compute-cmvn		
	bool MfccExtractFeature::ComputeCmvnStats(Matrix<BaseFloat>& feature, Matrix<double>& cmvn_feature)
	{
		try{
		//option spkutt

		//default filename
		//string wspecifier = "ark,scp:gop_cmvn.ark,gop_cmvn.scp";
		//string weights_rspecifier;

		//init file
		//DoubleMatrixWriter writer(wspecifier);

		InitCmvnStats(feature.NumCols(), &cmvn_feature);
		AccCmvnStats(feature,NULL, &cmvn_feature);

		} catch (const exception &e){
			cerr << e.what();
			return false;
		}
		return true;
	}// end CmvnFeats
	
	//apply-cmvn
	bool MfccExtractFeature::ApplyCmvnFeature(Matrix<BaseFloat>& feature, Matrix<double>& cmvn_feature)
	{
		try{
		//option uttspk
		
		//default int 
		bool norm_vars = false;
		bool norm_means = true;
		bool reverse = false;

		//default filename
		//string feat_wspecifier = "ark:gop_apply_cmvn.ark";

		//BaseFloatMatrixWriter feat_writer(feat_wspecifier);
		
		ApplyCmvn(cmvn_feature, norm_vars, &feature);
		
		//write file
		//feat_writer.Write(m_strUtt, feature);

		} catch (const exception &e){
			cerr << e.what();
			return false;
		}
		return true;
	}//end applycmvn

	//splice-feats
	bool MfccExtractFeature::SpliceFeats(Matrix<BaseFloat>& feature, Matrix<BaseFloat>& splice_feature)
	{
		try{
		//default init
		int32 left_context = 3, right_context = 3;

		//default file
		//string rspecifier = "ark:gop_apply_cmvn.ark";
		//string wspecifier = "ark:gop_splice.ark";

		//SequentialBaseFloatMatrixReader kaldi_reader(rspecifier);
		//BaseFloatMatrixWriter kaldi_writer(wspecifier);

		//Matrix<BaseFloat> spliced;
		SpliceFrames(feature, left_context, right_context, &splice_feature);

		//kaldi_writer.Write(kaldi_reader.Key(), splice_feature);

		} catch (const exception &e){
			cerr << e.what();
			return false;
		}
		return true;
	}//end splice 

	//transform-feats
	bool MfccExtractFeature::TransFormFeats(const string &mat_file, Matrix<BaseFloat>& splice_feature, Matrix<BaseFloat>& transform_feature)
	{
		try{
			string utt2spk_rspecifier;
			string transform_rspecifier_or_rxfilename = mat_file;
			//string feat_rspecifier = "ark:gop_splice.ark";
			//string feat_wspecifier = "ark:gop_transform.ark";
		
			//SequentialBaseFloatMatrixReader feat_reader(feat_rspecifier);
			//BaseFloatMatrixWriter feat_writer(feat_wspecifier);

			//读取mat文件做lda转换
			RandomAccessBaseFloatMatrixReaderMapped transform_reader;
			bool use_global_transform;
			Matrix<BaseFloat> global_transform;
			if (ClassifyRspecifier(transform_rspecifier_or_rxfilename, NULL, NULL)
				== kNoRspecifier) {
				// not an rspecifier -> interpret as rxfilename....
				use_global_transform = true;
				ReadKaldiObject(transform_rspecifier_or_rxfilename, &global_transform);
			} else {  // an rspecifier -> not a global transform.
				use_global_transform = false;
				if (!transform_reader.Open(transform_rspecifier_or_rxfilename,
                                 utt2spk_rspecifier)) {
				KALDI_ERR << "Problem opening transforms with rspecifier "
                  << '"' << transform_rspecifier_or_rxfilename << '"'
                  << " and utt2spk rspecifier "
                  << '"' << utt2spk_rspecifier << '"';
				}
			}	

			enum { Unknown, Logdet, PseudoLogdet, DimIncrease };
			int32 logdet_type = Unknown;
			double tot_t = 0.0, tot_logdet = 0.0;  // to compute average logdet weighted by time...
			int32 num_done = 0;
			BaseFloat cached_logdet = -1;

			//std::string utt = feat_reader.Key();
			//const Matrix<BaseFloat> &feat(feat_reader.Value());

			if (!use_global_transform && !transform_reader.HasKey(m_strUtt)) {
				KALDI_WARN << "No fMLLR transform available for utterance " << m_strUtt << ", producing no output for this utterance";
				return false;
			}

			const Matrix<BaseFloat> &trans = (use_global_transform ? global_transform : transform_reader.Value(m_strUtt));

			int32 transform_rows = trans.NumRows(),
				  transform_cols = trans.NumCols(),
				  feat_dim = splice_feature.NumCols();

			Matrix<BaseFloat> feat_out(splice_feature.NumRows(), transform_rows);

			if (transform_cols == feat_dim) {
				feat_out.AddMatMat(1.0, splice_feature, kNoTrans, trans, kTrans, 0.0);
			} 
			else if (transform_cols == feat_dim + 1) {
				// append the implicit 1.0 to the input features.
				SubMatrix<BaseFloat> linear_part(trans, 0, transform_rows, 0, feat_dim);
				feat_out.AddMatMat(1.0, splice_feature, kNoTrans, linear_part, kTrans, 0.0);
				Vector<BaseFloat> offset(transform_rows);
				offset.CopyColFromMat(trans, feat_dim);
				feat_out.AddVecToRows(1.0, offset);
			} 
			else {
				return false;
			}
			
			transform_feature = feat_out;

			//feat_writer.Write(m_strUtt, feat_out);

    		/*
			for (;!feat_reader.Done(); feat_reader.Next()) {
				std::string utt = feat_reader.Key();
				const Matrix<BaseFloat> &feat(feat_reader.Value());
				if (!use_global_transform && !transform_reader.HasKey(utt)) {
				KALDI_WARN << "No fMLLR transform available for utterance "
					<< utt << ", producing no output for this utterance";
				num_error++;
				continue;
			}
			const Matrix<BaseFloat> &trans =
			(use_global_transform ? global_transform : transform_reader.Value(utt));
			int32 transform_rows = trans.NumRows(),
			transform_cols = trans.NumCols(),
			feat_dim = feat.NumCols();

			Matrix<BaseFloat> feat_out(feat.NumRows(), transform_rows);

			if (transform_cols == feat_dim) {
			feat_out.AddMatMat(1.0, feat, kNoTrans, trans, kTrans, 0.0);
			} else if (transform_cols == feat_dim + 1) {
			// append the implicit 1.0 to the input features.
			SubMatrix<BaseFloat> linear_part(trans, 0, transform_rows, 0, feat_dim);
			feat_out.AddMatMat(1.0, feat, kNoTrans, linear_part, kTrans, 0.0);
			Vector<BaseFloat> offset(transform_rows);
			offset.CopyColFromMat(trans, feat_dim);
			feat_out.AddVecToRows(1.0, offset);
			} else {
			return false;
			}
			
			
			feat_writer.Write(utt, feat_out);
			}
			*/
		} catch (const exception &e){
			cerr << e.what();
			return false;
		}		
		return true;
	}//end transform

	//add-deltas
	bool MfccExtractFeature::AddDeltas()
	{
		try{
		DeltaFeaturesOptions opts;
		//default filename
		string rspecifier = "ark:gop_cmvn.ark";
		string wspecifier = "ark:gop_add_deltas.ark";

		//init file
		SequentialBaseFloatMatrixReader feat_reader(rspecifier);
		BaseFloatMatrixWriter feat_writer(wspecifier);

		//compute
		string key = feat_reader.Key();
		const Matrix<BaseFloat> &feats = feat_reader.Value();

		Matrix<BaseFloat> new_feats;
		ComputeDeltas(opts, feats, &new_feats);
		feat_writer.Write(key, new_feats);
		} catch (const exception &e){
			cerr << e.what();
			return false;
		}
		return true;
	}//end AddDeltas

	bool MfccExtractFeature::ComputeTransformFeature(const string &transform_name)
	{
		/*
		try{
		    timer t;
		    //FasterGetLat(transform_name, "ark,s,cs:transform_fmllr.ark", "ark:lat.ark", true);
			LatfasterGetLat(transform_name, "ark,s,cs:transform_fmllr.ark", "ark:lat.ark", true);
			cout << "gmm decoder use time:" << t.elapsed()<< "s" << endl;

            timer t1;
			LatticeToPost("ark:lat.ark","ark:lattopost1.ark");
			cout << "latticetopost use time:" << t1.elapsed()<< "s" << endl;

            timer t2;
			WeightSilenceToPost(transform_name+"/final.alimdl", "ark:lattopost1.ark", "ark:weighttopost1.ark");
			cout << "WeightSilenceToPost use time:" << t2.elapsed()<< "s" << endl;

            timer t3;
			PostToGpost(transform_name,"ark:weighttopost1.ark", "ark,s,cs:transform_fmllr.ark", "ark:gmmgpost1.ark");
			cout << "PostToGpost use time:" << t3.elapsed()<< "s" << endl;

            timer t4;
			EstFmllrGpost(transform_name,"ark,s,cs:gmmgpost1.ark", "ark,s,cs:transform_fmllr.ark", "ark:pre_trans.1");
			cout << "EstFmllrGpost use time:" << t4.elapsed()<< "s" << endl;

            timer t5;
			TransFormFeats("ark:pre_trans.1","ark,s,cs:transform_fmllr.ark","ark:transform_feats2.ark");
			cout << "TransFormFeats use time:" << t5.elapsed()<< "s" << endl;

			timer t6;
			//FasterGetLat(transform_name, "ark,s,cs:transform_feats2.ark", "ark:lat2.ark", true);
			LatfasterGetLat(transform_name, "ark,s,cs:transform_feats2.ark", "ark:lat2.ark", true);
			cout << "second gmm decoder use time:" << t6.elapsed()<< "s" << endl;

			LatticeDeterMinize("ark:lat2.ark", "ark:latprune.ark");
			LatticeToPost("ark:latprune.ark", "ark:lattopost2.ark");
			WeightSilenceToPost(transform_name+"/final.mdl", "ark:lattopost2.ark", "ark:weighttopost2.ark");
			EstFmllr(transform_name,"ark,s,cs:weighttopost2.ark", "ark,s,cs:transform_feats2.ark", "ark:transtemp.1");
			ComposeTransform("ark:transtemp.1", "ark:pre_trans.1", "ark:trans.1");
		}catch(const exception &e){
			//should throw
		}
		*/

		return true;
	}
	
	bool MfccExtractFeature::LatfasterGetLat(const string &transform_name, const string &read_feature_name, const string &write_name, bool deter_lat)
	{
		try{
		bool allow_partial = true;
	   	BaseFloat acoustic_scale = 0.083;
		LatticeFasterDecoderConfig config;

		config.determinize_lattice = deter_lat;
		
		config.beam = 2.0;
		config.max_active = 200;
		config.lattice_beam = 2.0;


		/*config.beam = 13.0;
		config.max_active = 7000;
		config.lattice_beam = 6.0;*/

	    std::string model_in_filename = transform_name+"/final.mdl",
        fst_in_str = transform_name+"/HCLG.fst",
        //fst_in_str = transform_name+"/L.fst",
        word_syms_filename = transform_name+"/words.txt",
        feature_rspecifier = read_feature_name,
        lattice_wspecifier = write_name;

		TransitionModel trans_model;
		AmDiagGmm am_gmm;

	    timer t;
		bool binary;
		Input ki(model_in_filename, &binary);
		trans_model.Read(ki.Stream(), binary);
		am_gmm.Read(ki.Stream(), binary);
		cout << "read model " << t.elapsed() << "s" << endl;

        timer t1;
		bool determinize = config.determinize_lattice;
		CompactLatticeWriter compact_lattice_writer;
		LatticeWriter lattice_writer;
		if (! (determinize ? compact_lattice_writer.Open(lattice_wspecifier)
           : lattice_writer.Open(lattice_wspecifier)))
				KALDI_ERR << "Could not open table for writing lattices: "
                 << lattice_wspecifier;
        cout << "read lat " << t1.elapsed() << "s" << endl;

        timer t2;
		fst::SymbolTable *word_syms = NULL;
		if (word_syms_filename != "")
			if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
				KALDI_ERR << "could not read symbol table from file " << word_syms_filename;
        cout << "read word_sys_filename " << t2.elapsed() << "s" << endl;

		if (ClassifyRspecifier(fst_in_str, NULL, NULL) == kNoRspecifier) {
			SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
			timer t3;
			Fst<StdArc> *decode_fst = fst::ReadFstKaldiGeneric(fst_in_str);
			LatticeFasterDecoder decoder(*decode_fst, config);
            cout << "ReadFstKaldiGeneric use time" << t3.elapsed() << "s" << endl;

			std::string utt = feature_reader.Key();
			Matrix<BaseFloat> features (feature_reader.Value());
			feature_reader.FreeCurrent();
			if (features.NumRows() == 0) {
				KALDI_WARN << "Zero-length utterance: " << utt;
			}

			DecodableAmDiagGmmScaled gmm_decodable(am_gmm, trans_model, features,
                                                 acoustic_scale);

			double like;
			string words_wspecifier;
			string alignment_wspecifier;
			Int32VectorWriter words_writer(words_wspecifier);
			Int32VectorWriter alignment_writer(alignment_wspecifier);

            timer t4;
			if (DecodeUtteranceLatticeFaster(
                  decoder, gmm_decodable, trans_model, word_syms, utt,
                  acoustic_scale, determinize, allow_partial, &alignment_writer,
                  &words_writer, &compact_lattice_writer, &lattice_writer,
                  &like)) {
					cout << "succeed!" << endl;
				} else cout << utt << "decode failed!" << endl;
			cout << "DecodeUtteranceLatticeFaster use timer" << t4.elapsed() << "s" << endl;

			delete decode_fst; // delete this only after decoder goes out of scope.
			delete word_syms;
		} 
		}catch (const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true;
	}
	
	bool MfccExtractFeature::LatticeToPost(const string &read_name, const string &write_name)
	{
		try{
			BaseFloat acoustic_scale = 0.083,lm_scale = 1.0;
			string lats_rspecifier = read_name;
			string posteriors_wspecifier = write_name;
			
			SequentialLatticeReader lattice_reader(lats_rspecifier);

			PosteriorWriter posterior_writer(posteriors_wspecifier);
			
			string key = lattice_reader.Key();
			Lattice lat = lattice_reader.Value();
			lattice_reader.FreeCurrent();
			if (acoustic_scale != 1.0 || lm_scale != 1.0)
				fst::ScaleLattice(fst::LatticeScale(lm_scale, acoustic_scale), &lat);

			uint64 props = lat.Properties(fst::kFstProperties, false);
			if (!(props & fst::kTopSorted)) {
				if (fst::TopSort(&lat) == false)
				KALDI_ERR << "Cycles detected in lattice.";
			}
			double lat_ac_like,lat_like;
			Posterior post;
			lat_like = kaldi::LatticeForwardBackward(lat, &post, &lat_ac_like);
			posterior_writer.Write(key, post);
			
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true;
	}
	bool MfccExtractFeature::WeightSilenceToPost(const string &trans_dir_name, const string &read_name, const string &write_name)
	{
		try{
			
			bool distribute = false;
			string silence_weight_str ="0.01";
			string silence_phones_str = "1:2:3:4:5:6:7:8:9:10";
			string model_rxfilename = trans_dir_name;
            string posteriors_rspecifier = read_name;
            string posteriors_wspecifier = write_name;

			BaseFloat silence_weight = 0.0;
			if (!ConvertStringToReal(silence_weight_str, &silence_weight))
				KALDI_ERR << "Invalid silence-weight parameter: expected float, got \""
					<< silence_weight_str << '"';
			std::vector<int32> silence_phones;
			if (!SplitStringToIntegers(silence_phones_str, ":", false, &silence_phones))
					KALDI_ERR << "Invalid silence-phones string " << silence_phones_str;
			if (silence_phones.empty())
					KALDI_WARN <<"No silence phones, this will have no effect";
			ConstIntegerSet<int32> silence_set(silence_phones);  // faster lookup.

			TransitionModel trans_model;
			ReadKaldiObject(model_rxfilename, &trans_model);

			int32 num_posteriors = 0;
			SequentialPosteriorReader posterior_reader(posteriors_rspecifier);
			PosteriorWriter posterior_writer(posteriors_wspecifier);

		for (; !posterior_reader.Done(); posterior_reader.Next()) {
				num_posteriors++;
				// Posterior is vector<vector<pair<int32, BaseFloat> > >
				Posterior post = posterior_reader.Value();
				// Posterior is vector<vector<pair<int32, BaseFloat> > >
				if (distribute)
				WeightSilencePostDistributed(trans_model, silence_set,
                                     silence_weight, &post);
				else
				WeightSilencePost(trans_model, silence_set,silence_weight, &post);

				posterior_writer.Write(posterior_reader.Key(), post);
		}
			
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true;
	}
	
	bool MfccExtractFeature::PostToGpost(const string &trans_dir_name, const string &read_name, const string &read_feature_name,const string &write_name)
	{
		try{
			bool binary = true;
			BaseFloat rand_prune = 0.0;
			string model_filename = trans_dir_name+"/final.alimdl";
			string feature_rspecifier = read_feature_name;
            string posteriors_rspecifier = read_name;
			string gpost_wspecifier = write_name;
			
			AmDiagGmm am_gmm;
			TransitionModel trans_model;
			{
				bool binary;
				Input ki(model_filename, &binary);
				trans_model.Read(ki.Stream(), binary);
				am_gmm.Read(ki.Stream(), binary);
			}

			double tot_like = 0.0;
			double tot_t = 0.0;

			SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
			RandomAccessPosteriorReader posteriors_reader(posteriors_rspecifier);

			GaussPostWriter gpost_writer(gpost_wspecifier);
			
			int32 num_done = 0, num_no_posterior = 0, num_other_error = 0;
			string key = feature_reader.Key();
			
			if (!posteriors_reader.HasKey(key)) {
					num_no_posterior++;
			} else {
			const Matrix<BaseFloat> &mat = feature_reader.Value();
			const Posterior &posterior = posteriors_reader.Value(key);
			GaussPost gpost(posterior.size());

			if (posterior.size() != mat.NumRows()) {
				KALDI_WARN << "Posterior vector has wrong size "<< (posterior.size()) << " vs. "<< (mat.NumRows());
				num_other_error++;
			}
			num_done++;
			BaseFloat tot_like_this_file = 0.0, tot_weight = 0.0;
			
			Posterior pdf_posterior;
			ConvertPosteriorToPdfs(trans_model, posterior, &pdf_posterior);
			for (size_t i = 0; i < posterior.size(); i++) {
				gpost[i].reserve(pdf_posterior[i].size());
				for (size_t j = 0; j < pdf_posterior[i].size(); j++) {
					int32 pdf_id = pdf_posterior[i][j].first;
					BaseFloat weight = pdf_posterior[i][j].second;
					const DiagGmm &gmm = am_gmm.GetPdf(pdf_id);
					Vector<BaseFloat> this_post_vec;
					BaseFloat like = gmm.ComponentPosteriors(mat.Row(i), &this_post_vec);
					this_post_vec.Scale(weight);
					if (rand_prune > 0.0)
					for (int32 k = 0; k < this_post_vec.Dim(); k++)
						this_post_vec(k) = RandPrune(this_post_vec(k),rand_prune);
					if (!this_post_vec.IsZero())
						gpost[i].push_back(std::make_pair(pdf_id, this_post_vec));
					tot_like_this_file += like * weight;
					tot_weight += weight;
				}
			}
			KALDI_VLOG(1) << "Average like for this file is "
                      << (tot_like_this_file/tot_weight) << " over "
                      << tot_weight <<" frames.";
				tot_like += tot_like_this_file;
				tot_t += tot_weight;
				gpost_writer.Write(key, gpost);
			}
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true; 
	}
	
	bool MfccExtractFeature::EstFmllrGpost(const string &trans_dir_name, const string &read_name, const string &read_feature_name, const string &write_name)
	{
		try{
			FmllrOptions fmllr_opts;
			
			//string spk2utt_rspecifier = "ark:"+transform_name+"/spk2utt";
			string spk2utt_rspecifier;
			string
			model_rxfilename = trans_dir_name+"/final.mdl",
			feature_rspecifier = read_feature_name,
			gpost_rspecifier = read_name,
			trans_wspecifier = write_name;
			
			TransitionModel trans_model;
			AmDiagGmm am_gmm;
			{
				bool binary;
				Input ki(model_rxfilename, &binary);
				trans_model.Read(ki.Stream(), binary);
				am_gmm.Read(ki.Stream(), binary);
			}
			
			RandomAccessGaussPostReader gpost_reader(gpost_rspecifier);
			double tot_impr = 0.0, tot_t = 0.0;
			BaseFloatMatrixWriter transform_writer(trans_wspecifier);
			int32 num_done = 0, num_no_gpost = 0, num_other_error = 0;
			if (spk2utt_rspecifier != "") {  // per-speaker adaptation
				SequentialTokenVectorReader spk2utt_reader(spk2utt_rspecifier);
				RandomAccessBaseFloatMatrixReader feature_reader(feature_rspecifier);
				for (; !spk2utt_reader.Done(); spk2utt_reader.Next()) {
				FmllrDiagGmmAccs spk_stats(am_gmm.Dim());
				string spk = spk2utt_reader.Key();
				const vector<string> &uttlist = spk2utt_reader.Value();
				for (size_t i = 0; i < uttlist.size(); i++) {
						std::string utt = uttlist[i];
					if (!feature_reader.HasKey(utt)) {
					KALDI_WARN << "Did not find features for utterance " << utt;
					num_other_error++;
					continue;
					}
					if (!gpost_reader.HasKey(utt)) {
						KALDI_WARN << "Did not find posteriors for utterance " << utt;
						num_no_gpost++;
						continue;
					}
					const Matrix<BaseFloat> &feats = feature_reader.Value(utt);
					const GaussPost &gpost = gpost_reader.Value(utt);
					if (static_cast<int32>(gpost.size()) != feats.NumRows()) {
						KALDI_WARN << "GaussPost vector has wrong size " << (gpost.size())
						<< " vs. " << (feats.NumRows());
						num_other_error++;
						continue;
					}
					AccumulateForUtterance(feats, gpost, trans_model, am_gmm, &spk_stats);
					num_done++;
				}  // end looping over all utterances of the current speaker

				BaseFloat impr, spk_tot_t;
				{  // Compute the transform and write it out.
				Matrix<BaseFloat> transform(am_gmm.Dim(), am_gmm.Dim()+1);
				transform.SetUnit();
				spk_stats.Update(fmllr_opts, &transform, &impr, &spk_tot_t);
				transform_writer.Write(spk, transform);
				}
				}  // end looping over speakers
			} else {  // per-utterance adaptation
				SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
				for (; !feature_reader.Done(); feature_reader.Next()) {
				string utt = feature_reader.Key();
				if (!gpost_reader.HasKey(utt)) {
					KALDI_WARN << "Did not find gposts for utterance "
						<< utt;
					num_no_gpost++;
					continue;
				}
				const Matrix<BaseFloat> &feats = feature_reader.Value();
				const GaussPost &gpost = gpost_reader.Value(utt);
				if (static_cast<int32>(gpost.size()) != feats.NumRows()) {
					KALDI_WARN << "GaussPost has wrong size " << (gpost.size())
						<< " vs. " << (feats.NumRows());
					num_other_error++;
					continue;
				}
				num_done++;

				FmllrDiagGmmAccs spk_stats(am_gmm.Dim());

				AccumulateForUtterance(feats, gpost, trans_model, am_gmm,
                               &spk_stats);

				BaseFloat impr, utt_tot_t;
				{  // Compute the transform and write it out.
					Matrix<BaseFloat> transform(am_gmm.Dim(), am_gmm.Dim()+1);
					transform.SetUnit();
					spk_stats.Update(fmllr_opts, &transform, &impr, &utt_tot_t);
					transform_writer.Write(utt, transform);
				}
				}
			}
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true;
	}
	
	void MfccExtractFeature::AccumulateForUtterance(const Matrix<BaseFloat> &feats,const GaussPost &gpost,const TransitionModel &trans_model,const AmDiagGmm &am_gmm,FmllrDiagGmmAccs *spk_stats)
	{
		for (size_t i = 0; i < gpost.size(); i++) {
			for (size_t j = 0; j < gpost[i].size(); j++) {
				int32 pdf_id = gpost[i][j].first;
				const Vector<BaseFloat> & posterior(gpost[i][j].second);
				spk_stats->AccumulateFromPosteriors(am_gmm.GetPdf(pdf_id),
                                          feats.Row(i), posterior);
			}
		}						
	}
	
	void MfccExtractFeature::AccumulateForUtterance(const Matrix<BaseFloat> &feats,
                            const Posterior &post,
                            const TransitionModel &trans_model,
                            const AmDiagGmm &am_gmm,
                            FmllrDiagGmmAccs *spk_stats) {
		Posterior pdf_post;
		ConvertPosteriorToPdfs(trans_model, post, &pdf_post);
		for (size_t i = 0; i < post.size(); i++) {
			for (size_t j = 0; j < pdf_post[i].size(); j++) {
				int32 pdf_id = pdf_post[i][j].first;
				spk_stats->AccumulateForGmm(am_gmm.GetPdf(pdf_id),
                                  feats.Row(i),
                                  pdf_post[i][j].second);
			}
		}
	}
	
	bool MfccExtractFeature::LatticeDeterMinize(const string &read_name, const string &write_name)
	{
		try{
			BaseFloat acoustic_scale = 0.083;
			BaseFloat beam = 4.0;
			bool minimize = false;
			fst::DeterminizeLatticePrunedOptions opts;
			opts.max_mem = 50000000;
			opts.max_loop = 0; // was 500000;
			std::string lats_rspecifier = read_name,
			lats_wspecifier = write_name;
			
			SequentialLatticeReader lat_reader(lats_rspecifier);

			// Write as compact lattice.
			CompactLatticeWriter compact_lat_writer(lats_wspecifier);
			int32 n_done = 0, n_warn = 0;

			// depth stats (for diagnostics).
			double sum_depth_in = 0.0,
			sum_depth_out = 0.0, sum_t = 0.0;
			if (acoustic_scale == 0.0)
				KALDI_ERR << "Do not use a zero acoustic scale (cannot be inverted)";

			for (; !lat_reader.Done(); lat_reader.Next()) {
				std::string key = lat_reader.Key();
				Lattice lat = lat_reader.Value();

				KALDI_VLOG(2) << "Processing lattice " << key;

				Invert(&lat); // so word labels are on the input side.
				lat_reader.FreeCurrent();
				fst::ScaleLattice(fst::AcousticLatticeScale(acoustic_scale), &lat);
				if (!TopSort(&lat)) {
					KALDI_WARN << "Could not topologically sort lattice: this probably means it"
					" has bad properties e.g. epsilon cycles.  Your LM or lexicon might "
					"be broken, e.g. LM with epsilon cycles or lexicon with empty words.";
				}
				fst::ArcSort(&lat, fst::ILabelCompare<LatticeArc>());
				CompactLattice det_clat;
				if (!DeterminizeLatticePruned(lat, beam, &det_clat, opts)) {
				KALDI_WARN << "For key " << key << ", determinization did not succeed"
					"(partial output will be pruned tighter than the specified beam.)";
					n_warn++;
				}
				fst::Connect(&det_clat);
				if (det_clat.NumStates() == 0) {
					KALDI_WARN << "For key " << key << ", determinized and trimmed lattice "
					"was empty.";
					n_warn++;
				}
				if (minimize) {
					PushCompactLatticeStrings(&det_clat);
					PushCompactLatticeWeights(&det_clat);
					MinimizeCompactLattice(&det_clat);
				}

				int32 t;
				TopSortCompactLatticeIfNeeded(&det_clat);
				double depth = CompactLatticeDepth(det_clat, &t);
				sum_depth_in += lat.NumStates();
				sum_depth_out += depth * t;
				sum_t += t;

				fst::ScaleLattice(fst::AcousticLatticeScale(1.0/acoustic_scale), &det_clat);
				compact_lat_writer.Write(key, det_clat);
				n_done++;
			}	
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true; 
	}
	
	bool MfccExtractFeature::EstFmllr(const string &trans_dir_name, const string &read_name, const string &read_feature_name, const string &write_name)
	{
		try{
			FmllrOptions fmllr_opts;
			string spk2utt_rspecifier;
		
			string
				model_rxfilename = trans_dir_name+"/final.mdl",
				feature_rspecifier = read_feature_name,
				post_rspecifier = read_name,
				trans_wspecifier = write_name;
			
			TransitionModel trans_model;
			AmDiagGmm am_gmm;
			{
				bool binary;
				Input ki(model_rxfilename, &binary);
				trans_model.Read(ki.Stream(), binary);
				am_gmm.Read(ki.Stream(), binary);
			}
			
			RandomAccessPosteriorReader post_reader(post_rspecifier);

			double tot_impr = 0.0, tot_t = 0.0;

			BaseFloatMatrixWriter transform_writer(trans_wspecifier);

			int32 num_done = 0, num_no_post = 0, num_other_error = 0;
			if (spk2utt_rspecifier != "") {  // per-speaker adaptation
				SequentialTokenVectorReader spk2utt_reader(spk2utt_rspecifier);
				RandomAccessBaseFloatMatrixReader feature_reader(feature_rspecifier);

				for (; !spk2utt_reader.Done(); spk2utt_reader.Next()) {
					FmllrDiagGmmAccs spk_stats(am_gmm.Dim(), fmllr_opts);
					string spk = spk2utt_reader.Key();
					const vector<string> &uttlist = spk2utt_reader.Value();
					for (size_t i = 0; i < uttlist.size(); i++) {
						std::string utt = uttlist[i];
						if (!feature_reader.HasKey(utt)) {
							KALDI_WARN << "Did not find features for utterance " << utt;
							num_other_error++;
							continue;
						}
					if (!post_reader.HasKey(utt)) {
						KALDI_WARN << "Did not find posteriors for utterance " << utt;
						num_no_post++;
						continue;
					}
					const Matrix<BaseFloat> &feats = feature_reader.Value(utt);
					const Posterior &post = post_reader.Value(utt);
					if (static_cast<int32>(post.size()) != feats.NumRows()) {
					KALDI_WARN << "Posterior vector has wrong size " << (post.size())
                       << " vs. " << (feats.NumRows());
						num_other_error++;
						continue;
					}

					AccumulateForUtterance(feats, post, trans_model, am_gmm, &spk_stats);

					num_done++;
					}  // end looping over all utterances of the current speaker

					BaseFloat impr, spk_tot_t;
					{  // Compute the transform and write it out.
						Matrix<BaseFloat> transform(am_gmm.Dim(), am_gmm.Dim()+1);
						transform.SetUnit();
						spk_stats.Update(fmllr_opts, &transform, &impr, &spk_tot_t);
						transform_writer.Write(spk, transform);
					}
					KALDI_LOG << "For speaker " << spk << ", auxf-impr from fMLLR is "
						<< (impr/spk_tot_t) << ", over " << spk_tot_t << " frames.";
						tot_impr += impr;
						tot_t += spk_tot_t;
				}  // end looping over speakers
			} else {  // per-utterance adaptation
				SequentialBaseFloatMatrixReader feature_reader(feature_rspecifier);
				for (; !feature_reader.Done(); feature_reader.Next()) {
				string utt = feature_reader.Key();
				if (!post_reader.HasKey(utt)) {
				KALDI_WARN << "Did not find posts for utterance "
                     << utt;
					num_no_post++;
					continue;
				}
				const Matrix<BaseFloat> &feats = feature_reader.Value();
				const Posterior &post = post_reader.Value(utt);
				if (static_cast<int32>(post.size()) != feats.NumRows()) {
					KALDI_WARN << "Posterior has wrong size " << (post.size())
					<< " vs. " << (feats.NumRows());
					num_other_error++;
				continue;
				}
				num_done++;

				FmllrDiagGmmAccs spk_stats(am_gmm.Dim(), fmllr_opts);

				AccumulateForUtterance(feats, post, trans_model, am_gmm,
								&spk_stats);

				BaseFloat impr, utt_tot_t;
				{  // Compute the transform and write it out.
					Matrix<BaseFloat> transform(am_gmm.Dim(), am_gmm.Dim()+1);
					transform.SetUnit();
					spk_stats.Update(fmllr_opts, &transform, &impr, &utt_tot_t);
					transform_writer.Write(utt, transform);
				}
				KALDI_LOG << "For utterance " << utt << ", auxf-impr from fMLLR is "
					<< (impr/utt_tot_t) << ", over " << utt_tot_t << " frames.";
					tot_impr += impr;
					tot_t += utt_tot_t;
				}
			}
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true; 	
	}
	
	bool MfccExtractFeature::ComposeTransform(const string &read_name, const string &read_pre_name, const string &write_name)
	{
		try{
			bool b_is_affine = true;
			bool binary = true;
			std::string utt2spk_rspecifier;
			
			std::string transform_a_fn = read_name;
			std::string transform_b_fn = read_pre_name;
			std::string transform_c_fn = write_name;
			
			bool a_is_rspecifier =
				(ClassifyRspecifier(transform_a_fn, NULL, NULL)
					!= kNoRspecifier),
				b_is_rspecifier =
				(ClassifyRspecifier(transform_b_fn, NULL, NULL)
					!= kNoRspecifier),
				c_is_wspecifier =
				(ClassifyWspecifier(transform_c_fn, NULL, NULL, NULL)
					!= kNoWspecifier);

			RandomAccessTokenReader utt2spk_reader;
			if (utt2spk_rspecifier != "") {
				if (!(a_is_rspecifier && b_is_rspecifier))
					KALDI_ERR << "Error: utt2spk option provided compose transforms but "
					"at least one of the inputs is a global transform.";
				if (!utt2spk_reader.Open(utt2spk_rspecifier))
					KALDI_ERR << "Error upening utt2spk map from "
					<< utt2spk_rspecifier;
			}


			if ( (a_is_rspecifier || b_is_rspecifier) !=  c_is_wspecifier)
					KALDI_ERR << "Formats of the input and output rspecifiers/rxfilenames do "
					"not match (if either a or b is an rspecifier, then the output must "
					"be a wspecifier.";

			if (a_is_rspecifier || b_is_rspecifier) {
				BaseFloatMatrixWriter c_writer(transform_c_fn);
				if (a_is_rspecifier) {
					SequentialBaseFloatMatrixReader a_reader(transform_a_fn);
					if (b_is_rspecifier) {  // both are rspecifiers.
						RandomAccessBaseFloatMatrixReader b_reader(transform_b_fn);
						for (;!a_reader.Done(); a_reader.Next()) {
							if (utt2spk_rspecifier != "") {  // assume a is per-utt, b is per-spk.
								std::string utt = a_reader.Key();
									if (!utt2spk_reader.HasKey(utt)) {
										KALDI_WARN << "No speaker provided for utterance " << utt
										<< " (perhaps you wrongly provided utt2spk option to "
										" compose-transforms?)";
										continue;
									}
									std::string spk = utt2spk_reader.Value(utt);
									if (!b_reader.HasKey(spk)) {
										KALDI_WARN << "Second table does not have key " << spk;
										continue;
									}
									Matrix<BaseFloat> c;
									if (!ComposeTransforms(a_reader.Value(), b_reader.Value(a_reader.Key()),
											b_is_affine, &c))
											continue;  // warning will have been printed already.
									c_writer.Write(utt, c);
									} else {  // Normal case: either both per-utterance or both per-speaker.
										if (!b_reader.HasKey(a_reader.Key())) {
											KALDI_WARN << "Second table does not have key " << a_reader.Key();
										} else {
											Matrix<BaseFloat> c;
											if (!ComposeTransforms(a_reader.Value(), b_reader.Value(a_reader.Key()),
											b_is_affine, &c))
												continue;  // warning will have been printed already.
												c_writer.Write(a_reader.Key(), c);
										}
									}
							}
						} else {  // a is rspecifier,  b is rxfilename
							Matrix<BaseFloat> b;
							ReadKaldiObject(transform_b_fn, &b);
							for (;!a_reader.Done(); a_reader.Next()) {
							Matrix<BaseFloat> c;
							if (!ComposeTransforms(a_reader.Value(), b,
                                  b_is_affine, &c))
							continue;  // warning will have been printed already.
							c_writer.Write(a_reader.Key(), c);
							}
						}
					} else {
						Matrix<BaseFloat> a;
						ReadKaldiObject(transform_a_fn, &a);
						SequentialBaseFloatMatrixReader b_reader(transform_b_fn);
						for (; !b_reader.Done(); b_reader.Next()) {
						Matrix<BaseFloat> c;
						if (!ComposeTransforms(a, b_reader.Value(),
                                b_is_affine, &c))
							continue;  // warning will have been printed already.
							c_writer.Write(b_reader.Key(), c);
						}
					}
				} else {  // all are just {rx, wx}filenames.
					Matrix<BaseFloat> a;
					ReadKaldiObject(transform_a_fn, &a);
					Matrix<BaseFloat> b;
					ReadKaldiObject(transform_b_fn, &b);
					Matrix<BaseFloat> c;
					if (!b_is_affine && a.NumRows() == a.NumCols()+1 && a.NumRows() == b.NumRows()
						&& a.NumCols() == b.NumCols())
						KALDI_WARN << "It looks like you are trying to compose two affine transforms"
							<< ", but you omitted the --b-is-affine option.";
					if (!ComposeTransforms(a, b, b_is_affine, &c)) exit (1);

				WriteKaldiObject(c, transform_c_fn, binary);
			}
		}catch(const exception &e){
			cerr << e.what();
			//should throw;
		}
		return true; 
	}

	bool MfccExtractFeature::FasterGetLat(const string &transform_name, const string &read_feature_name, const string &write_name,bool deter_lat)
	{
	    try {
            bool allow_partial = true;
            BaseFloat acoustic_scale = 0.083333;
            string word_syms_filename = transform_name+"/words.txt";
            FasterDecoderOptions decoder_opts;

            decoder_opts.beam = 13.0;
            decoder_opts.max_active = 7000;

            string    model_rxfilename = transform_name+"/final.mdl";
            string    fst_rxfilename = transform_name+"/HCLG.fst";
            string    words_wspecifier = "";
            string    alignment_wspecifier = "";
            string    lattice_wspecifier = write_name;

            TransitionModel trans_model;
            AmDiagGmm am_gmm;
            {
                bool binary;
                Input ki(model_rxfilename, &binary);
                trans_model.Read(ki.Stream(), binary);
                am_gmm.Read(ki.Stream(), binary);
            }

            Int32VectorWriter words_writer(words_wspecifier);
            Int32VectorWriter alignment_writer(alignment_wspecifier);
            CompactLatticeWriter clat_writer(lattice_wspecifier);

            fst::SymbolTable *word_syms = NULL;
            if (word_syms_filename != "")
                if (!(word_syms = fst::SymbolTable::ReadText(word_syms_filename)))
                    KALDI_ERR << "Could not read symbol table from file "
                        << word_syms_filename;
            SequentialBaseFloatMatrixReader feature_reader(read_feature_name);
            Fst<StdArc> *decode_fst = fst::ReadFstKaldiGeneric(fst_rxfilename);

            BaseFloat tot_like = 0.0;
            kaldi::int64 frame_count = 0;
            int num_success = 0, num_fail = 0;
            FasterDecoder decoder(*decode_fst, decoder_opts);


            for (; !feature_reader.Done(); feature_reader.Next()) {
                 string key = feature_reader.Key();
                 Matrix<BaseFloat> features (feature_reader.Value());
                 feature_reader.FreeCurrent();
                 if (features.NumRows() == 0) {
                    KALDI_WARN << "Zero-length utterance: " << key;
                    num_fail++;
                    continue;
                    }

                DecodableAmDiagGmmScaled gmm_decodable(am_gmm, trans_model, features,
                                             acoustic_scale);
                decoder.Decode(&gmm_decodable);

                fst::VectorFst<LatticeArc> decoded;  // linear FST.

                if ( (allow_partial || decoder.ReachedFinal())
                            && decoder.GetBestPath(&decoded) ) {
                            if (!decoder.ReachedFinal())
                                KALDI_WARN << "Decoder did not reach end-state, "
                                << "outputting partial traceback since --allow-partial=true";
                    num_success++;
                    std::vector<int32> alignment;
                    std::vector<int32> words;
                    LatticeWeight weight;
                    frame_count += features.NumRows();

                    GetLinearSymbolSequence(decoded, &alignment, &words, &weight);

                    if (lattice_wspecifier != "") {
          // We'll write the lattice without acoustic scaling.
                    if (acoustic_scale != 0.0)
                        fst::ScaleLattice(fst::AcousticLatticeScale(1.0 / acoustic_scale),
                              &decoded);
                    fst::VectorFst<CompactLatticeArc> clat;
                    ConvertLattice(decoded, &clat, true);
                    clat_writer.Write(key, clat);
                 }

            if (word_syms != NULL) {
                std::cerr << key << ' ';
          for (size_t i = 0; i < words.size(); i++) {
            std::string s = word_syms->Find(words[i]);
            if (s == "")
              KALDI_ERR << "Word-id " << words[i] <<" not in symbol table.";
            std::cerr << s << ' ';
          }
          std::cerr << '\n';
        }
      } else {
        num_fail++;
        KALDI_WARN << "Did not successfully decode utterance " << key
                   << ", len = " << features.NumRows();
      }
    }
    delete word_syms;
    delete decode_fst;


  } catch(const std::exception &e) {
    std::cerr << e.what();
    return false;
  }
    return true;
	}
}//end namespace gop


