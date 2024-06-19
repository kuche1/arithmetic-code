#! /usr/bin/env bash

set -euo pipefail

HERE=$(readlink -f $(dirname -- "$BASH_SOURCE"))

# FILE=urandom-40960
# FILE=pmv-10240  #  10k, comp+dec 0m0,236s,  comp ratio 0.3904296875
# FILE=pmv-20480  #  20k, comp+dec 0m0,458s,  comp ratio 0.285888671875
  FILE=pmv-40960  #  40k, comp+dec 0m1,009s,  comp ratio 0.227294921875
# FILE=pmv-81920  #  80k, comp+dec 0m2,601s,  comp ratio 0.1971923828125
# FILE=pmv-163840 # 160k, comp+dec 0m7,663s,  comp ratio 0.1824951171875
# FILE=pmv-327680 # 320k, comp+dec 1m28,304s, comp ratio 0.25567626953125

clear

echo "~~~ compiling..."
echo

"$HERE/compile.sh"

echo
echo "~~~ encoding..."
echo

time "$HERE/arithmetic-code" enc "$HERE/data/$FILE" "$HERE/data/$FILE-compressed"

echo
echo "~~~ decoding..."
echo

time "$HERE/arithmetic-code" dec "$HERE/data/$FILE-compressed" "$HERE/data/$FILE-regenerated"

echo

du -b "$HERE/data/$FILE"
du -b "$HERE/data/$FILE-compressed"
du -b "$HERE/data/$FILE-regenerated"

echo

sha512sum "$HERE/data/$FILE" "$HERE/data/$FILE-regenerated"
diff "$HERE/data/$FILE" "$HERE/data/$FILE-regenerated"
