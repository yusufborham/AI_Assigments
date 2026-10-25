#!/usr/bin/env python3
"""Animate the saved route from the C 8-puzzle solver.

Run:
    python3 puzzle_viewer.py puzzle_states.txt
    python3 puzzle_viewer.py --demo
    python3 puzzle_viewer.py puzzle_states.txt --check

Expected input (one board per line, kept as a string to preserve leading 0):
    Move Number 0 is 125340678
    Move Number 1 is 120345678
    Move Number 2 is 102345678
    Move Number 3 is 012345678

Plain nine-digit board strings are also accepted. Consecutively numbered
backward logs are automatically reversed. Every board and move is validated.
The viewer reads the route; it does not solve or change the puzzle.

Requires Python 3.9+ and Tkinter. On Ubuntu, if Tkinter is missing:
    sudo apt install python3-tk
No pip packages are required.
"""

from __future__ import annotations

import argparse
from dataclasses import dataclass
from pathlib import Path
import re
import sys
import time

try:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk
except ImportError:
    tk = None


DEMO = ("125340678", "120345678", "102345678", "012345678")
MOVE_LINE = re.compile(r"Move\s+Number\s+(\d+)\s+is\s+(\S+)", re.IGNORECASE)
TILES = set("012345678")


@dataclass(frozen=True)
class Solution:
    states: tuple[str, ...]
    name: str
    note: str = ""


def legal_move(before: str, after: str) -> bool:
    """True only when one numbered tile slides into the neighboring blank."""
    first = before.index("0")
    second = after.index("0")
    distance = abs(first // 3 - second // 3) + abs(first % 3 - second % 3)
    if distance != 1:
        return False
    swapped = list(before)
    swapped[first], swapped[second] = swapped[second], swapped[first]
    return "".join(swapped) == after


def read_solution(filename: str | Path) -> Solution:
    """Read, order, and validate the entire saved route before playback."""
    path = Path(filename).expanduser()
    records = []
    numbered = None

    for line_number, raw in enumerate(path.read_text(encoding="utf-8-sig").splitlines(), 1):
        line = raw.strip()
        if not line:
            continue
        match = MOVE_LINE.fullmatch(line)
        if match:
            label, board = int(match.group(1)), match.group(2)
            this_numbered = True
        elif re.fullmatch(r"[0-9]+", line):
            label, board = None, line
            this_numbered = False
        else:
            raise ValueError(
                f"Line {line_number}: expected 'Move Number 0 is 012345678' "
                "or a plain nine-digit state."
            )

        if len(board) != 9 or set(board) != TILES:
            raise ValueError(
                f"Line {line_number}: {board!r} is not an 8-puzzle board. "
                "Use all nine digits 0 through 8 exactly once; 0 is the blank."
            )
        if numbered is not None and numbered != this_numbered:
            raise ValueError("Use either numbered lines or plain states throughout the file.")
        numbered = this_numbered
        records.append((label, board, line_number))

    if not records:
        raise ValueError("The file contains no puzzle states.")

    note = ""
    if numbered and len(records) > 1:
        differences = [b[0] - a[0] for a, b in zip(records, records[1:])]
        if all(difference == -1 for difference in differences):
            records.reverse()
            note = "Backward log reordered to show start → finish."
        elif not all(difference == 1 for difference in differences):
            raise ValueError(
                "Move numbers must be consecutive and increase or decrease consistently. "
                "If several runs were appended, export one run using fopen(..., \"w\")."
            )

    for before, after in zip(records, records[1:]):
        if not legal_move(before[1], after[1]):
            raise ValueError(
                f"Invalid move between source lines {before[2]} and {after[2]}:\n"
                f"{before[1]} → {after[1]}\n"
                "Only one numbered tile may swap with a horizontally or vertically adjacent blank."
            )

    return Solution(tuple(record[1] for record in records), path.name, note)


def demo_solution() -> Solution:
    return Solution(DEMO, "Three-move demo", "Open puzzle_states.txt to view your own solution.")


class PuzzleViewer:
    """Tk interface: one board, timed playback, stepping, and random access."""

    BG = "#0b1221"
    PANEL = "#111d31"
    BORDER = "#273852"
    TILE = "#263d5a"
    TEXT = "#eef5ff"
    MUTED = "#a2b3cc"
    ACCENT = "#62dfc2"

    def __init__(self, root, solution: Solution):
        self.root = root
        self.solution = solution
        self.index = 0
        self.phase = 0.0
        self.playing = False
        self.timer = None
        self.last_tick = 0.0
        self.updating_slider = False

        root.title("8 Puzzle · Solution Viewer")
        root.geometry("740x790")
        root.minsize(620, 720)
        root.configure(bg=self.BG)
        root.protocol("WM_DELETE_WINDOW", self.close)

        style = ttk.Style(root)
        style.theme_use("clam")
        style.configure("TFrame", background=self.BG)
        style.configure("TLabel", background=self.BG, foreground=self.TEXT,
                        font=("DejaVu Sans", 10))
        style.configure("Muted.TLabel", foreground=self.MUTED)
        style.configure("Title.TLabel", font=("DejaVu Sans", 24, "bold"))
        style.configure("Count.TLabel", font=("DejaVu Sans", 17, "bold"))
        style.configure("TButton", font=("DejaVu Sans", 10), padding=(13, 9),
                        background=self.PANEL, foreground=self.TEXT, borderwidth=0)
        style.map("TButton", background=[("active", self.TILE)],
                  foreground=[("disabled", "#66758c")])
        style.configure("Play.TButton", background=self.ACCENT, foreground=self.BG)
        style.map("Play.TButton", background=[("active", "#8eecd6")],
                  foreground=[("disabled", "#596979")])
        style.configure("Horizontal.TScale", background=self.BG,
                        troughcolor=self.PANEL)

        body = ttk.Frame(root, padding=24)
        body.pack(fill="both", expand=True)

        header = ttk.Frame(body)
        header.pack(fill="x")
        title = ttk.Frame(header)
        title.pack(side="left")
        ttk.Label(title, text="8 PUZZLE", style="Title.TLabel").pack(anchor="w")
        ttk.Label(title, text="Your solution, one move at a time.",
                  style="Muted.TLabel").pack(anchor="w", pady=(3, 0))
        ttk.Button(header, text="Open file…", command=self.open_file).pack(side="right")
        ttk.Button(header, text="Demo", command=lambda: self.set_solution(demo_solution())).pack(
            side="right", padx=(0, 8))

        self.filename = ttk.Label(body, style="Muted.TLabel", wraplength=650)
        self.filename.pack(anchor="w", pady=(16, 12))

        self.canvas = tk.Canvas(body, width=380, height=380, bg=self.PANEL,
                                highlightthickness=1, highlightbackground=self.BORDER)
        self.canvas.pack(fill="both", expand=True)
        self.canvas.bind("<Configure>", lambda _event: self.draw_board())

        info = ttk.Frame(body)
        info.pack(fill="x", pady=(18, 5))
        self.counter = ttk.Label(info, style="Count.TLabel")
        self.counter.pack(side="left")
        self.direction = ttk.Label(info, style="Muted.TLabel")
        self.direction.pack(side="right")

        self.position = tk.DoubleVar(value=0)
        self.seek_slider = ttk.Scale(body, variable=self.position, from_=0,
                                     to=1, command=self.seek)
        self.seek_slider.pack(fill="x", pady=(4, 12))

        controls = ttk.Frame(body)
        controls.pack()
        self.first_button = ttk.Button(controls, text="First", command=lambda: self.jump(0))
        self.back_button = ttk.Button(controls, text="Back", command=lambda: self.step(-1))
        self.play_button = ttk.Button(controls, text="Play", style="Play.TButton",
                                      command=self.toggle_play, width=9)
        self.next_button = ttk.Button(controls, text="Next", command=lambda: self.step(1))
        self.last_button = ttk.Button(controls, text="Last", command=lambda: self.jump(self.last))
        for button in (self.first_button, self.back_button, self.play_button,
                       self.next_button, self.last_button):
            button.pack(side="left", padx=3)

        speed_row = ttk.Frame(body)
        speed_row.pack(fill="x", pady=(18, 0))
        ttk.Label(speed_row, text="Playback speed", style="Muted.TLabel").pack(side="left")
        self.speed = tk.DoubleVar(value=5)
        self.speed_label = ttk.Label(speed_row, text="5.0 moves/s", width=13, anchor="e")
        self.speed_label.pack(side="right")
        ttk.Scale(speed_row, variable=self.speed, from_=0.5, to=60,
                  command=self.change_speed).pack(side="left", fill="x", expand=True, padx=14)

        self.status = ttk.Label(body, style="Muted.TLabel", wraplength=650)
        self.status.pack(anchor="w", pady=(15, 0))
        ttk.Label(body, text="Blank = empty square   ·   Space: play/pause   ·   ← / →: step",
                  style="Muted.TLabel").pack(anchor="w", pady=(7, 0))

        # Run shortcuts before a focused button/slider handles the same key.
        shortcut_tag = "PuzzleViewerKeys"
        for key, action in (("<space>", self.toggle_play),
                            ("<Left>", lambda: self.step(-1)),
                            ("<Right>", lambda: self.step(1)),
                            ("<Home>", lambda: self.jump(0)),
                            ("<End>", lambda: self.jump(self.last))):
            root.bind_class(shortcut_tag, key,
                            lambda event, callback=action: self.key_action(event, callback))

        def attach_shortcuts(widget):
            widget.bindtags((shortcut_tag,) + widget.bindtags())
            for child in widget.winfo_children():
                attach_shortcuts(child)

        attach_shortcuts(root)
        self.set_solution(solution)

    @property
    def last(self) -> int:
        return len(self.solution.states) - 1

    def key_action(self, _event, callback):
        callback()
        return "break"

    def set_solution(self, solution: Solution):
        self.pause()
        self.solution = solution
        self.index = 0
        self.seek_slider.configure(to=max(1, self.last))
        self.filename.configure(text=f"{solution.name}  ·  {self.last:,} moves")
        self.status.configure(text=solution.note or "Ready. Press Play, step, or drag the timeline.")
        self.refresh()

    def open_file(self):
        self.pause()
        filename = filedialog.askopenfilename(
            parent=self.root, title="Open saved puzzle solution",
            filetypes=[("Puzzle solution", "*.txt"), ("All files", "*")])
        if not filename:
            return
        try:
            solution = read_solution(filename)
        except (OSError, UnicodeError, ValueError) as error:
            messagebox.showerror("Cannot load solution", str(error), parent=self.root)
            return
        self.set_solution(solution)

    def change_speed(self, _value=None):
        self.speed_label.configure(text=f"{self.speed.get():.1f} moves/s")

    def pause(self):
        self.playing = False
        if self.timer is not None:
            self.root.after_cancel(self.timer)
            self.timer = None
        self.phase = 0.0
        if hasattr(self, "play_button"):
            self.refresh()

    def jump(self, index: int):
        self.pause()
        self.index = max(0, min(self.last, index))
        self.refresh()

    def step(self, delta: int):
        self.jump(self.index + delta)

    def seek(self, value):
        if not self.updating_slider:
            self.jump(round(float(value)))

    def toggle_play(self):
        if self.playing:
            self.pause()
            return
        if self.last == 0:
            return
        if self.index == self.last:
            self.index = 0
        self.phase = 0.0
        self.last_tick = time.monotonic()
        self.playing = True
        self.refresh()
        self.timer = self.root.after(16, self.tick)

    def tick(self):
        self.timer = None
        if not self.playing:
            return
        now = time.monotonic()
        self.phase += (now - self.last_tick) * self.speed.get()
        self.last_tick = now
        advance = int(self.phase)
        self.index = min(self.last, self.index + advance)
        self.phase -= advance
        if self.index == self.last:
            self.playing = False
            self.phase = 0.0
        self.refresh()
        if self.playing:
            self.timer = self.root.after(16, self.tick)

    def refresh(self):
        self.counter.configure(text=f"Move {self.index:,} / {self.last:,}")
        if self.last == 0:
            direction = "Single saved state"
        elif self.index == self.last:
            direction = "End of saved route"
        elif self.index == 0:
            direction = "Starting board"
        else:
            previous = self.solution.states[self.index - 1].index("0")
            current = self.solution.states[self.index].index("0")
            names = {-3: "UP", -1: "LEFT", 1: "RIGHT", 3: "DOWN"}
            direction = "Blank moved " + names[current - previous]
        self.direction.configure(text=direction)
        self.play_button.configure(text="Pause" if self.playing else (
            "Replay" if self.index == self.last and self.last > 0 else "Play"))
        self.play_button.state(["disabled"] if self.last == 0 else ["!disabled"])
        for button in (self.first_button, self.back_button):
            button.state(["disabled"] if self.index == 0 else ["!disabled"])
        for button in (self.next_button, self.last_button):
            button.state(["disabled"] if self.index == self.last else ["!disabled"])
        self.seek_slider.state(["disabled"] if self.last == 0 else ["!disabled"])
        self.updating_slider = True
        try:
            self.position.set(self.index)
        finally:
            self.updating_slider = False
        self.draw_board()

    def rounded_box(self, x1, y1, x2, y2, radius, **options):
        points = [x1 + radius, y1, x2 - radius, y1, x2, y1,
                  x2, y1 + radius, x2, y2 - radius, x2, y2,
                  x2 - radius, y2, x1 + radius, y2, x1, y2,
                  x1, y2 - radius, x1, y1 + radius, x1, y1]
        self.canvas.create_polygon(points, smooth=True, splinesteps=24, **options)

    def draw_board(self):
        canvas = self.canvas
        canvas.delete("all")
        width, height = canvas.winfo_width(), canvas.winfo_height()
        size = max(30, min(width, height) - 40)
        left, top = (width - size) / 2, (height - size) / 2
        gap = max(5, size * 0.035)
        tile_size = (size - 2 * gap) / 3

        def position(cell):
            return (left + (cell % 3) * (tile_size + gap),
                    top + (cell // 3) * (tile_size + gap))

        for cell in range(9):
            x, y = position(cell)
            self.rounded_box(x, y, x + tile_size, y + tile_size, 14,
                             fill=self.BG, outline=self.BORDER, width=1)

        board = self.solution.states[self.index]
        next_board = (self.solution.states[self.index + 1]
                      if self.playing and self.index < self.last else board)
        fraction = self.phase * self.phase * (3 - 2 * self.phase)
        finished = self.index == self.last and self.last > 0
        for number in "12345678":
            source = board.index(number)
            destination = next_board.index(number)
            x, y = position(source)
            next_x, next_y = position(destination)
            x += (next_x - x) * fraction
            y += (next_y - y) * fraction
            highlighted = source != destination or finished
            self.rounded_box(x, y, x + tile_size, y + tile_size, 14,
                             fill=self.TILE, outline=self.ACCENT if highlighted else self.TILE,
                             width=2)
            canvas.create_text(x + tile_size / 2, y + tile_size / 2, text=number,
                               fill=self.ACCENT if finished else self.TEXT,
                               font=("DejaVu Sans", max(14, int(tile_size * 0.40)), "bold"))

    def close(self):
        self.pause()
        self.root.destroy()


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("file", nargs="?", help="Saved route; defaults to puzzle_states.txt if present")
    parser.add_argument("--demo", action="store_true", help="Open the built-in three-move example")
    parser.add_argument("--check", action="store_true", help="Validate the file without opening a window")
    args = parser.parse_args()

    filename = args.file or "puzzle_states.txt"
    try:
        if args.demo:
            solution = demo_solution()
        elif args.file or Path(filename).exists() or args.check:
            solution = read_solution(filename)
        else:
            solution = demo_solution()
    except (OSError, UnicodeError, ValueError) as error:
        print(f"Cannot load solution: {error}", file=sys.stderr)
        return 2

    if args.check:
        print(f"Valid route: {len(solution.states):,} states, {len(solution.states) - 1:,} moves.")
        if solution.note:
            print(solution.note)
        return 0

    if tk is None:
        print("Tkinter is missing. On Ubuntu run: sudo apt install python3-tk", file=sys.stderr)
        return 1
    try:
        root = tk.Tk()
    except tk.TclError as error:
        print(f"Cannot open a desktop window: {error}\n"
              "Run this on your desktop, or use --check to validate without a window.", file=sys.stderr)
        return 1
    PuzzleViewer(root, solution)
    root.mainloop()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
