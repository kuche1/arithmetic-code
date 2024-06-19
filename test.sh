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

# FILE=pmv-10240  #  10k, encode 0m 0,020s, decode 0m 0,219s
# FILE=pmv-20480  #  20k, encode 0m 0,052s, decode 0m 0,410s
# FILE=pmv-40960  #  40k, encode 0m 0,154s, decode 0m 0,860s
# FILE=pmv-81920  #  80k, encode 0m 0,510s, decode 0m 2,090s
# FILE=pmv-163840 # 160k, encode 0m 1,922s, decode 0m 5,872s
# FILE=pmv-327680 # 320k, encode 0m11,963s, decode 1m17,613s

### 95f60eb2efdbe1904e590c20ce7cd53a76d0b935 (decoder now detects if symbol count is 0, and skips calculations if so)

# FILE=pmv-10240  #  10k, decode 0m 0,170s
# FILE=pmv-20480  #  20k, decode 0m 0,321s
# FILE=pmv-40960  #  40k, decode 0m 0,712s
# FILE=pmv-81920  #  80k, decode 0m 1,768s
# FILE=pmv-163840 # 160k, decode 0m 5,195s
# FILE=pmv-327680 # 320k, decode 1m18,695s

### not commited since results are slower across the board (altho this might have been a random fluctuation) (decoder now uses a vector to track the remaining symbols, rather than checking the count on every iteration)

# FILE=pmv-10240  #  10k, decode 0m 0,181s
# FILE=pmv-20480  #  20k, decode 0m 0,343s
# FILE=pmv-40960  #  40k, decode 0m 0,743s
# FILE=pmv-81920  #  80k, decode 0m 1,821s
# FILE=pmv-163840 # 160k, decode 0m 5,298s
# FILE=pmv-327680 # 320k, decode 1m19,346s

### 384670361e86e7c68e2b411969841ce465a40e01 multithreading finished, blocksize is 81920

# FILE=pmv-10240  #  10k; encode 0m0,021s; decode 0m 0,171s
# FILE=pmv-20480  #  20k; encode 0m0,053s; decode 0m 0,321s
# FILE=pmv-40960  #  40k; encode 0m0,153s; decode 0m 0,700s
# FILE=pmv-81920  #  80k; encode 0m0,517s; decode 0m 1,749s
# FILE=pmv-163840 # 160k; encode 0m0,523s; decode 0m 1,844s
# FILE=pmv-327680 # 320k; encode 0m0,975s; decode 0m17,167s
# FILE=pmv-655360 # 640k; encode 0m3,076s; decode 3m43,480s; compression ratio 0.48369903564453126

### <current> reduced blocksize to 40960

FILE=pmv-655360 # 640k; encode 0m1,217s; decode 1m27,058s; compression ratio 0.4791107177734375

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
