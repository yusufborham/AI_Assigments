"""Play against the sibling Connect Four C executable in a Tkinter window."""

import argparse
import os
from pathlib import Path
from queue import Empty, Queue
import subprocess
from threading import Thread
from time import perf_counter
import tkinter as tk


BACKGROUND = "#131C2B"
BOARD = "#263850"
EMPTY_CELL = "#18283D"
WHITE = "#F2F6FA"
MUTED = "#A8B8CB"
AI_COLOR = "#62DEC3"
HUMAN_COLOR = "#FFC36A"
DISC_COLORS = {".": EMPTY_CELL, "x": AI_COLOR, "o": HUMAN_COLOR}
ENGINE = Path(__file__).resolve().with_name(
    "connect4_engine.exe" if os.name == "nt" else "connect4_engine"
)


class ConnectFourApp:
    def __init__(self, root, depth, use_alpha_beta):
        self.root = root
        self.depth = depth
        self.use_alpha_beta = use_alpha_beta
        self.process = None
        self.messages = Queue()
        self.rows, self.cols = 6, 7
        self.board = "." * (self.rows * self.cols)
        self.turn = "busy"
        self.last_col = -1
        self.hover_col = None
        self.ai_started_at = None

        root.title("Connect Four")
        root.geometry("740x680")
        root.minsize(510, 510)
        root.configure(bg=BACKGROUND)
        root.protocol("WM_DELETE_WINDOW", self.close)

        header = tk.Frame(root, bg=BACKGROUND)
        header.pack(fill="x", padx=25, pady=(19, 6))
        tk.Label(header, text="Connect Four", font=("TkDefaultFont", 23, "bold"),
                 bg=BACKGROUND, fg=WHITE).pack(side="left")
        tk.Button(header, text="New game", command=self.start_game, bg=BOARD,
                  fg=WHITE, activebackground="#394F6C", activeforeground=WHITE,
                  relief="flat", padx=14, pady=6).pack(side="right")

        scoreboard = tk.Frame(root, bg=BACKGROUND)
        scoreboard.pack(fill="x", padx=25, pady=(0, 8))
        self.x_score = tk.StringVar(value="AI (X)  0")
        self.o_score = tk.StringVar(value="You (O)  0")
        tk.Label(scoreboard, textvariable=self.x_score, bg=BACKGROUND, fg=AI_COLOR,
                 font=("TkDefaultFont", 12, "bold")).pack(side="left")
        tk.Label(scoreboard, textvariable=self.o_score, bg=BACKGROUND, fg=HUMAN_COLOR,
                 font=("TkDefaultFont", 12, "bold")).pack(side="right")
        tk.Label(scoreboard, text="Four-in-a-row count", bg=BACKGROUND,
                 fg=MUTED).pack()

        self.canvas = tk.Canvas(root, bg=BACKGROUND, highlightthickness=0)
        self.canvas.pack(fill="both", expand=True, padx=16)
        self.canvas.bind("<Configure>", self.draw)
        self.canvas.bind("<Button-1>", self.play_column)
        self.canvas.bind("<Motion>", self.hover)
        self.canvas.bind("<Leave>", self.clear_hover)

        self.status = tk.StringVar(value="Starting game…")
        tk.Label(root, textvariable=self.status, bg=BACKGROUND, fg=WHITE,
                 font=("TkDefaultFont", 12, "bold")).pack(pady=(9, 2))
        self.ai_time = tk.StringVar(value="Last AI move: —")
        tk.Label(root, textvariable=self.ai_time, bg=BACKGROUND, fg=MUTED).pack(pady=(0, 4))
        tk.Label(root, text="The player with the most fours when the board is full wins.",
                 bg=BACKGROUND, fg=MUTED).pack(pady=(0, 16))

        self.root.after(25, self.poll_engine)
        self.start_game()

    def start_game(self):
        self.stop_engine()
        self.board = "." * (self.rows * self.cols)
        self.turn = "busy"
        self.last_col = -1
        self.hover_col = None
        self.ai_started_at = None
        self.ai_time.set("Last AI move: —")
        self.x_score.set("AI (X)  0")
        self.o_score.set("You (O)  0")
        self.status.set("AI is thinking…")
        self.draw()

        try:
            self.process = subprocess.Popen(
                [str(ENGINE), "--gui", str(self.depth), "1" if self.use_alpha_beta else "0"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE, text=True, encoding="ascii", bufsize=1,
            )
        except OSError as error:
            if isinstance(error, FileNotFoundError):
                self.status.set(f"Build {ENGINE.name}, then click New game.")
            else:
                self.status.set(f"Could not start the engine: {error.strerror}")
            return

        Thread(target=self.read_engine, args=(self.process,), daemon=True).start()

    def read_engine(self, process):
        with process.stdout as output:
            for line in output:
                self.messages.put((process, line.strip(), perf_counter()))
        process.wait()
        self.messages.put((process, None, None))

    def poll_engine(self):
        while True:
            try:
                process, message, received_at = self.messages.get_nowait()
            except Empty:
                break
            if process is self.process:
                self.handle_message(message, received_at)
        self.root.after(25, self.poll_engine)

    def handle_message(self, message, received_at):
        if message is None:
            if self.turn != "-":
                self.turn = "busy"
                self.status.set("The engine stopped. Click New game to restart.")
                self.draw()
            return

        fields = message.split()
        if fields[0] == "READY":
            self.rows, self.cols = map(int, fields[1:])
            self.board = "." * (self.rows * self.cols)
            return
        if fields[0] == "INVALID":
            self.turn = "o"
            self.status.set("That column is unavailable. Pick another column.")
            self.draw()
            return

        _, self.turn, last_col, x_score, o_score, self.board = fields
        self.last_col = int(last_col)
        self.x_score.set(f"AI (X)  {x_score}")
        self.o_score.set(f"You (O)  {o_score}")
        if self.turn == "x":
            self.ai_started_at = received_at
            self.status.set("AI is thinking…")
        elif self.turn == "o":
            if self.ai_started_at is not None:
                elapsed = received_at - self.ai_started_at
                self.ai_time.set(f"Last AI move: {elapsed:.3f} s")
                self.ai_started_at = None
            self.status.set("Your turn. Click a column to drop a disc.")
        else:
            difference = int(x_score) - int(o_score)
            winner = "AI wins." if difference > 0 else "You win." if difference < 0 else "Draw."
            self.status.set(f"Board full. {winner}")
        self.draw()

    def play_column(self, event):
        col = self.column_at(event.x, event.y)
        if col is None or not self.can_play(col):
            return

        self.turn = "busy"
        self.status.set("Sending your move…")
        self.draw()
        try:
            self.process.stdin.write(f"{col}\n")
            self.process.stdin.flush()
        except OSError:
            self.status.set("The engine stopped. Click New game to restart.")

    def can_play(self, col):
        return self.turn == "o" and self.board[col] == "."

    def layout(self):
        width = self.canvas.winfo_width()
        height = self.canvas.winfo_height()
        cell = max(1, min((width - 12) // self.cols, (height - 12) // (self.rows + 1)))
        left = (width - self.cols * cell) / 2
        top = (height - (self.rows + 1) * cell) / 2
        return left, top, cell

    def column_at(self, x, y):
        left, top, cell = self.layout()
        if left <= x < left + self.cols * cell and top <= y < top + (self.rows + 1) * cell:
            return int((x - left) // cell)
        return None

    def hover(self, event):
        col = self.column_at(event.x, event.y)
        if col != self.hover_col:
            self.hover_col = col
            self.draw()

    def clear_hover(self, _event):
        self.hover_col = None
        self.draw()

    def draw(self, _event=None):
        canvas = self.canvas
        canvas.delete("all")
        left, top, cell = self.layout()
        board_top = top + cell
        board_bottom = board_top + self.rows * cell
        canvas.create_rectangle(left, board_top, left + self.cols * cell,
                                board_bottom, fill=BOARD, width=0)
        for col in range(self.cols):
            x = left + (col + 0.5) * cell
            highlighted = col == self.hover_col and self.can_play(col)
            if highlighted:
                canvas.create_rectangle(x - cell / 2, board_top, x + cell / 2,
                                        board_bottom, fill="#394F6C", width=0)
            canvas.create_text(x, top + cell / 2, text=str(col + 1),
                               fill=HUMAN_COLOR if highlighted else MUTED,
                               font=("TkDefaultFont", 13, "bold"))

            latest_row = None
            if col == self.last_col:
                latest_row = next((r for r in range(self.rows)
                                   if self.board[r * self.cols + col] != "."), None)
            for row in range(self.rows):
                y = board_top + (row + 0.5) * cell
                radius = cell * 0.37
                disc = self.board[row * self.cols + col]
                canvas.create_oval(x - radius, y - radius, x + radius, y + radius,
                                   fill=DISC_COLORS[disc], outline=WHITE if row == latest_row else "",
                                   width=2)
        can_click = self.hover_col is not None and self.can_play(self.hover_col)
        canvas.configure(cursor="hand2" if can_click else "")

    def stop_engine(self):
        process = self.process
        self.process = None
        if process is None:
            return
        if process.poll() is None:
            process.terminate()
        process.stdin.close()

    def close(self):
        self.stop_engine()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--depth", type=int, default=8)
    parser.add_argument("--no-alpha-beta", action="store_true")
    args = parser.parse_args()

    if not 1 <= args.depth <= 42:
        parser.error("--depth must be between 1 and 42")

    root = tk.Tk()
    ConnectFourApp(root, args.depth, not args.no_alpha_beta)
    root.mainloop()


if __name__ == "__main__":
    main()
