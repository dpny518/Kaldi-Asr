// fluency/mfcc.h

// Copyright 2017.11.24  Zhang weiyu
// Version 1.0

//特征提取的顺序：
/*
1.~/kaldi/src/featbin/compute-mfcc-feats --use-energy=false scp:wav.scp ark:feats.ark
2.~/kaldi/src/featbin/copy-feats ark:feats.ark ark,scp:feat.ark,feat.scp

3.~/kaldi/src/featbin/compute-cmvn-stats scp:feat.scp ark,scp:cmvn.ark,cmvn.scp

4.~/kaldi/src/featbin/apply-cmvn scp:cmvn.scp scp:feat.scp ark:applycmvn.ark
5.~/kaldi/src/featbin/splice-feats --left-context=3 --right-context=3 ark:applycmvn.ark ark:splice.ark
6.~/kaldi/src/featbin/transform-feats final.mat ark:splice.ark ark:transform.ark

//剩下的都是去掉的，实用chain模型解码
7.~/kaldi/src/gmmbin/gmm-latgen-faster --max-active=7000 --beam=13.0 --lattice-beam=6.0 --acoustic-scale=0.083333 --allow-partial=true --word-symbol-table=words.txt final.alimdl HCLG.fst 'ark,s,cs:transform.ark' ark:lat
9.~/kaldi/src/latbin/lattice-to-post --acoustic-scale=0.083333 ark:lat ark:1.post
10.~/kaldi/src/bin/weight-silence-post 0.01 1:2:3:4:5:6:7:8:9:10 final.alimdl ark:1.post ark:nosil.post
11.~/kaldi/src/gmmbin/gmm-post-to-gpost final.alimdl ark,s,cs:transform.ark ark:nosil.post ark:1.gpost
12.~/kaldi/src/gmmbin/gmm-est-fmllr-gpost --fmllr-update-type=full final.alimdl 'ark,s,cs:transform.ark' ark,s,cs:1.gpost ark:pre_trans.1
13.~/kaldi/src/featbin/transform-feats ark:pre_trans.1 ark:transform.ark ark:passone.ark
14.~/kaldi/src/gmmbin/gmm-latgen-faster --max-active=7000 --beam=13.0 --lattice-beam=6.0 --acoustic-scale=0.083333 --allow-partial=true --word-symbol-table=words.txt final.alimdl HCLG.fst 'ark,s,cs:passone.ark' ark:lat2
15.~/kaldi/src/latbin/lattice-determinize-pruned --acoustic-scale=0.083333 --beam=4.0 ark:lat2 ark:det
16.~/kaldi/src/latbin/lattice-to-post --acoustic-scale=0.083333 ark:det ark:2.post
17.~/kaldi/src/bin/weight-silence-post 0.01 1:2:3:4:5:6:7:8:9:10 final.alimdl ark:2.post ark:nosil2.ark
18.~/kaldi/src/gmmbin/gmm-est-fmllr --fmllr-update-type=full final.alimdl ark:passone.ark ark,s,cs:nosil2.ark ark:tmp_trans.1
19.~/kaldi/src/featbin/compose-transforms --b-is-affine=true   ark:tmp_trans.1 ark:pre_trans.1  ark:trans.1
20.~/kaldi/src/featbin/transform-feats ark:trans.1 ark:transform.ark ark:final.ark
21.~/kaldi/src/nnet2bin/nnet-latgen-faster --minimize=false --max-active=7000 --min-active=200 --beam=15.0 --lattice-beam=8.0 --acoustic-scale=0.1 --allow-partial=true --word-symbol-table=words.txt final.mdl HCLG.fst ark,s,cs:final.ark "ark:|gzip -c > lat.11.gz"
*/

#ifndef KALDI_MFCC_H_
#define KALDI_MFCC_H_ 1


namespace kaldi {

class MfccFeature {
public:
  explicit MfccFeature() { m_strUtt = ""; }
  ~MfccFeature() {}

  //计算特征向量（39维）给解码调用，依次调用下面的函数
  //每个函数的输入和输出需要考虑
  bool ComputeMfccFeature();

  //根据传入的wav文件名，生成对应的scp文件
  void Init(std::string &wav_in_filename);

 //对应compute-mfcc-feats 源程序
  bool  ComputeMfccFeats(Matrix<BaseFloat>& kaldi_features);

 //对应copy-feats源程序
  bool CopyFeats(Matrix<BaseFloat>& kaldi_features);

 //对应compute-cmvn-stats源程序
  bool  ComputeCMVNStats(Matrix<BaseFloat>& kaldi_features, Matrix<double>& cmvn_features);

 //对应apply-cmvn源程序
  bool  ApplyCMVN(Matrix<BaseFloat>& kaldi_features, Matrix<double>& cmvn_features);

 //对应splice-feats源程序
  bool  SpliceFeats(Matrix<BaseFloat>& kaldi_features, Matrix<BaseFloat>& splice_feature);

 //对应transform源程序
  bool  Transform(Matrix<BaseFloat>& splice_feature);

 //对应add-deltas源程序
  bool  AddDeltas(std::string& input_Ark, std::string& output_Ark);

  //通过特征矩阵获取对数能量
  bool  GetLogEnergyFromFeature(Matrix<BaseFloat>& features);

  //保存计算的对数能量，对应每一帧
  std::vector<BaseFloat>     m_vecEnergy;

  //回收函数（变量重新初始化）
  void  Recovery();

private:
  std::string m_strUtt;   //utter id
};

}  // End namespace kaldi

#endif  // KALDI_MFCC_H_
