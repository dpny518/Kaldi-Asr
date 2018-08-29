/*
date:     2017-12-7
author:   dinghaojie
refer to: http://www.kaldi-asr.org
*/

#include <iostream>
#include "compute-center.h"
#include <boost/timer.hpp>
using namespace boost;

using namespace gop;
using namespace compute;
int main(int argc, char *argv[])
{
	
    timer t;
	string wav_file = string(argv[1]); 

    string wav_text = "";
    for(int i=2; i<argc;i++)
    {
        wav_text += argv[i];
        wav_text += " ";
    }
    cout << argc << " " << wav_text << endl;
	ComputeCenter comp;
	cout << "init use time:" << t.elapsed()<< "s" << endl;

	timer t2;
	comp.ComputeScore(wav_file,wav_text);
    cout << "compute use time:" << t2.elapsed()<< "s" << endl;

	return 0;
}
