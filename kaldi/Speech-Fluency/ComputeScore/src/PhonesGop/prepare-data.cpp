
#include <sstream>
#include <fstream>
#include <string>
#include "prepare-data.h"
#include "lat/word-align-lattice-lexicon.h"

namespace gop{

	using namespace std;

		void PrepareData::WordTextToInt(const string &word_filename, const string &text_filename, map< string,vector<int> >&utt_map)
		{
			//default init
			map<string,int> word_map;
			char line[1024] = {0};
			string str;
			int str_int;
			map<string,int>::iterator iter;

			//read word_file
			ifstream fword(word_filename, ios::in);
			while (fword.getline(line, sizeof(line)))
			{
				stringstream word(line);
				word >> str;
				word >> str_int;
				word_map[str] = str_int;
			}	
			
			//read text_file
			ifstream ftext(text_filename, ios::in); 
			while (ftext.getline(line, sizeof(line)))
			{
				string utt;
				vector<int> text_int;

				stringstream text(line);
				text >> utt;

				while (text >> str)
				{
					iter = word_map.find(str);
					if (iter != word_map.end())
					text_int.push_back(word_map[str]);	
				}
				utt_map[utt] = text_int;	
			}
		}

		bool PrepareData::WavDataPrepare(const string &wav_file, string &wav_scp_filename)
		{
			//wav_file---->wav_scp
			ofstream fwrite;
			string wav_scp;
			string utt;
			size_t found = 0;
			//flac
			found = wav_file.find("flac");
			if (found != string::npos)
			{
			    utt = wav_file.substr(0,found-1);
			    wav_scp = utt + " flac -c -d -s ./" + wav_file + "|";
					
			}
			else if (wav_file.find("wav") != string::npos)
			{
				found = wav_file.find("wav");
				utt = wav_file.substr(0,found-1);
				wav_scp = utt + " " + wav_file;
			}
			wav_scp_filename = utt + ".scp";
			fwrite.open(wav_scp_filename, ios::out);

			if (!fwrite.is_open())
			{
			    cout << "open wav_scp_file failed!" << endl;
			    return false;
			}
			fwrite << wav_scp << endl;
			fwrite.close();
			return true;
		}

		bool PrepareData::FormatPhonesFileData(const string &align_lexfile)
		{
			vector< vector<int32> > lexicon;
			bool binary_in;
			Input ki(align_lexfile, &binary_in);
			if (!ReadLexiconForWordAlign(ki.Stream(), &lexicon))
			{
				//因为是初始化 所以直接打印错误，外边会退出
				cout << "read failed!" << endl;
				return false;
			}
			for (size_t i = 0; i < lexicon.size(); i++)
			{
				vector<int32> &lexicon_entry = lexicon[i];
				MakeLexiconMap(lexicon_entry);
				MakeNumPhonesMap(lexicon_entry);	
			}
			FinalDealMap();
			return true;
		}

		void PrepareData::MakeLexiconMap(const vector<int32> &lexicon_entry)
		{
			int32 word = lexicon_entry[0];
			int num_phones = static_cast<int32>(lexicon_entry.size()-2);
			vector<int32> phones;
			if(num_phones > 0)
				phones.reserve(num_phones);
			for (int32 n = 0; n< num_phones; n++)
			{
				phones.push_back(lexicon_entry[n+2]);
			}
			lexicon_map_[phones].push_back(word);
			word_lexicon_map_.insert(make_pair(word,phones));
		}

		void PrepareData::MakeNumPhonesMap(const vector<int32> &lexicon_entry)
		{
			int32 num_phones = static_cast<int32>(lexicon_entry.size()) - 2;
			vector<int32> nums_phones;

			int word = lexicon_entry[0];
			num_phones_map_[word].push_back(num_phones);
		}

		void PrepareData::FinalDealMap()
		{
			for (LexiconMap::iterator iter = lexicon_map_.begin();
					iter != lexicon_map_.end(); ++iter)
			{
				vector<int32> &words = iter->second;
				SortAndUniq(&words);
			}

			for (NumPhonesMap::iterator iter = num_phones_map_.begin();
					iter != num_phones_map_.end(); ++iter)
			{
				vector<int32> &nums = iter->second;
				SortAndUniq(&nums);
			}
		}
}

