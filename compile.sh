#! /usr/bin/env bash

set -euo pipefail

HERE=$(readlink -f $(dirname -- "$BASH_SOURCE"))

# set compiler flags

FLAGS_STANDARD='-std=c++23'

FLAGS_STRICT='-Werror -Wextra -Wall -pedantic -Wfatal-errors -Wshadow -Wconversion -fsanitize=undefined'
# `-fsanitize=undefined` will detect undefined behaviour at runtime (example: signed overflow)

FLAGS_OPTIMISATION='-Ofast -march=native'
# architecture specific optimisations

FLAGS_MISC=''
# can't have both `-static` and `-fsanitize=undefined`

FLAGS_LIB='-lgmp'

FLAGS="$FLAGS_STANDARD $FLAGS_STRICT $FLAGS_OPTIMISATION $FLAGS_MISC $FLAGS_LIB"

# set define containing current git commit

DEFINE_COMMIT_ID=$(git -C "$HERE" rev-parse HEAD)

# compile

g++ $FLAGS -DCOMMIT_ID="\"$DEFINE_COMMIT_ID\"" -o "$HERE/arithmetic-code" "$HERE/arithmetic-code.cpp"
