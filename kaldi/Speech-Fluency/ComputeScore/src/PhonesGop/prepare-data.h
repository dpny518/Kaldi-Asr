
#ifndef _PREPARE_DATA_
#define _PREPARE_DATA_

#include <iostream>
#include <string>
#include <map>
#include <vector>
#include "base/kaldi-common.h"
#include "util/common-utils.h"

namespace gop{
	using namespace std;
	using namespace kaldi;

class PrepareData{
	public:
		void WordTextToInt(const string &word_filename, const string &text_filename, map< string, vector<int> > &utt_map);
		bool WavDataPrepare(const string &wav_file, string &wav_scp);

		bool FormatPhonesFileData(const string &ali_filename);
		//phone1 phone2..=> word1,word2,word3...
		typedef unordered_map<vector<int32>, vector<int32>, VectorHasher<int32> > LexiconMap;
		//word => (num1 num2 num3)
		typedef unordered_map<int32, vector<int32> > NumPhonesMap;

		LexiconMap lexicon_map_;
		NumPhonesMap num_phones_map_;
		multimap<int32, vector<int32>> word_lexicon_map_;
	protected:
		void MakeNumPhonesMap(const vector<int32> &lexicon);
		void MakeLexiconMap(const vector<int32> &lexicon);
		void FinalDealMap();
		
};//end PrepareData

}

#endif
