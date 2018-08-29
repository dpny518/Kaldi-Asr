// dataExtract.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

#ifndef KALDI_DATA_EXTRACT_H_
#define KALDI_DATA_EXTRACT__H_ 1

#include <vector>
#include <string>

using namespace kaldi;

namespace fluency {
//解码后，根据对齐信息计算停顿和重音保存到相应类中，供评分类调用

class DataExtract {
public:
  explicit DataExtract();
  ~DataExtract();

  //根据对应的transition-id的开始和结尾位置,  从ark文件中获得矩阵 并保存到文件中
    bool    MatrixExtractWithTransitionIndex(std::string& strIndex,std::string& strEndWord, std::string& strStartWord,
                                                                     int32 front_phone_start_transition_index,
                                                                     int32 front_phone_end_transition_index,
                                                                     int32 next_phone_start_transition_index,
                                                                     int32  next_phone_end_transition_index);

    //将连读取出的矩阵保存到一个文件中
    bool   SaveMatrixToTxtFile();



private:

    //通过提取的矩阵，调用python得到分类结果
    //void  GetClassifyResultWithPython(double**& array,int rows, int cols);
    //void  init_numpy();

    std::vector<Matrix<BaseFloat>>  m_vecMatrix;
    std::vector<std::string>  m_vecSpeaker;

    std::string    word_one;
    std::string    word_two;
};

}  // End namespace kaldi

#endif  // KALDI_DATA_EXTRACT_H_
