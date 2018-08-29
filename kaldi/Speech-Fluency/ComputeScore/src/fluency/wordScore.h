
#ifndef _WORD_SCORE_
#define _WORD_SCORE_

#include <iostream>
#include <string>
#include <vector>

class WordScore{
    public:
    WordScore(){}
    ~WordScore(){}

    float EditDistance(const std::vector<std::string> &v1, const std::vector<std::string> &v2);
    //std::vector<std::string> vecReferenceTextWord;
};

#endif