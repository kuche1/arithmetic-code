#! /usr/bin/env bash

set -euo pipefail

HERE=$(readlink -f $(dirname -- "$BASH_SOURCE"))

### before f5d68989ee8a45117b58cc7be63517c358d13da0

# FILE=urandom-40960
# FILE=pmv-10240  #  10k, enc+dec 0m 0,236s, comp ratio 0.3904296875
# FILE=pmv-20480  #  20k, enc+dec 0m 0,458s, comp ratio 0.285888671875
# FILE=pmv-40960  #  40k, enc+dec 0m 1,009s, comp ratio 0.227294921875
# FILE=pmv-81920  #  80k, enc+dec 0m 2,601s, comp ratio 0.1971923828125
# FILE=pmv-163840 # 160k, enc+dec 0m 7,663s, comp ratio 0.1824951171875
# FILE=pmv-327680 # 320k, enc+dec 1m28,304s, comp ratio 0.25567626953125

### f5d68989ee8a45117b58cc7be63517c358d13da0 (encoding and decoding times are now measured separately)

# FILE=urandom-40960
# FILE=pmv-10240  #  10k, encode 0m 0,020s, decode 0m 0,219s
# FILE=pmv-20480  #  20k, encode 0m 0,052s, decode 0m 0,410s
# FILE=pmv-40960  #  40k, encode 0m 0,154s, decode 0m 0,860s
# FILE=pmv-81920  #  80k, encode 0m 0,510s, decode 0m 2,090s
# FILE=pmv-163840 # 160k, encode 0m 1,922s, decode 0m 5,872s
# FILE=pmv-327680 # 320k, encode 0m11,963s, decode 1m17,613s

### 95f60eb2efdbe1904e590c20ce7cd53a76d0b935 (decoder now detects if symbol count is 0, and skips calculations if so)

# FILE=urandom-40960
# FILE=pmv-10240  #  10k, decode 0m0,170s
# FILE=pmv-20480  #  20k, decode 0m0,321s
  FILE=pmv-40960  #  40k, decode 0m0,712s
# FILE=pmv-81920  #  80k, decode 0m1,768s
# FILE=pmv-163840 # 160k, decode 0m5,195s
# FILE=pmv-327680 # 320k, decode 1m18,695s

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
