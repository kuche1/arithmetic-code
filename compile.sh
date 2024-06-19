#! /usr/bin/env bash

set -euo pipefail

HERE=$(readlink -f $(dirname -- "$BASH_SOURCE"))

FLAGS_STANDARD='-std=c++23'

FLAGS_STRICT='-Werror -Wextra -Wall -pedantic -Wfatal-errors -Wshadow -Wconversion -fsanitize=undefined'
# `-fsanitize=undefined` will detect undefined behaviour at runtime (example: signed overflow)

FLAGS_OPTIMISATION='-Ofast -march=native'
# architecture specific optimisations

FLAGS_MISC=''
# can't have both `-static` and `-fsanitize=undefined`

FLAGS_LIB='-lgmp'

FLAGS="$FLAGS_STANDARD $FLAGS_STRICT $FLAGS_OPTIMISATION $FLAGS_MISC $FLAGS_LIB"

g++ $FLAGS -o "$HERE/compressor" "$HERE/compressor.cpp"
