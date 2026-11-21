#!/usr/bin/env bash

set -e

d1="${1:-8}"
d2="${2:-8}"

if (( $# > 2 )); then
    echo "Usage: bash runGame.sh [depth] [true|false]" >&2
    exit 1
fi

gui_args=(--duel "$d1" "$d2")


cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
gcc -std=c11 -O3 -march=native connect4_engine.c -o connect4_engine
exec python3 connect4_gui.py "${gui_args[@]}"
