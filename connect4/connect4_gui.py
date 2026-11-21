"""Play against a Connect Four C engine or watch two engines play."""

import argparse
import os
from pathlib import Path
from queue import Empty, Queue
import subprocess
from threading import Event, Thread
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
DUEL_MOVE_DELAY = 0.35
ENGINE = Path(__file__).resolve().with_name(
    "connect4_engine.exe" if os.name == "nt" else "connect4_engine"
)


class ConnectFourApp:
    def __init__(self, root, depth, use_alpha_beta, duel_depths=None):
        self.root = root
        self.depth = depth
        self.use_alpha_beta = use_alpha_beta
        self.duel_depths = duel_depths
        self.process = None
        self.opponent_process = None
        self.duel_stop = Event()
        self.player_names = {"x": "AI (X)", "o": "You (O)"}
        if duel_depths is not None:
            self.player_names = {
                "x": f"X (depth {duel_depths[0]})",
                "o": f"O (depth {duel_depths[1]})",
            }
        self.time_placeholder = (
            "Last move: —" if duel_depths is not None else "Last AI move: —"
        )
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
        self.x_score = tk.StringVar()
        self.o_score = tk.StringVar()
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
        self.ai_time = tk.StringVar(value=self.time_placeholder)
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
        self.ai_time.set(self.time_placeholder)
        self.update_scores(0, 0)
        self.status.set("Starting match…" if self.duel_depths else "AI is thinking…")
        self.draw()

        try:
            if self.duel_depths is None:
                self.process = self.open_engine(self.depth)
            else:
                x_depth, o_depth = self.duel_depths
                self.process = self.open_engine(x_depth)
                self.opponent_process = self.open_engine(o_depth, "o")
        except OSError as error:
            started_process = self.process
            self.stop_engine()
            if started_process is not None:
                started_process.stdout.close()
                started_process.wait()
            if isinstance(error, FileNotFoundError):
                self.status.set(f"Build {ENGINE.name}, then click New game.")
            else:
                self.status.set(f"Could not start the engine: {error.strerror}")
            return

        if self.duel_depths is None:
            Thread(target=self.read_engine, args=(self.process,), daemon=True).start()
        else:
            self.duel_stop = Event()
            Thread(
                target=self.read_duel,
                args=(self.process, self.opponent_process, self.duel_stop),
                daemon=True,
            ).start()

    def open_engine(self, depth, role="x"):
        command = [str(ENGINE), "--gui", str(depth), "1" if self.use_alpha_beta else "0"]
        if role == "o":
            command.append(role)
        return subprocess.Popen(
            command, stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            text=True, encoding="ascii", bufsize=1,
        )

    def read_engine(self, process):
        with process.stdout as output:
            for line in output:
                self.messages.put((process, line.strip(), perf_counter(), None))
        process.wait()
        self.messages.put((process, None, None, None))

    @staticmethod
    def read_duel_state(process):
        state = process.stdout.readline().strip()
        if not state.startswith("STATE ") or len(state.split()) != 6:
            raise ValueError("Unexpected engine response. Rebuild the duel engine and try again.")
        return state

    @staticmethod
    def send_column(process, col):
        process.stdin.write(f"{col}\n")
        process.stdin.flush()

    def read_duel(self, x_engine, o_engine, stopped):
        """Relay each move and check that both engines agree on the board."""
        completed = False
        try:
            ready = x_engine.stdout.readline().strip()
            initial = self.read_duel_state(x_engine)
            started_at = perf_counter()
            if (not ready.startswith("READY ") or
                    o_engine.stdout.readline().strip() != ready or
                    self.read_duel_state(o_engine) != initial):
                raise ValueError("The engines did not start with the same board.")

            self.messages.put((x_engine, ready, None, None))
            self.messages.put((x_engine, initial, None, None))
            playing, waiting = x_engine, o_engine
            while not stopped.is_set():
                state = self.read_duel_state(playing)
                elapsed = perf_counter() - started_at
                _, next_turn, column, _, _, _ = state.split()

                if next_turn != "-":
                    self.messages.put((x_engine, state, None, elapsed))
                    # Pace the display before handing over to the other engine.
                    if stopped.wait(DUEL_MOVE_DELAY):
                        break

                self.send_column(waiting, column)
                confirmed = self.read_duel_state(waiting)
                started_at = perf_counter()
                if confirmed != state:
                    raise ValueError("The engines disagree on the board after a move.")
                if next_turn == "-":
                    completed = True
                    self.messages.put((x_engine, state, None, elapsed))
                    break
                playing, waiting = waiting, playing
        except (OSError, ValueError) as error:
            if not stopped.is_set():
                self.messages.put((x_engine, f"ERROR {error}", None, None))
        finally:
            for process in (x_engine, o_engine):
                if not completed and process.poll() is None:
                    process.terminate()
                process.wait()
                process.stdin.close()
                process.stdout.close()

    def poll_engine(self):
        while True:
            try:
                process, message, received_at, move_seconds = self.messages.get_nowait()
            except Empty:
                break
            if process is self.process:
                self.handle_message(message, received_at, move_seconds)
        self.root.after(25, self.poll_engine)

    def update_scores(self, x_score, o_score):
        self.x_score.set(f"{self.player_names['x']}  {x_score}")
        self.o_score.set(f"{self.player_names['o']}  {o_score}")

    def handle_message(self, message, received_at, move_seconds=None):
        if message is None:
            if self.turn != "-":
                self.turn = "busy"
                self.status.set("The engine stopped. Click New game to restart.")
                self.draw()
            return

        if message.startswith("ERROR "):
            self.turn = "busy"
            self.status.set(message[6:])
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

        previous_turn = self.turn
        _, self.turn, last_col, x_score, o_score, self.board = fields
        self.last_col = int(last_col)
        self.update_scores(x_score, o_score)
        if self.duel_depths is not None:
            if move_seconds is not None:
                player = self.player_names[previous_turn]
                self.ai_time.set(f"Last move: {player} in {move_seconds:.3f} s")
            if self.turn == "-":
                if int(x_score) == int(o_score):
                    winner = "Draw."
                else:
                    player = "x" if int(x_score) > int(o_score) else "o"
                    winner = f"{self.player_names[player]} wins."
                self.status.set(f"Board full. {winner}")
            else:
                self.status.set(f"{self.player_names[self.turn]} to move…")
        elif self.turn == "x":
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
            self.send_column(self.process, col)
        except OSError:
            self.status.set("The engine stopped. Click New game to restart.")

    def can_play(self, col):
        return self.duel_depths is None and self.turn == "o" and self.board[col] == "."

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
        self.duel_stop.set()
        processes = (self.process, self.opponent_process)
        self.process = None
        self.opponent_process = None
        for process in processes:
            if process is not None:
                if process.poll() is None:
                    process.terminate()
                process.stdin.close()

    def close(self):
        self.stop_engine()
        self.root.destroy()


def main():
    parser = argparse.ArgumentParser()
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--depth", type=int, default=8, help="AI depth in human vs AI mode")
    mode.add_argument(
        "--duel", type=int, nargs=2, metavar=("X_DEPTH", "O_DEPTH"),
        help="watch two engines play in X, O order",
    )
    parser.add_argument("--no-alpha-beta", action="store_true")
    args = parser.parse_args()

    depths = args.duel if args.duel is not None else [args.depth]
    if any(not 1 <= depth <= 42 for depth in depths):
        parser.error("Search depth must be between 1 and 42")

    root = tk.Tk()
    ConnectFourApp(root, args.depth, not args.no_alpha_beta, args.duel)
    root.mainloop()


if __name__ == "__main__":
    main()
