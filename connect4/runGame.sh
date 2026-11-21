#!/usr/bin/env bash
set -e

depth="${1:-8}"
use_alpha_beta="${2:-true}"

if (( $# > 2 )); then
    echo "Usage: bash runGame.sh [depth] [true|false]" >&2
    exit 1
fi

gui_args=(--depth "$depth")

case "$use_alpha_beta" in
    true|1) ;;
    false|0) gui_args+=(--no-alpha-beta) ;;
    *)
        echo "useAlphaBeta must be true, false, 1, or 0." >&2
        exit 1
        ;;
esac

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
gcc -std=c11 -O3 -march=native MinMaxEngineNoDuelMode.c -o connect4_engine
exec python3 connect4_guiNoDuelMode.py "${gui_args[@]}"