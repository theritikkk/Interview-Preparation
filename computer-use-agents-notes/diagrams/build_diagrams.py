#!/usr/bin/env python3
"""Generate the SVG diagrams used by the computer-use-agents-notes series.

Run from anywhere:
    python3 build_diagrams.py

Outputs .svg files next to this script. The SVGs are plain hand-rolled SVG
(no external libraries) so they render on GitHub, in VS Code, and in browsers.
"""

from pathlib import Path

OUT = Path(__file__).resolve().parent

# ---------------------------------------------------------------- palette ---
INK = "#0f172a"          # main text
SUB = "#475569"          # secondary text
FAINT = "#94a3b8"        # weak lines
LINE = "#cbd5e1"
BG = "#ffffff"
BLUE = "#2563eb"
GREEN = "#16a34a"
AMBER = "#d97706"
PURPLE = "#7c3aed"
RED = "#dc2626"
TEAL = "#0d9488"

FONT = "'Helvetica Neue',Helvetica,Arial,sans-serif"


def header(width, height, title, subtitle=None):
    parts = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" '
        f'viewBox="0 0 {width} {height}" font-family="{FONT}">',
        "<defs>",
        '<marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
        'markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M 0 0 L 10 5 L 0 10 z" fill="{SUB}"/></marker>',
        '<marker id="arrow-blue" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
        'markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M 0 0 L 10 5 L 0 10 z" fill="{BLUE}"/></marker>',
        '<marker id="arrow-green" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
        'markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M 0 0 L 10 5 L 0 10 z" fill="{GREEN}"/></marker>',
        '<marker id="arrow-amber" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" '
        'markerHeight="7" orient="auto-start-reverse">'
        f'<path d="M 0 0 L 10 5 L 0 10 z" fill="{AMBER}"/></marker>',
        "</defs>",
        f'<rect x="0.5" y="0.5" width="{width - 1}" height="{height - 1}" rx="14" '
        f'fill="{BG}" stroke="{LINE}"/>',
        f'<text x="{width / 2}" y="42" font-size="23" font-weight="700" fill="{INK}" '
        f'text-anchor="middle">{title}</text>',
    ]
    if subtitle:
        parts.append(
            f'<text x="{width / 2}" y="68" font-size="14" fill="{SUB}" '
            f'text-anchor="middle">{subtitle}</text>'
        )
    return parts


def box(parts, x, y, w, h, title, lines, color=BLUE, title_size=15, body_size=12.5):
    parts.append(
        f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="10" fill="#ffffff" '
        f'stroke="{color}" stroke-width="1.6"/>'
    )
    parts.append(
        f'<rect x="{x}" y="{y}" width="{w}" height="30" rx="10" fill="{color}" opacity="0.12"/>'
    )
    parts.append(
        f'<text x="{x + w / 2}" y="{y + 20}" font-size="{title_size}" font-weight="700" '
        f'fill="{color}" text-anchor="middle">{title}</text>'
    )
    ty = y + 48
    for ln in lines:
        parts.append(
            f'<text x="{x + w / 2}" y="{ty}" font-size="{body_size}" fill="{INK}" '
            f'text-anchor="middle">{ln}</text>'
        )
        ty += 19
    return ty


def note(parts, x, y, text, color=SUB, size=12.5, anchor="middle"):
    parts.append(
        f'<text x="{x}" y="{y}" font-size="{size}" fill="{color}" '
        f'text-anchor="{anchor}">{text}</text>'
    )


def arrow(parts, x1, y1, x2, y2, color=SUB, marker="arrow", width=2):
    parts.append(
        f'<line x1="{x1}" y1="{y1}" x2="{x2}" y2="{y2}" stroke="{color}" '
        f'stroke-width="{width}" marker-end="url(#{marker})"/>'
    )


def path_arrow(parts, d, color=SUB, marker="arrow", width=2):
    parts.append(
        f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" '
        f'marker-end="url(#{marker})"/>'
    )


# ------------------------------------------------------------- 1. loop -----
def agent_loop():
    W, H = 1060, 480
    p = header(W, H, "The computer-use agent loop",
               "Observe → think → act → verify, with the model never touching the machine directly")
    bw, bh = 215, 150
    y = 150
    xs = [50, 300, 550, 800]
    colors = [BLUE, PURPLE, AMBER, TEAL]
    titles = ["1 · Observe", "2 · Think", "3 · Act", "4 · Verify"]
    bodies = [
        ["Capture the screen:", "screenshot (pixels) or", "accessibility tree", "(structured text)"],
        ["LLM receives observation", "+ history, outputs one", "tool call:", "click / type / key …"],
        ["Executor translates the", "call into real input:", "XTEST, SendInput,", "CGEvent, or DoAction()"],
        ["Re-capture after the", "action and compare:", "did the screen change", "the way we expected?"],
    ]
    for x, c, t, b in zip(xs, colors, titles, bodies):
        box(p, x, y, bw, bh, t, b, color=c)
    for i in range(3):
        arrow(p, xs[i] + bw + 4, y + bh / 2, xs[i + 1] - 6, y + bh / 2)
    # return path
    path_arrow(
        p,
        f"M {xs[3] + bw / 2} {y + bh + 6} L {xs[3] + bw / 2} {y + bh + 60} "
        f"L {xs[0] + bw / 2} {y + bh + 60} L {xs[0] + bw / 2} {y + bh + 8}",
        color=BLUE, marker="arrow-blue",
    )
    note(p, W / 2, y + bh + 92, "repeat until the task is done (or the step budget runs out)", SUB)
    note(p, W / 2, y + bh + 124,
         "every iteration re-sends the whole conversation  ·  a screenshot ≈ 0.5–2k tokens  ·  a tree dump ≈ 1–5k tokens", SUB, 12.5)
    p.append("</svg>")
    (OUT / "agent-loop.svg").write_text("\n".join(p))
    print("wrote agent-loop.svg")


# ------------------------------------------------------- 2. pixel path -----
def pixel_path():
    W, H = 1060, 780
    p = header(W, H, "The pixel path: screenshot in, synthetic input out",
               "Everything the model knows arrives as pixels; everything it does leaves as low-level input events")
    # top row: display -> capture -> encode -> model
    bw, bh = 225, 128
    y1 = 130
    xs = [40, 292, 544, 796]
    box(p, xs[0], y1, bw, bh, "Virtual display",
        ["Xvfb :99 — 1024×768×24", "a real X server with no", "physical monitor", "every GUI app runs here"], color=BLUE)
    box(p, xs[1], y1, bw, bh, "Capture",
        ["XGetImage / scrot / API", "full-screen PNG", "≈ 100–400 KB on the wire,", "then re-encoded by the API"], color=BLUE)
    box(p, xs[2], y1, bw, bh, "Tokenize inside model",
        ["image chopped into fixed", "patches (e.g. 28×28 in", "Qwen2-VL lineage);", "cost grows quadratically"], color=PURPLE)
    box(p, xs[3], y1, bw, bh, "Model (vision + text)",
        ["reasons over the image", "+ conversation history", "and emits a tool call:", "click(x, y) / type('…')"], color=PURPLE)
    for i in range(3):
        arrow(p, xs[i] + bw + 4, y1 + bh / 2, xs[i + 1] - 6, y1 + bh / 2)
    # down to executor row
    path_arrow(p, f"M {xs[3] + bw / 2} {y1 + bh + 6} L {xs[3] + bw / 2} {y1 + bh + 60} L {W / 2} {y1 + bh + 60} L {W / 2} {y1 + bh + 96}",
               color=AMBER, marker="arrow-amber")
    # middle: executor
    ew, eh = 420, 150
    ex = (W - ew) / 2
    y2 = y1 + bh + 100
    box(p, ex, y2, ew, eh, "Executor (outside the model)",
        ["validate call → translate coordinates", "from model image space to display space",
         "→ inject the event → wait → re-capture", "", "no intelligence here — just plumbing"], color=AMBER)
    # bottom row: three injection APIs converging up into executor
    bw3, bh3 = 300, 128
    y3 = y2 + eh + 60
    x3 = [70, 380, 690]
    box(p, x3[0], y3, bw3, bh3, "Linux · X11",
        ["XTEST extension:", "XTestFakeMotionEvent /", "XTestFakeButtonEvent /", "XTestFakeKeyEvent"], color=GREEN)
    box(p, x3[1], y3, bw3, bh3, "Windows · Win32",
        ["SendInput():", "INPUT records, absolute coords", "normalized to 0–65535", "across the virtual desktop"], color=GREEN)
    box(p, x3[2], y3, bw3, bh3, "macOS · CoreGraphics",
        ["CGEvent → CGEventPost:", "mouse/keyboard events post", "straight into the event tap", "(needs Accessibility trust)"], color=GREEN)
    for x in x3:
        arrow(p, x + bw3 / 2, y3 - 5, ex + ew / 2 + (x - 380) * 0.25, y2 + eh + 4, color=AMBER, marker="arrow-amber")
    # feedback from executor to display (long left arrow)
    path_arrow(p, f"M {ex - 6} {y2 + eh / 2 + 3} L {xs[0] + bw / 2} {y2 + eh / 2 + 3} L {xs[0] + bw / 2} {y1 + bh + 6}",
               color=BLUE, marker="arrow-blue")
    note(p, 234, y2 + eh / 2 - 32, "events land on the", SUB, 12)
    note(p, 234, y2 + eh / 2 - 16, "display the capture came from", SUB, 12)
    note(p, W / 2, H - 26,
         "the executor is plumbing — all intelligence lives in the model, and every action loops back through the display",
         SUB, 12.5)
    p.append("</svg>")
    (OUT / "pixel-path.svg").write_text("\n".join(p))
    print("wrote pixel-path.svg")


# ------------------------------------------- 3. accessibility stack --------
def a11y_stack():
    W, H = 1060, 660
    p = header(W, H, "The structure path on Linux: AT-SPI over D-Bus",
               "Applications publish what they are, so the agent can act on elements instead of guessing coordinates")
    # left column: app side (top to bottom)
    lx, lw = 40, 430
    rows = [
        ("Application", ["GTK / Qt / Electron / Firefox — the app being controlled"], BLUE, 60),
        ("Toolkit bridge", ["ATK objects live inside the app;",
                            "atk-adaptor publishes them on D-Bus",
                            "at paths like /org/a11y/atspi/accessible/12"], TEAL, 88),
        ("AT-SPI D-Bus interfaces", ["org.a11y.atspi.Accessible  (Name, Role, State, children…)",
                                     "org.a11y.atspi.Action  (DoAction — a real, coordinate-free click)",
                                     "org.a11y.atspi.Component  (GetExtents — screen rectangle)",
                                     "org.a11y.atspi.Text / Value  (read and set content)"], PURPLE, 128),
    ]
    y = 110
    for title, lines, c, h in rows:
        box(p, lx, y, lw, h, title, lines, color=c, body_size=11.5)
        y += h + 44
    # middle: bus
    bx, bw2 = 510, 200
    yb = 110
    box(p, bx, yb, bw2, 60 + 88 + 128 + 92, "Accessibility bus",
        ["a private D-Bus", "", "started per session", "", "registryd tracks all", "", "apps and their object", "", "trees; clients connect", "", "once, then walk", "", "thousands of nodes"],
        color=AMBER, title_size=14)
    # right column: agent
    ax = 790
    aw = 230
    box(p, ax, 110, aw, 255, "Agent client",
        ["connects via libatspi", "or raw D-Bus", "", "1 · enumerate windows", "2 · recurse the tree", "3 · find the element", "4 · act on it directly", "5 · actions re-enter the bus",
         "", "reads text; needs no", "screenshot at all"], color=GREEN)
    box(p, ax, 400, aw, 205, "Why it wins",
        ["no cursor hijacking", "works in background", "several agents can", "share one machine",
         "", "but: stale nodes, and", "apps that expose", "nothing useful"], color=GREEN)
    # arrows
    arrow(p, lx + lw + 4, 200, bx - 6, 200, color=TEAL)
    arrow(p, lx + lw + 4, 350, bx - 6, 350, color=TEAL)
    arrow(p, bx + bw2 + 4, 220, ax - 6, 220, color=GREEN, marker="arrow-green")
    arrow(p, ax - 6, 330, bx + bw2 + 6, 330, color=GREEN, marker="arrow-green")
    note(p, 750, 318, "DoAction", GREEN, 11.5)
    note(p, bx + bw2 / 2 + 20, 516, "actions travel back down the same stack", SUB, 12)
    note(p, W / 2, H - 24, "Windows equivalent: UI Automation control patterns  ·  macOS equivalent: AXUIElement + AXPress", SUB, 13)
    p.append("</svg>")
    (OUT / "accessibility-stack.svg").write_text("\n".join(p))
    print("wrote accessibility-stack.svg")


# ------------------------------------------------------- 4. lifecycle ------
def cloud_lifecycle():
    W, H = 1100, 620
    p = header(W, H, "Lifecycle of an agent computer in the cloud",
               "Templates fork into microVMs; snapshots make stop, resume and clone nearly instant")
    # top row boxes
    bw, bh = 235, 125
    y1 = 120
    xs = [42, 302, 562, 822]
    box(p, xs[0], y1, bw, bh, "Template", ["golden disk image:", "OS + browser + agent", "runtime + VM config", "never modified"], color=PURPLE)
    box(p, xs[1], y1, bw, bh, "Launch", ["Firecracker microVM on KVM", "~125 ms class boot;", "disk = copy-on-write overlay", "on top of the template"], color=BLUE)
    box(p, xs[2], y1, bw, bh, "Run", ["agent works; every write", "lands in the overlay;", "secrets injected here,", "not into the template"], color=GREEN)
    box(p, xs[3], y1, bw, bh, "Snapshot", ["freeze: memory image +", "disk state; VM state file;", "cheap to keep, near-zero", "CPU while idle"], color=AMBER)
    for i in range(3):
        arrow(p, xs[i] + bw + 4, y1 + bh / 2, xs[i + 1] - 6, y1 + bh / 2)
    # second row: resume / clone / destroy
    bw2, bh2 = 330, 165
    y2 = y1 + bh + 70
    xr = [40, 400, 760]
    box(p, xr[0], y2, bw2, bh2, "Resume (stop → start)",
        ["restore memory pages lazily via", "userfaultfd; VMGenID changes →", "guest reseeds its RNG;", "open sockets are dead, clocks jump,",
         "so the agent must retry, not trust"], color=BLUE)
    box(p, xr[1], y2, bw2, bh2, "Clone / fork (×N)",
        ["each clone gets a fresh overlay", "off the same base + a fresh", "VMGenID; secrets are re-injected",
         "per clone — never baked into", "the shared image"], color=TEAL)
    box(p, xr[2], y2, bw2, bh2, "Destroy",
        ["delete overlay + snapshot;", "the template and siblings", "are untouched (copy-on-write", "means isolation for free)"],
        color=RED)
    # trunk + branches from Snapshot
    cx = xs[3] + bw / 2
    p.append(f'<line x1="{cx}" y1="{y1 + bh + 6}" x2="{cx}" y2="{y2 - 30}" stroke="{SUB}" stroke-width="2"/>')
    for i in range(3):
        tx = xr[i] + bw2 / 2
        path_arrow(p, f"M {cx} {y2 - 30} L {tx} {y2 - 30} L {tx} {y2 - 6}", color=SUB)
    note(p, W / 2, H - 28, "one small snapshot per branch lets a fleet of agents start from the same worked-through state", SUB, 13)
    p.append("</svg>")
    (OUT / "cloud-lifecycle.svg").write_text("\n".join(p))
    print("wrote cloud-lifecycle.svg")


# --------------------------------------------------- 5. control spectrum ---
def control_spectrum():
    W, H = 1100, 460
    p = header(W, H, "The control spectrum: five ways an agent can touch a GUI",
               "From pure pixels (most universal, most fragile) to native commands (most precise, most restricted)")
    bw, bh = 186, 232
    y = 140
    gap = 18
    xs = [49 + i * (bw + gap) for i in range(5)]
    specs = [
        ("Raw pixels", BLUE, ["screenshot →", "coordinates", "", "any app works,", "even games and", "video walls", "", "slow, imprecise,", "breaks on layout", "change"]),
        ("Pixels + Set-of-Mark", PURPLE, ["boxes drawn on the", "screenshot, model", "picks a number", "", "better grounding,", "still only what is", "visible", "", "needs draw +", "re-screenshot pass"]),
        ("DOM / a11y refs", TEAL, ["tree dump with", "stable refs:", "click(ref=e12)", "", "text-cheap, robust,", "works in background", "", "only when the app", "exposes a tree;", "refs go stale"]),
        ("Native API actions", GREEN, ["UIA patterns,", "AXPress, AT-SPI", "DoAction / Value", "", "no pointer at all,", "works minimized", "", "app must implement", "the pattern"]),
        ("CLI / protocol", AMBER, ["ssh, shell tools,", "CDP, app APIs", "", "fast, scriptable,", "testable", "", "not GUI at all —", "the endpoint of", "automation"]),
    ]
    for x, (t, c, lines) in zip(xs, specs):
        box(p, x, y, bw, bh, t, lines, color=c, title_size=13.5, body_size=11.5)
    for i in range(4):
        arrow(p, xs[i] + bw + 1, y + bh / 2, xs[i + 1] - 3, y + bh / 2, color=FAINT)
    note(p, W / 2, y + bh + 42, "most agents ship a hybrid: tree or DOM first, pixels as fallback and verifier", SUB, 13)
    p.append("</svg>")
    (OUT / "control-spectrum.svg").write_text("\n".join(p))
    print("wrote control-spectrum.svg")


if __name__ == "__main__":
    agent_loop()
    pixel_path()
    a11y_stack()
    cloud_lifecycle()
    control_spectrum()
    print("done")
