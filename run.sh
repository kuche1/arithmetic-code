#! /usr/bin/env bash

set -euo pipefail

HERE=$(readlink -f $(dirname -- "$BASH_SOURCE"))

# FILE=urandom-40960
# FILE=pmv-40960
# FILE=pmv-81920
FILE=pmv-163840 # even at this size (80k) the compression is super fast (the decompression is a bit slower, but still acceptable)

clear

echo "compiling..."

"$HERE/compile.sh"

echo "running..."

time "$HERE/compressor" "$HERE/data/$FILE" "$HERE/data/$FILE-compressed" "$HERE/data/$FILE-regenerated"

echo

du -b "$HERE/data/$FILE"
du -b "$HERE/data/$FILE-compressed"
du -b "$HERE/data/$FILE-regenerated"

echo

sha512sum "$HERE/data/$FILE" "$HERE/data/$FILE-regenerated"
diff "$HERE/data/$FILE" "$HERE/data/$FILE-regenerated"
