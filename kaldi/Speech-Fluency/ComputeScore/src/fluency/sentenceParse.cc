// fluency/sentenceParse.cc

// Copyright 2017.12.13  Zhang weiyu
// Version 1.0

#include "base/kaldi-common.h"
#include "util/common-utils.h"
#include "hmm/transition-model.h"
#include "decoder/decoder-wrappers.h"
#include "hmm/hmm-utils.h"
#include "feat/pitch-functions.h"
#include "feat/wave-reader.h"
#include "lat/word-align-lattice-lexicon.h"
#include "fluency/sentenceParse.h"
#include <string>
#include <algorithm>

namespace fluency {

SentenceParse::SentenceParse(SpeechPublic*& pPublic, std::vector<std::string>& vec_decode_words ,std::string&  str_sentence)
{
    m_pSpeechPublic = pPublic;

    for(int32 i=0; i<vec_decode_words.size(); i++)
    {
        m_vec_decode_words.push_back(vec_decode_words.at(i));
    }

    ParseSentenceFromFile(str_sentence);

    //add by zhangweiyu 2018.8.17 用来生成不能添加停顿的容器
    GenerateNonPause();
}

SentenceParse::~SentenceParse()
{
    /*if(m_pSpeechPublic)
    {
        delete m_pSpeechPublic;
        m_pSpeechPublic = NULL;
    }*/
}

void  SentenceParse::ParseSentenceFromFile(std::string&  str_sentence)
{

    SplitWordFromSentence(str_sentence);

}

void  SentenceParse::SplitWordFromSentence(std::string std_words)
{
    if(!std_words.empty())
    {
            //去掉标点符号
            std_words.erase(std::remove(std_words.begin(), std_words.end(), '.'), std_words.end());
            std_words.erase(std::remove(std_words.begin(), std_words.end(), ','), std_words.end());
            std_words.erase(std::remove(std_words.begin(), std_words.end(), '?'), std_words.end());
            std_words.erase(std::remove(std_words.begin(), std_words.end(), ':'), std_words.end());
            std_words.erase(std::remove(std_words.begin(), std_words.end(), ';'), std_words.end());

            //得到所有的单词信息
            ParseWordsFromSentence(std_words);

            //读取停顿信息
            ParsePauseInfoFromSentence(std_words);

            //读取重音信息
            ParseStressInfoFromSentence(std_words);

            //根据得到的单词和解码出来的单词做强制对齐
            ForceAlignment();
    }
}

void  SentenceParse::ParseWordsFromSentence(std::string  str_sentence)
{
    if(!str_sentence.empty())
    {
            //去掉停顿符号  重音符号
            std::string  str_one = "[-]";
            std::string  str_two = "[s]";
            std::string  str_three = "[/s]";

            DelSubStrFromSentence(str_sentence,str_one);
            DelSubStrFromSentence(str_sentence,str_two);
            DelSubStrFromSentence(str_sentence,str_three);

            //将所有的单词变成大写
            transform(str_sentence.begin(), str_sentence.end(), str_sentence.begin(), ::toupper);

            //根据空格将所有的单词保存起来
            std::string str_symbol = " ";

            std::string::size_type pos1, pos2;
            pos2 = str_sentence.find(str_symbol);
            pos1 = 0;

            while(std::string::npos != pos2)
            {
                std::string add_word = str_sentence.substr(pos1, pos2-pos1);

                if(!add_word.empty())
                    m_vec_word_str.push_back(add_word);

                pos1 = pos2 + str_symbol.size();
                pos2 = str_sentence.find(str_symbol, pos1);

            }
            if(pos1 != str_sentence.length())
            {
                std::string add_word = str_sentence.substr(pos1);
                if(!add_word.empty())
                    m_vec_word_str.push_back(add_word);
            }

    }
}

void  SentenceParse::ParsePauseInfoFromSentence(std::string  str_sentence)
{
    //去掉所有的重音标记符号
    std::string  str_pause = "[-]";
    std::string  str__stress_one = "[s]";
    std::string  str_stress_two = "[/s]";

    DelSubStrFromSentence(str_sentence,str__stress_one);
    DelSubStrFromSentence(str_sentence,str_stress_two);


    string strsplit;
    //将字符串读到input中 
    std::stringstream str_input(str_sentence);

    int word_index = 0;
 
    while(str_input>>strsplit)
    {
        //判断当前字符串是否包含重音符号
 
        if((strsplit.find(str_pause) != std::string::npos))
        {
            m_vecWord_pause_index.push_back(word_index);
        }

        word_index = word_index + 1;
    }


    /*
    //根据空格将所有的单词保存起来
    std::string str_symbol = " ";

    std::string::size_type pos1, pos2;
    pos2 = str_sentence.find(str_symbol);
    pos1 = 0;

    int32  pause_index = 0;

    while(std::string::npos != pos2)
    {
         std::string strsplit = str_sentence.substr(pos1, pos2-pos1);

         //判断当前字符串是否包含重音符号
        if(strsplit.find(str_pause) < strsplit.length())
        {
            m_vecWord_pause_index.push_back(pause_index - 1);   //保存前一个词的索引
        }
         pos1 = pos2 + str_symbol.size();
         pos2 = str_sentence.find(str_symbol, pos1);

         pause_index++;
    }

    if(pos1 != str_sentence.length())
    {
        std::string strsplit = str_sentence.substr(pos1);

        if(strsplit.find(str_pause) < strsplit.length() )
        {
            m_vecWord_pause_index.push_back(pause_index-1);
        }
    }
    */

}

void  SentenceParse::ParseStressInfoFromSentence(std::string  str_sentence)
{
    //去掉所有的停顿符号
    std::string  str_pause = "[-]";
    std::string  str_stress_one = "[s]";
    std::string  str_stress_two = "[/s]";

    DelSubStrFromSentence(str_sentence,str_pause);

    std::cout<<"去掉停顿符号的文本是："<<str_sentence<<std::endl;

    //根据空格将所有的单词保存起来
    //std::string str_symbol = " ";

    string strsplit;
    //将字符串读到input中 
    std::stringstream str_input(str_sentence);

    int word_index = 0;
 
    while(str_input>>strsplit)
    {
        //判断当前字符串是否包含重音符号
 
        if((strsplit.find(str_stress_one) != std::string::npos) && (strsplit.find(str_stress_two) != std::string::npos) )
        {
            m_vecWord_stress_indexs.push_back(word_index);
        }

        word_index = word_index + 1;
    }

    /*

    std::string::size_type pos1, pos2;
    pos2 = str_sentence.find(str_symbol);
    pos1 = 0;

    int32  stress_index = 0;

    while(std::string::npos != pos2)
    {
         //m_vec_word_str.push_back(str_sentence.substr(pos1, pos2-pos1));
         std::string strsplit = str_sentence.substr(pos1, pos2-pos1);

         //判断当前字符串是否包含重音符号
        if((strsplit.find(str__stress_one) < strsplit.length()) && (strsplit.find(str_stress_two) < strsplit.length()) )
        {
            m_vecWord_stress_indexs.push_back(stress_index);
        }

         pos1 = pos2 + str_symbol.size();
         pos2 = str_sentence.find(str_symbol, pos1);

         stress_index++;
    }

    if(pos1 != str_sentence.length())
    {
        std::string strsplit = str_sentence.substr(pos1);

        if((strsplit.find(str__stress_one) < strsplit.length()) && (strsplit.find(str_stress_two) < strsplit.length()) )
        {
            m_vecWord_stress_indexs.push_back(stress_index);
        }
    }
    */
}

void  SentenceParse::ForceAlignment()
{
    //动态规划  求解LCS序列
    int32 m = m_vec_word_str.size();                             //行是读取序列
    int32 n = m_vec_decode_words.size();                            //列是解码序列

    //std::cout << "ForceAlignment(): " << std::endl;
    //for(auto a: m_vec_word_str) std::cout << a << " ";
    //std::cout << std::endl;

    //for(auto a: m_vec_decode_words) std::cout << a << " ";
    //std::cout << std::endl;

    int32 **c=new int32*[m+1];       //动态分配二维数组
    for(int32 i=0;i<=m;i++)
        c[i]=new int32[n+1];

    int32 **b=new int32*[m];            //动态分配二维数组
    for(int32 j=0;j<m;j++)
        b[j]=new int32[n];

    int32 lcs_len = Length_LCS(c,b,m,n);

    Print_LCS(b,m,n);

    SplitDecodeWord();

    //释放二维数组
    for(int32 i=0; i<m+1; i++) delete[] c[i];
    delete[] c;
    for(int32 i=0; i<m; i++) delete[] b[i];
    delete[] b;
}

void  SentenceParse::SplitDecodeWord()
{
    // 将解码单词强制对齐
    if((m_vecWordIndex.size() == 0) || (m_vecDecodeWordIndex.size() == 0)||(m_vecWordIndex.size() != m_vecDecodeWordIndex.size()))
        return;

    int32  compare_index = 0;
    int32  compare_decode_index = 0;

    for(int32 i=0; i<m_vecWordIndex.size()+1; i++)
    {
            int32  countOne = 0;
            int32  countTwo = 0;
            if(i == m_vecWordIndex.size())
            {
                    //处理结尾
                    countOne = m_vec_word_str.size() - m_vecWordIndex.at(i-1) - 1;
                    countTwo = m_vec_decode_words.size() - m_vecDecodeWordIndex.at(i-1) - 1;
            }
            else
            {
                countOne = m_vecWordIndex.at(i) - compare_index;
                countTwo = m_vecDecodeWordIndex.at(i) - compare_decode_index;
            }

            if(countOne != 0)
            {
                if(countOne==countTwo )  //1对1匹配
                {
                      for(int32 j=0;j<countTwo;j++)
                      {
                            std::vector<std::string>  vecStr;
                             vecStr.push_back(m_vec_decode_words.at(compare_decode_index + j));
                             vec_lcs_ali.push_back(vecStr);
                      }
                }
                else if(countOne > countTwo)
                {
                    for(int32 j=0; j<countTwo;j++)
                    {
                        std::vector<std::string>  vecStr;
                        vecStr.push_back(m_vec_decode_words.at(compare_decode_index + j));
                        vec_lcs_ali.push_back(vecStr);
                    }

                    //添加空字符串
                    for(int32 j =0; j<(countOne-countTwo);j++)
                    {
                        std::vector<std::string>  vecStr;
                        std::string strEmpty = " ";
                        vecStr.push_back(strEmpty);
                        vec_lcs_ali.push_back(vecStr);
                    }
                }
                else
                {
                        int32 rescount = countTwo%countOne;
                        int32 selcount = (countTwo-rescount) / countOne;

                        int  start_index = compare_decode_index;

                        int cycle_count = 0;

                        //计算平分的次数
                        for(int32 j=0; j<countOne; j++)
                        {
                            cycle_count = cycle_count + 1;
                            if(cycle_count <= 1)
                            {
                                std::vector<std::string>  vecStr;
                                for(int32 k=0; k<(rescount+selcount); k++)
                                    vecStr.push_back(m_vec_decode_words.at(start_index + k));
                                vec_lcs_ali.push_back(vecStr);

                                start_index = rescount + selcount;
                            }
                            else
                            {
                                std::vector<std::string>  vecStr;
                                for(int32 k=0; k<selcount; k++)
                                    vecStr.push_back(m_vec_decode_words.at(start_index + k));
                                vec_lcs_ali.push_back(vecStr);

                                start_index = start_index + selcount;
                            }
                     }
                }
            }
            //添加匹配单词
            if(i<m_vecWordIndex.size())     //防止越界
            {
                std::vector<std::string>  vecStr;
                vecStr.push_back(m_vec_decode_words.at(m_vecDecodeWordIndex.at(i)));
                vec_lcs_ali.push_back(vecStr);
            }

            compare_index = compare_index+countOne + 1;
            compare_decode_index = compare_decode_index + countTwo + 1;
    }
    /*std::cout << "vec_lcs_ali: " << std::endl;
    for(auto s:vec_lcs_ali)
        for(auto a:s)
            std::cout << a << " ";
        std::cout << std::endl;
    std::cout << std::endl;*/
}

int32  SentenceParse::Length_LCS(int32 **&count,int32 **&dir,int32 row,int32 col)
{
    /*处理特殊的0行和0列*/
    // c 数组保存个数  b数组保存方向
    for(int i=0;i<=row;i++)
        count[i][0]=0;
    for(int j=0;j<=col;j++)
        count[0][j]=0;

    /*处理其他行和列*/
    for(int i=1;i<=row;i++)
    {
         for(int j=1;j<=col;j++)
         {
             if(!m_vec_word_str.at(i-1).compare(m_vec_decode_words.at(j-1)))
             {
                 count[i][j]=count[i-1][j-1]+1;
                 dir[i-1][j-1]=0;
             }
             else
             {
                 if(count[i-1][j] >= count[i][j-1])
                 {
                     count[i][j]=count[i-1][j];
                     dir[i-1][j-1]=1;
                 }
                 else
                 {
                     count[i][j]=count[i][j-1];
                     dir[i-1][j-1]=-1;
                 }
             }
         }
    }
    return count[row][col];
}

void  SentenceParse::GenerateNonPause()
{
    //根据停顿位置找出不该停顿的位置
    if(m_vecWord_pause_index.size() > 0)
    {
        for(int i=1; i<(m_vec_word_str.size()-1); i++)
        {
            if(!is_element_in_vector(m_vecWord_pause_index, i))
            {
                m_vecWord_notpossible_pause_index.push_back(i);
            }
        }
    }
}

void  SentenceParse::Print_LCS(int32 **dir, int32 row, int32 col)   //输出LCS序列
{
    //使用递归得到对应的最大匹配序列
    if( (row==0) ||  (col==0) )
        return ;
    if(dir[row-1][col-1]==0)
    {
        Print_LCS(dir,row-1,col-1);

        m_vecWordIndex.push_back(row-1);
        m_vecDecodeWordIndex.push_back(col-1);
    }
    else if(dir[row-1][col-1]==1)
    {
        Print_LCS(dir,row-1,col);
    }
    else
        Print_LCS(dir,row,col-1);
}

void  SentenceParse::DelSubStrFromSentence(std::string& str_sentence, std::string& substr)
{
    //int pos,l=substr.length();  //获得子字符串长度

    //while(1)
    //{
        //这个循环保证最后str中不再有子串
    //    if((pos = str_sentence.find(substr.c_str())) < 0 )        //若未找到子串则结束
    //        break;

   //     str_sentence.erase(pos, l);        //将找到的子串删除　　
   //     s1.insert(pos,s3);
   // }
    std::string  replace_str = std::string(" ");
    std::string::size_type pos = 0;
    std::string::size_type a = substr.size();
    std::string::size_type b = replace_str.size();

    while ((pos = str_sentence.find(substr,pos)) != std::string::npos)
    {
        str_sentence.replace(pos, a, replace_str);
        pos += b;
    }
}

void  SentenceParse::PrintParseWords()
{
    if(m_vec_word_str.size() > 0)
    {
        std::cout<<"打印解析的单词信息！！" <<std::endl;
        std::string strPrint = std::string("");
        for(int32 i=0; i<m_vec_word_str.size(); i++)
        {
            strPrint = strPrint + m_vec_word_str.at(i) + std::string(" ");
        }
        std::cout<< strPrint <<std::endl;
    }

    if(m_vecWord_stress_indexs.size() > 0)
    {
        std::cout<<"打印重音索引信息！！" <<std::endl;
        std::string strPrint = std::string("");
        for(int32 i=0; i<m_vecWord_stress_indexs.size(); i++)
        {
            strPrint = strPrint + std::to_string(m_vecWord_stress_indexs.at(i)) + std::string(" ");
        }
        std::cout<< strPrint <<std::endl;
    }

    if(m_vecWord_pause_index.size() > 0)
    {
        std::cout<<"打印重音索引信息！！" <<std::endl;
        std::string strPrint = std::string("");
        for(int32 i=0; i<m_vecWord_pause_index.size(); i++)
        {
            strPrint = strPrint + std::to_string(m_vecWord_pause_index.at(i)) + std::string(" ");
        }
        std::cout<< strPrint <<std::endl;
    }

    if(vec_lcs_ali.size() > 0)
    {
        std::cout<<"打印强制对齐的单词信息" <<std::endl;
        for(int i=0; i<vec_lcs_ali.size(); i++)
        {
            std::string  strResult = std::string("");
            for(int j=0; j<vec_lcs_ali.at(i).size(); j++)
            {
                   strResult = strResult + vec_lcs_ali.at(i).at(j) + std::string(" ");
            }
            std::cout<<"单词信息:" << strResult <<std::endl;
        }
    }

    if(m_vecWordIndex.size() > 0)
    {
        std::string  strResult = std::string("");
        for(int i = 0; i<m_vecWordIndex.size(); i++)
        {
            strResult = strResult + std::to_string(m_vecWordIndex.at(i)) + std::string(" ");
        }
        std::cout<<"打印动态规划后的匹配单词索引" << strResult <<std::endl;
    }

    if(m_vecDecodeWordIndex.size() > 0)
    {
        std::string  strResult = std::string("");
        for(int i = 0; i<m_vecDecodeWordIndex.size(); i++)
        {
            strResult = strResult + std::to_string(m_vecDecodeWordIndex.at(i)) + std::string(" ");
        }
        std::cout<<"打印动态规划后的解码单词索引" << strResult <<std::endl;
    }

}

bool SentenceParse::is_element_in_vector(std::vector<int> v,int element)
{
    std::vector<int>::iterator it;
    it=find(v.begin(),v.end(),element);
    if (it!=v.end())
        return true;
    else
        return false;
}

void  SentenceParse::Recovery()
{

    for(int i =0; i<vec_lcs_ali.size(); i++)
    {
        vec_lcs_ali.at(i).clear();
        std::vector <std::string>().swap(vec_lcs_ali.at(i));
    }

    vec_lcs_ali.clear();
    std::vector<std::vector<std::string>>().swap(vec_lcs_ali);

    m_vecWord_stress_indexs.clear();
    std::vector <int32>().swap(m_vecWord_stress_indexs);

    m_vecWord_pause_index.clear();
    std::vector <int32>().swap(m_vecWord_pause_index);

    m_vec_word_str.clear();
    std::vector <std::string>().swap(m_vec_word_str);

    m_vec_decode_words.clear();
    std::vector <std::string>().swap(m_vec_decode_words);

    m_vecWordIndex.clear();
    std::vector <int32>().swap(m_vecWordIndex);

    m_vecDecodeWordIndex.clear();
    std::vector <int32>().swap(m_vecDecodeWordIndex);

    m_vecWord_notpossible_pause_index.clear();
    std::vector <int>().swap(m_vecWord_notpossible_pause_index) ;

}

}  // End namespace kaldi
