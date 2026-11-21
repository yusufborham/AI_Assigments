#!/usr/bin/env python3
"""Turn one Connect Four search reply from GameTree.txt into an interactive HTML tree."""

import argparse
from dataclasses import asdict, dataclass, field
import json
from pathlib import Path


@dataclass
class Node:
    layer: int
    parent: int | None
    children: list[int] = field(default_factory=list)
    board: str = ""
    column: int | None = None
    leaf_score: int | None = None
    value: int | None = None
    bound: str = "exact"
    cutoff: bool = False


def list_replies(path):
    with path.open(encoding="utf-8") as stream:
        for line in stream:
            if line.startswith("REPLY NUMBER "):
                yield int(line.split()[-1])


def read_reply(path, reply, max_nodes):
    rows, cols = 6, 7
    nodes = []
    active = []
    found = False
    base_layer = None
    metadata = {"reply": reply}
    with path.open(encoding="utf-8") as stream:
        for number, raw_line in enumerate(stream, 1):
            line = raw_line.strip()
            if line.startswith("READY "):
                fields = line.split()
                if len(fields) == 3:
                    rows, cols = map(int, fields[1:])
            elif line.startswith("REPLY NUMBER "):
                if found:
                    break
                found = int(line.split()[-1]) == reply
            elif not found:
                continue
            elif line.startswith("LAYER NUMBER "):
                if len(nodes) == max_nodes:
                    metadata["viewer_limit"] = max_nodes
                    break
                printed_layer = int(line.split()[-1])
                if base_layer is None:
                    base_layer = printed_layer
                layer = printed_layer - base_layer
                if layer < 0 or layer > len(active) or (nodes and layer == 0):
                    raise ValueError(f"Line {number} has an invalid tree layer.")
                parent = active[layer - 1] if layer else None
                node_id = len(nodes)
                nodes.append(Node(layer, parent))
                active[layer:] = [node_id]
                if parent is not None:
                    nodes[parent].children.append(node_id)
            elif line.startswith("|") and nodes:
                cells = line.split("|")[1:-1]
                row = "".join(cell.strip().lower() or "." for cell in cells)
                if len(cells) != cols or len(row) != cols or set(row) - set("xo."):
                    raise ValueError(f"Line {number} has an invalid board row.")
                nodes[-1].board += row
            elif line.startswith("The Score Here Is ") and nodes:
                nodes[-1].leaf_score = int(line.split()[-1])
            elif line.startswith("RETURN "):
                _, layer_text, score, bound, cutoff = line.split()
                layer = int(layer_text)
                if layer < 0 or layer >= len(active):
                    raise ValueError(f"Line {number} refers to an unknown tree layer.")
                node = nodes[active[layer]]
                node.value = int(score)
                node.bound = bound
                node.cutoff = cutoff == "1"
            else:
                for prefix, key in (
                    ("SEARCH DEPTH ", "search_depth"),
                    ("TRACE LAYERS ", "trace_layers"),
                    ("BEST COLUMN ", "best_column"),
                    ("TRACE NODE LIMIT ", "trace_limit"),
                ):
                    if line.startswith(prefix):
                        metadata[key] = int(line[len(prefix):])
                        break

    if not nodes:
        raise ValueError(f"No search tree for AI reply {reply}. Use --list-replies to see what is in the log.")
    for node in nodes:
        if len(node.board) != rows * cols:
            raise ValueError("A board is incomplete. Wait until the engine finishes writing the selected reply.")
        if node.parent is not None:
            parent_board = nodes[node.parent].board
            changes = [i for i, (before, after) in enumerate(zip(parent_board, node.board))
                       if before != after]
            if len(changes) == 1:
                node.column = changes[0] % cols

    metadata.update(rows=rows, cols=cols, nodes=[asdict(node) for node in nodes])
    return metadata


PAGE = """<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Connect Four · Search Tree</title>
<style>
:root {
  color-scheme: dark;
  --bg: #101322; --panel: #181d32; --card: #1f2640; --line: #465575;
  --ink: #f5f6ff; --muted: #b1bad2; --x: #55e9c4; --o: #ff947f;
  --leaf: #d6b3ff; --accent: #f6d276;
}
* { box-sizing: border-box; }
body { margin: 0; background: var(--bg); color: var(--ink);
  font: 15px/1.5 system-ui, sans-serif; }
header { padding: 25px clamp(16px, 4vw, 56px); background: #171d34; border-bottom: 1px solid #34415e; }
h1 { margin: 0 0 6px; font-size: clamp(24px, 3vw, 40px); letter-spacing: -.025em; }
h2 { margin: 0 0 10px; font-size: 20px; }
#summary { margin: 0; color: var(--muted); }
#note { margin: 7px 0 0; color: var(--accent); }
#note:empty { display: none; }
main { display: grid; grid-template-columns: minmax(0, 1fr) 280px; gap: 16px;
  padding: 18px clamp(12px, 3vw, 40px); align-items: start; }
section, aside { min-width: 0; border: 1px solid #303d59; background: var(--panel); border-radius: 14px; }
.toolbar { display: flex; gap: 8px; align-items: center; flex-wrap: wrap; padding: 12px; }
button { color: var(--ink); background: #263151; border: 1px solid #47587a;
  border-radius: 8px; padding: 6px 10px; font: inherit; cursor: pointer; }
button:hover { border-color: var(--accent); }
button:focus-visible { outline: 2px solid var(--accent); outline-offset: 3px; }
button:disabled { opacity: .5; cursor: default; }
#scroll { min-height: 340px; max-height: 70vh; overflow: auto; border-top: 1px solid #303d59;
  background-color: #14192b; background-image: radial-gradient(#313c58 1px, transparent 1px);
  background-size: 18px 18px; border-radius: 0 0 14px 14px; }
#tree { padding: 24px; width: max-content; min-width: 100%; }
.branch { display: flex; flex-direction: column; align-items: center; position: relative;
  padding: 24px 5px 0; flex: 0 0 auto; }
#tree > .branch { padding-top: 0; }
.children { display: flex; justify-content: center; width: max-content; position: relative; padding: 14px 0 0; }
.children::before { content: ""; position: absolute; height: 14px; border-left: 1px solid var(--line); top: 0; left: 50%; }
.children > .branch::before, .children > .branch::after { content: ""; position: absolute; top: 0;
  width: 50%; height: 24px; border-top: 1px solid var(--line); }
.children > .branch::before { right: 50%; }
.children > .branch::after { left: 50%; border-left: 1px solid var(--line); }
.children > .branch:first-child::before { border-top: 0; }
.children > .branch:last-child::after { border-top: 0; }
.head { width: 142px; }
.node { display: block; width: 100%; text-align: left; background: var(--card); padding: 8px 10px;
  border: 1px solid #485681; border-top: 3px solid var(--player); }
.node[aria-pressed="true"] { outline: 2px solid var(--accent); outline-offset: 2px; }
.head:has(.node[aria-pressed="true"]) { z-index: 1; }
.node span { display: block; }
.node .small { font-size: 12px; color: var(--muted); }
.node .score { margin-top: 4px; color: var(--leaf); font-weight: 700; }
.node .chosen { color: var(--accent); font-weight: 700; }
.node.x { --player: var(--x); }
.node.o { --player: var(--o); }
.toggle { margin-top: 7px; width: 100%; color: var(--muted); font-size: 12px; padding: 3px 5px; }
aside { padding: 16px; }
#detail, #detailscore { margin: 0 0 8px; }
#detailscore { color: var(--leaf); font-weight: 650; }
#board { display: grid; gap: 5px; background: #263457; border-radius: 10px;
  padding: 8px; margin: 12px 0; }
.cell { aspect-ratio: 1; border-radius: 50%; background: #11182e; }
.cell.x { background: var(--x); }
.cell.o { background: var(--o); }
.legend { display: flex; flex-wrap: wrap; gap: 10px; font-size: 13px; color: var(--muted); }
.legend b { color: var(--player); }
@media (max-width: 800px) { main { grid-template-columns: 1fr; } #scroll { max-height: 55vh; }
  #board { max-width: 260px; } }
</style>
</head>
<body>
<header>
  <h1>Connect Four Search Tree</h1>
  <p id="summary"></p>
  <p id="note"></p>
</header>
<main>
  <section aria-label="Search tree">
    <div class="toolbar">
      <button id="expand" type="button">Expand one level</button>
      <button id="collapse" type="button">Collapse branches</button>
      <div class="legend">
        <span><b style="--player: var(--x)">X</b> maximizing</span>
        <span><b style="--player: var(--o)">O</b> minimizing</span>
      </div>
    </div>
    <div id="scroll"><div id="tree"></div></div>
  </section>
  <aside aria-label="Selected node">
    <h2 id="detailtitle">Root position</h2>
    <p id="detail"></p>
    <p id="detailscore"></p>
    <div id="board" role="img"></div>
  </aside>
</main>
<script>
const data = __TREE_DATA__;
const nodes = data.nodes;
const expanded = new Set([0]);
let selected = 0;
const byId = name => document.getElementById(name);
const tree = byId("tree");
const scroll = byId("scroll");
const expand = byId("expand");
const collapse = byId("collapse");

function el(tag, className, text) {
  const item = document.createElement(tag);
  if (className) item.className = className;
  if (text !== undefined) item.textContent = text;
  return item;
}
function nextPlayer(node) { return node.layer % 2 === 0 ? "X" : "O"; }
function moveLabel(node) {
  if (node.parent === null) return "Root position";
  const player = nextPlayer(nodes[node.parent]);
  return node.column === null ? player + " moves" : player + " → column " + node.column;
}
function scoreLabel(node) {
  if (node.leaf_score !== null) return "Leaf score " + node.leaf_score;
  if (node.value !== null) {
    const sign = node.bound === "lower" ? " ≥ " : node.bound === "upper" ? " ≤ " : " ";
    return (node.children.length ? "Search value" : "Frontier value") + sign + node.value;
  }
  return node.children.length ? "" : "Score unavailable";
}
function select(id) {
  selected = id;
  const node = nodes[id];
  tree.querySelector('[aria-pressed="true"]')?.setAttribute("aria-pressed", "false");
  byId("node-" + id)?.setAttribute("aria-pressed", "true");
  byId("detailtitle").textContent = moveLabel(node);
  byId("detail").textContent = "Layer " + node.layer + " · " + nextPlayer(node) + " to move"
    + (node.cutoff ? " · Alpha-beta cutoff" : "");
  let value = scoreLabel(node);
  if (!node.children.length && node.leaf_score === null && node.value !== null)
    value += " · Deeper nodes hidden";
  byId("detailscore").textContent = value;
  const board = byId("board");
  board.style.gridTemplateColumns = "repeat(" + data.cols + ", 1fr)";
  board.setAttribute("aria-label", Array.from({length: data.rows}, (_, row) =>
    node.board.slice(row * data.cols, (row + 1) * data.cols).split("").join(" ")
  ).join(", "));
  board.replaceChildren(...Array.from(node.board, cell => el("span", "cell " + (cell === "." ? "" : cell))));
}
function renderBranch(id) {
  const node = nodes[id];
  const branch = el("div", "branch");
  const head = el("div", "head");
  const button = el("button", "node " + nextPlayer(node).toLowerCase());
  button.id = "node-" + id;
  button.type = "button";
  button.setAttribute("aria-pressed", String(id === selected));
  button.append(el("span", "", moveLabel(node)), el("span", "small", "Layer " + node.layer));
  const score = scoreLabel(node);
  if (score) button.append(el("span", "small score", score));
  if (node.parent === 0 && node.column === data.best_column)
    button.append(el("span", "small chosen", "Chosen move"));
  button.addEventListener("click", () => select(id));
  head.append(button);
  branch.append(head);
  if (node.children.length) {
    const open = expanded.has(id);
    const toggle = el("button", "toggle", (open ? "− Hide " : "+ Show ") + node.children.length + " branches");
    toggle.type = "button";
    toggle.setAttribute("aria-expanded", String(open));
    toggle.onclick = () => {
      if (open) expanded.delete(id);
      else expanded.add(id);
      select(id);
      render();
      byId("node-" + id).parentElement.querySelector(".toggle").focus();
    };
    head.append(toggle);
    if (open) {
      const children = el("div", "children");
      for (const child of node.children) children.append(renderBranch(child));
      branch.append(children);
    }
  }
  return branch;
}
function visible() {
  const ids = [];
  const queue = [0];
  for (let i = 0; i < queue.length; ++i) {
    const id = queue[i];
    ids.push(id);
    if (expanded.has(id)) queue.push(...nodes[id].children);
  }
  return ids;
}
function render() {
  tree.replaceChildren(renderBranch(0));
  select(selected);
  expand.disabled = !visible().some(id => !expanded.has(id) && nodes[id].children.length);
  collapse.disabled = expanded.size === 0;
}
expand.onclick = () => {
  for (const id of visible()) if (nodes[id].children.length) expanded.add(id);
  render();
};
collapse.onclick = () => {
  expanded.clear();
  selected = 0;
  render();
};
const summary = ["AI reply " + data.reply, nodes.length + " recorded positions",
  nodes.filter(node => node.leaf_score !== null).length + " scored leaves"];
if (data.search_depth !== undefined) summary.push("Search depth " + data.search_depth);
if (data.best_column !== undefined) summary.push("Chosen column " + data.best_column);
byId("summary").textContent = summary.join(" · ");
const notes = [];
if (data.trace_layers !== undefined && data.trace_layers < data.search_depth)
  notes.push("The trace stops at layer " + data.trace_layers + ". Frontier values come from deeper search.");
if (data.trace_limit || data.viewer_limit)
  notes.push("The node limit hid some branches.");
if (data.search_depth === undefined)
  notes.push("Older logs can omit cached leaf scores. Missing values are marked as unavailable.");
byId("note").textContent = notes.join(" ");
render();
scroll.scrollLeft = Math.max(0, (scroll.scrollWidth - scroll.clientWidth) / 2);
</script>
</body>
</html>
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path, nargs="?", default=Path("GameTree.txt"))
    parser.add_argument("--reply", type=int, help="AI reply number to show")
    parser.add_argument("--list-replies", action="store_true", help="list reply numbers in the log")
    parser.add_argument("--max-nodes", type=int, default=4000, help="maximum positions in the HTML")
    parser.add_argument("-o", "--output", type=Path, help="HTML file to write")
    args = parser.parse_args()
    if args.max_nodes < 1 or (args.reply is not None and args.reply < 1):
        parser.error("reply and max-nodes must be positive")
    try:
        if args.list_replies:
            replies = list(list_replies(args.log))
            print(" ".join(map(str, replies)) if replies else "No AI replies found.")
            return
        reply = args.reply
        if reply is None:
            with_replies = list_replies(args.log)
            try:
                reply = next(with_replies, None)
            finally:
                with_replies.close()
            if reply is None:
                raise ValueError("No AI replies found in the log.")
        data = read_reply(args.log, reply, args.max_nodes)
        output = args.output or Path(f"GameTree_reply_{reply}.html")
        if output.resolve() == args.log.resolve():
            raise ValueError("Choose a different output path from the input log.")
        payload = json.dumps(data, separators=(",", ":")).replace("<", "\\u003c")
        output.write_text(PAGE.replace("__TREE_DATA__", payload, 1), encoding="utf-8")
        print(f"Created {output.resolve()}")
    except (OSError, ValueError) as error:
        parser.exit(1, f"{error}\n")


if __name__ == "__main__":
    main()
