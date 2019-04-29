#!/bin/bash

# Begin configuration section.
stage=0
nj=20
cmd=run.pl
frames_per_chunk=20
extra_left_context_initial=0
min_active=200
max_active=7000
beam=15.0
lattice_beam=6.0
acwt=0.1   
post_decode_acwt=1.0  
per_utt=false
online=true  
do_endpointing=false
do_speex_compressing=false
scoring_opts=
skip_scoring=false
silence_weight=1.0  # set this to a value less than 1 (e.g. 0) to enable silence weighting.
max_state_duration=40 # This only has an effect if you are doing silence
iter=final
online_config=
# End configuration section.


echo "Usage: ./zhang_decode.sh <data-dir> "
echo " Options:"
echo " example:  ./zhang_decode.sh /data/zhangweiyu/timit  "

[ -f ./path.sh ] && . ./path.sh; # source the path.
. parse_options.sh || exit 1;

graph_dir=exp/tri6b/graph_tgsmall
conf_dir=config
data=/data/zhangweiyu/timit
sdata=$data/split$nj;

if [ "$online_config" == "" ]; then
  online_config=$conf_dir/online.conf;
fi

mkdir -p $data/decode_log
mkdir -p $data/lat

sort $data/wav.scp

cp $data/wav.scp $data/spk2utt

utils/spk2utt_to_utt2spk.pl $data/spk2utt > $data/utt2spk

[[ -d $sdata && $data/feats.scp -ot $sdata ]] || split_data.sh $data $nj || exit 1;
echo $nj > num_jobs

for f in $online_config $conf_dir/${iter}.mdl \
    $conf_dir/HCLG.fst $conf_dir/words.txt $data/wav.scp; do
  if [ ! -f $f ]; then
    echo "$0: no such file $f"
    exit 1;
  fi
done

if ! $per_utt; then
  spk2utt_rspecifier="ark:$sdata/JOB/spk2utt"
else
  mkdir -p per_utt
  for j in $(seq $nj); do
    awk '{print $1, $1}' <$sdata/$j/utt2spk >per_utt/utt2spk.$j || exit 1;
  done
  spk2utt_rspecifier="ark:per_utt/utt2spk.JOB"
fi

if [ -f $data/segments ]; then
  wav_rspecifier="ark,s,cs:extract-segments scp,p:$sdata/JOB/wav.scp $sdata/JOB/segments ark:- |"
else
  wav_rspecifier="ark,s,cs:wav-copy scp,p:$sdata/JOB/wav.scp ark:- |"
fi
if $do_speex_compressing; then
  wav_rspecifier="$wav_rspecifier compress-uncompress-speex ark:- ark:- |"
fi
if $do_endpointing; then
  wav_rspecifier="$wav_rspecifier extend-wav-with-silence ark:- ark:- |"
fi

if [ "$silence_weight" != "1.0" ]; then
  silphones=$(cat $graphdir/phones/silence.csl) || exit 1
  silence_weighting_opts="--ivector-silence-weighting.max-state-duration=$max_state_duration --ivector-silence-weighting.silence_phones=$silphones --ivector-silence-weighting.silence-weight=$silence_weight"
else
  silence_weighting_opts=
fi


if [ "$post_decode_acwt" == 1.0 ]; then
  lat_wspecifier="ark:|gzip -c >$data/lat/lat.JOB.gz"
else
  lat_wspecifier="ark:|lattice-scale --acoustic-scale=$post_decode_acwt ark:- ark:- | gzip -c >$data/lat/lat.JOB.gz"
fi


if [ -f frame_subsampling_factor ]; then
  # e.g. for 'chain' systems
  frame_subsampling_opt="--frame-subsampling-factor=$(catframe_subsampling_factor)"
fi

echo "start decode"

if [ $stage -le 0 ]; then
  $cmd JOB=1:$nj $data/decode_log/decode.JOB.log \
    online2-wav-nnet3-latgen-faster $silence_weighting_opts --do-endpointing=$do_endpointing \
    --frames-per-chunk=$frames_per_chunk \
    --extra-left-context-initial=$extra_left_context_initial \
    --online=$online \
       $frame_subsampling_opt \
     --config=$online_config \
     --min-active=$min_active --max-active=$max_active --beam=$beam --lattice-beam=$lattice_beam \
     --acoustic-scale=$acwt --word-symbol-table=$conf_dir/words.txt \
     $conf_dir/${iter}.mdl $conf_dir/HCLG.fst $spk2utt_rspecifier "$wav_rspecifier" \
      "$lat_wspecifier" || exit 1;
fi

echo "decode finished!!!"

decode_mbr=true
word_ins_penalty=0.0,0.5,1.0
min_lmwt=7
max_lmwt=17
#end configuration section.

[ -f ./path.sh ] && . ./path.sh
. parse_options.sh || exit 1;

symtab=$conf_dir/words.txt

for f in $symtab $data/lat/lat.1.gz $data/text; do
  [ ! -f $f ] && echo "score.sh: no such file $f" && exit 1;
done

mkdir -p $data/scoring/log

cat $data/text | sed 's:<NOISE>::g' | sed 's:<SPOKEN_NOISE>::g' > $data/scoring/test_filt.txt

for wip in $(echo $word_ins_penalty | sed 's/,/ /g'); do
  $cmd LMWT=$min_lmwt:$max_lmwt $data/scoring/log/best_path.LMWT.$wip.log \
    lattice-scale --inv-acoustic-scale=LMWT "ark:gunzip -c $data/lat/lat.*.gz|" ark:- \| \
    lattice-add-penalty --word-ins-penalty=$wip ark:- ark:- \| \
    lattice-best-path --word-symbol-table=$symtab \
      ark:- ark,t:$data/scoring/LMWT.$wip.tra || exit 1;
done

# Note: the double level of quoting for the sed command
for wip in $(echo $word_ins_penalty | sed 's/,/ /g'); do
  $cmd LMWT=$min_lmwt:$max_lmwt $data/scoring/log/score.LMWT.$wip.log \
    cat $data/scoring/LMWT.$wip.tra \| \
    utils/int2sym.pl -f 2- $symtab \| sed 's:\<UNK\>::g' \| \
    compute-wer --text --mode=present \
    ark:$data/scoring/test_filt.txt  ark,p:- ">&" $data/wer_LMWT_$wip || exit 1;
done

echo "test finished !!!!"

exit 0;
