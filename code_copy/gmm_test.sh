#!/bin/bash

mfccdir=mfcc
stage=1

root_dir=$1

. ./cmd.sh
. ./path.sh
. parse_options.sh
set -e

if [ $stage -le 2 ]; then
  # decode using the tri4b model with pronunciation and silence probabilities
  (
    for test in chatbox_100; do
      steps/make_mfcc.sh  test_set/$test \
                     $root_dir/$test $root_dir/$test
      steps/compute_cmvn_stats.sh \
           $root_dir/$test $root_dir/$test $root_dir/$test

      cp $root_dir/$test/wav.scp $root_dir/$test/spk2utt
      utils/spk2utt_to_utt2spk.pl $root_dir/$test/spk2utt > $root_dir/$test/utt2spk

      decode_fmllr.sh --nj 4 --cmd "$decode_cmd" \
                            exp/tri4b/graph_tgsmall $root_dir/$test \
                            exp/tri4b/decode_tgsmall_$test
	  echo "tri4b decode finished!!!!"
      
      steps/lmrescore.sh --cmd "$decode_cmd" data/lang_test_{tgsmall,tgmed} \
                         $root_dir/$test exp/tri4b/decode_{tgsmall,tgmed}_$test
      steps/lmrescore_const_arpa.sh \
        --cmd "$decode_cmd" data/lang_test_{tgsmall,tglarge} \
        $root_dir/$test exp/tri4b/decode_{tgsmall,tglarge}_$test
      steps/lmrescore_const_arpa.sh \
        --cmd "$decode_cmd" data/lang_test_{tgsmall,fglarge} \
        $root_dir/$test exp/tri4b/decode_{tgsmall,fglarge}_$test
      echo "tri4b score finished!!!"


      decode_fmllr.sh --nj 4 --cmd "$decode_cmd" \
                            exp/tri5b/graph_tgsmall $root_dir/$test \
                            exp/tri5b/decode_tgsmall_$test
      echo "tri5b decode finished!!! " 
      steps/lmrescore.sh --cmd "$decode_cmd" data/lang_test_{tgsmall,tgmed} \
                         $root_dir/$test exp/tri5b/decode_{tgsmall,tgmed}_$test
      steps/lmrescore_const_arpa.sh \
        --cmd "$decode_cmd" data/lang_test_{tgsmall,tglarge} \
        $root_dir/$test exp/tri5b/decode_{tgsmall,tglarge}_$test
      steps/lmrescore_const_arpa.sh \
        --cmd "$decode_cmd" data/lang_test_{tgsmall,fglarge} \
        $root_dir/$test exp/tri5b/decode_{tgsmall,fglarge}_$test
      echo "tri5b score finished!!!! "    

    done
  )&
fi

echo "done!!!!"


