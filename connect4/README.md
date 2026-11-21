# Connect Four

Play against the AI, watch two engines compete, or inspect the minimax search behind an AI move.

**Requirements** Bash, GCC, and Python 3.10+ with Tkinter. The tree-viewing commands use Brave to open the generated HTML.

Run all commands from the project’s `connect4` folder.

## Play against the AI

```bash
bash runGame.sh 8 true
```

| Argument | Example | Meaning |
| --- | --- | --- |
| 1 | `8` | Search depth in plies. One ply is one move by either player. |
| 2 | `true` | Enable alpha-beta pruning. Use `false` to disable it. |

## Watch two AIs play

```bash
bash OneVsOne.sh 8 4
```

X searches at depth `8` and moves first. O searches at depth `4`. Swap the numbers to switch which engine goes first.

## Capture and view a search tree

### 1. Build the tracing engine and start a game

```bash
gcc -std=c11 -O3 connect4_engine_forTree_Printing.c -o connect4_engine
CONNECT4_TRACE_REPLY=3 CONNECT4_TRACE_LAYERS=4 python3 connect4_gui.py --depth 4
```

| Setting | What it controls |
| --- | --- |
| `CONNECT4_TRACE_REPLY=3` | Capture the AI’s third move. The AI’s opening move is reply `1`. |
| `CONNECT4_TRACE_LAYERS=4` | Record positions through layer `4`. The current board is layer `0`. |
| `--depth 4` | Search up to four plies to choose a move. |

Make **two human moves** and wait for the AI to finish its third move. The engine writes that search to `GameTree.txt`.

> Start with four trace layers. The trace is capped at 4,000 recorded positions. When the search depth is greater than the trace depth, the viewer labels the values at the display boundary as **frontier values**.

### 2. Generate the HTML and open it in Brave

Open a second terminal in the same folder after the selected reply finishes.

```bash
python3 view_game_tree.py GameTree.txt --reply 3
brave-browser "$PWD/GameTree_reply_3.html"
```

Click **Expand one level** to reveal more of the tree, or select a node to inspect its board and score. When you capture a different AI reply, use the same reply number in `--reply` and in the HTML filename.

### If the HTML shows an older search

Check which reply and depths are recorded in the log.

```bash
grep -E '^(REPLY NUMBER|SEARCH DEPTH|TRACE LAYERS)' GameTree.txt
```

If the settings are old, rebuild the tracing engine and play until the selected AI reply finishes. Then run the viewer again and refresh the page in Brave.
