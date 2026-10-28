#!/usr/bin/env bash
# Put this script beside astar.c, astar2.c, bfs.c, dfs.c, iterativeDFS.c,
# puzzle_viewer.py, and the TreeFoof/TreePriorityQueue/TreeQueue/TreeStack folders.
# Example: ./run_puzzle.sh astar 125340678

set -euo pipefail

usage() {
    cat <<'HELP'
Usage: ./run_puzzle.sh ALGORITHM INITIAL_STATE

Algorithms: ASTAR, ASTAR2, BFS, DFS, IDFS (case-insensitive)
INITIAL_STATE: nine digits containing 0 through 8 exactly once.

Examples:
  ./run_puzzle.sh astar 125340678
  ./run_puzzle.sh astar2 123456780
  ./run_puzzle.sh bfs 125340678
  ./run_puzzle.sh dfs 125340678
  ./run_puzzle.sh idfs 123456780

All five programs are compiled on each run. The selected program receives
the board through stdin, then puzzle_viewer.py opens the saved solution.
HELP
}

fail() {
    printf 'Error: %s\n' "$*" >&2
    exit 1
}

if [[ $# == 1 && ( $1 == -h || $1 == --help ) ]]; then
    usage
    exit 0
fi

if [[ $# != 2 ]]; then
    usage >&2
    exit 2
fi

algorithm=${1^^}
initial_state=$2

case "$algorithm" in
    ASTAR|ASTAR2|BFS|DFS|IDFS) program=$algorithm ;;
    IDDFS|ITERATIVEDFS) program=IDFS ;;
    *) fail "Unknown algorithm '$1'. Choose ASTAR, ASTAR2, BFS, DFS, or IDFS." ;;
esac

[[ $initial_state =~ ^[0-8]{9}$ ]] ||
    fail "Enter exactly nine digits from 0 to 8, with no spaces."

# Nine positions and all nine different digits present means no duplicates.
for digit in {0..8}; do
    [[ $initial_state == *"$digit"* ]] ||
        fail "Each digit from 0 through 8 must appear exactly once."
done

# Resolve source files and puzzle_states.txt relative to this script,
# even if the script was called from a different working directory.
cd -- "$(dirname -- "${BASH_SOURCE[0]}")"

command -v gcc >/dev/null 2>&1 || fail "gcc is not installed."
command -v python3 >/dev/null 2>&1 || fail "python3 is not installed."

for file in astar.c astar2.c bfs.c dfs.c iterativeDFS.c \
    TreeFoof/tree.c TreePriorityQueue/treePriorityQueue.c \
    TreeQueue/treeQueue.c TreeStack/stacktree.c puzzle_viewer.py; do
    [[ -f $file ]] || fail "Missing file: $file. Put run_puzzle.sh in your project folder."
done

printf 'Compiling all five algorithms...\n'
gcc -std=c11 -Wall -Wextra astar.c TreeFoof/tree.c TreePriorityQueue/treePriorityQueue.c -o ASTAR
gcc -std=c11 -Wall -Wextra astar2.c TreeFoof/tree.c TreePriorityQueue/treePriorityQueue.c -o ASTAR2 -lm
gcc -std=c11 -Wall -Wextra bfs.c TreeFoof/tree.c TreeQueue/treeQueue.c -o BFS
gcc -std=c11 -Wall -Wextra dfs.c TreeFoof/tree.c TreeStack/stacktree.c -o DFS
gcc -std=c11 -Wall -Wextra iterativeDFS.c TreeFoof/tree.c TreeStack/stacktree.c -o IDFS

# Move the previous solution aside so a solver that produces no new solution
# cannot accidentally open the old route. Restore it if this run fails.
[[ ! -d puzzle_states.txt ]] || fail "puzzle_states.txt is a directory."
backup_dir=$(mktemp -d)
solution_ready=false
run_started=false

cleanup() {
    if [[ $run_started == true && $solution_ready == false ]]; then
        if [[ -e $backup_dir/puzzle_states.txt || -L $backup_dir/puzzle_states.txt ]]; then
            mv -f -- "$backup_dir/puzzle_states.txt" puzzle_states.txt
        else
            rm -f -- puzzle_states.txt
        fi
    fi
    rm -rf -- "$backup_dir"
}

trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

if [[ -e puzzle_states.txt || -L puzzle_states.txt ]]; then
    mv -- puzzle_states.txt "$backup_dir/puzzle_states.txt"
fi
run_started=true

printf '\nRunning %s with initial state %s...\n' "$program" "$initial_state"

# A here-string supplies the board and a newline to the C scanf("%9s", ...).
if "./$program" <<< "$initial_state"; then
    :
else
    solver_status=$?
    printf '\n%s failed with exit status %d.\n' "$program" "$solver_status" >&2
    exit "$solver_status"
fi

# Some existing solvers only print a message when the start is already the
# project's fixed goal. Supply its zero-move route for the viewer in that case.
if [[ ! -s puzzle_states.txt && $initial_state == 012345678 ]]; then
    printf 'Move Number 0 is %s\n' "$initial_state" > puzzle_states.txt
fi

[[ -s puzzle_states.txt ]] ||
    fail "$program did not save a solution. The viewer will not open an old result."

solution_ready=true
printf '\nOpening puzzle viewer...\n'
python3 puzzle_viewer.py puzzle_states.txt
