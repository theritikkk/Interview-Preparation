# 45 — The Terminal as a State Machine

> **Where this fits in the stack**
> ```
> bytes from the PTY  →  [ PARSER: a state machine ]  →  actions  →  terminal state  →  rendered pixels
>                          ^^^^^^^^^^^^^^^^^^^^^^^^^
>                          this file
> ```
> File 24 covered what escape sequences *mean* — `ESC[31m` is red. This file covers the question underneath: how does a terminal *recognize* them in a stream where text and commands are mixed together byte by byte? The answer — a precisely specified state machine — turns out to explain terminal multiplexers (files 46-47), why a single missing byte can wreck a whole screen, and why "just parse the colors with a regex" is a classic mistake. Every trace below was produced by a working implementation (Section 10), not reasoned out by hand.

---

## 1. The problem: control and data share one stream

Compare this with a protocol like HTTP, which separates *kinds* of information structurally: a request line, headers, a body — you know what a piece of data is by where it sits in the message.

A terminal has none of that. A program that wants the word `error` printed in red has nothing but the PTY byte stream (file 02), so the instruction has to be spliced into the text itself:

```
27 91 51 49 109   e r r o r   27 91 48 109
ESC [  3  1  m                ESC [  0  m
```

Five bytes meaning "switch to red," then the word, then four meaning "reset" — which is why file 24 compared them to opening and closing HTML tags. There is no metadata layer and no command layer: **the only way to know whether a byte is text or part of an instruction is what bytes came before it.**

---

## 2. Why this needs a state machine (and why a regex fails)

A **state machine** is a program whose reaction to the next input depends on the *state* the previous inputs left it in. The same byte can mean entirely different things:

| Byte | In `ground` state | Inside a CSI sequence |
|---|---|---|
| `0x6D` (`m`) | Print the letter `m` | The *final byte* — "apply these style parameters now" |
| `0x48` (`H`) | Print the letter `H` | The final byte of cursor positioning |

A regular expression sees a flat string; it has no notion of "what state were we in when this byte arrived," and — critically — real terminal input can be interrupted, cancelled, malformed, or arrive split across reads. The DEC parser handles all of that by construction.

---

## 3. Williams' VT500 parser

Paul Flo Williams' reverse-engineered state diagram of DEC's VT500-series terminals (vt100.net/emu/dec_ansi_parser) is the reference model most serious terminal emulators implement or descend from. His two stated design properties are precisely worth quoting in spirit:

- **Completeness** — it specifies an action and a transition for *every* incoming byte in *every* state, including the C0 controls and the 8-bit range. (It deliberately does *not* cover everything an emulator needs — character-set mapping and cursor-movement semantics are outside it.)
- **Correctness** — fed random or deliberately pathological bytes, it is claimed to behave visibly the same as DEC's real 8-bit terminals (VT220 through VT525).

**Fourteen states:**

| State | Purpose |
|---|---|
| `ground` | Initial state; print characters, execute C0 controls |
| `escape` | Entered on ESC; decides what kind of sequence follows |
| `escape intermediate` | ESC followed by intermediate bytes (`0x20`-`0x2F`) |
| `csi entry` | Just saw `CSI` (`ESC [`); handles only the *first* byte (where private markers `<=>?` may appear) |
| `csi param` | Collecting parameters (digits and `;`) |
| `csi intermediate` | Collecting intermediate bytes |
| `csi ignore` | Malformed sequence: consume until a final byte, then dispatch *nothing* |
| `dcs entry` / `dcs param` / `dcs intermediate` / `dcs ignore` | The same shape, for Device Control Strings |
| `dcs passthrough` | After the DCS header is recognized: hand the data to a handler |
| `osc string` | Operating System Command strings (window title, etc.) |
| `sos/pm/apc string` | Strings the VT500 ignores entirely |

**Fourteen actions:** `ignore`, `print`, `execute`, `clear`, `collect`, `param`, `esc_dispatch`, `csi_dispatch`, `hook`, `put`, `unhook`, `osc_start`, `osc_put`, `osc_end`.

**Byte classes** (hex, exactly as the diagram labels them):

| Bytes | Class | Typical handling |
|---|---|---|
| `00`-`17`, `19`, `1C`-`1F` | C0 controls | **Executed** (even in the *middle* of a CSI sequence; *ignored* inside DCS/OSC strings) |
| `18`, `1A` (CAN, SUB) | Cancel | Anywhere: abort the sequence, back to `ground` |
| `1B` (ESC) | Escape | Anywhere: cancel whatever is in progress, enter `escape` |
| `20`-`2F` | Intermediates | `collect` |
| `30`-`39`, `3B` | Digits, `;` | `param` |
| `3A` (`:`) | Reserved | Forces the sequence into an *ignore* state |
| `3C`-`3F` (`< = > ?`) | Private markers | `collect` if it is the *first* parameter byte; anywhere else, forces *ignore* |
| `40`-`7E` | Final bytes | `csi_dispatch` / `esc_dispatch` — the sequence is complete |
| `7F` (DEL) | — | Ignored |
| `80`-`9F` | C1 controls | Cancel anything in progress and execute (`9B` = CSI, `90` = DCS, `9D` = OSC, ...) |
| `A0`-`FF` | GR | Treated exactly like `20`-`7F` in all states |

**Order of actions on a transition:** exit action of the old state → the transition's own action → entry action of the new state. (`clear` runs on *entry* to `escape`, `csi entry`, and `dcs entry`, which is what makes aborted sequences get forgotten.)

---

## 4. Worked traces

Each trace below is real output from the parser in Section 10.

**`ESC [ 3 1 m` — red foreground.**

```
0x1B      [ground]        → escape
0x5B '['  [escape]        → csi entry (clear params)
0x33 '3'  [csi-entry]     → csi param  (param action)
0x31 '1'  [csi-param]     → collect digit
0x6D 'm'  [csi-param]     → FINAL: csi_dispatch final='m' params=[31]
```
The parser reports *what was recognized* — final `m`, parameters `[31]` — and a separate layer decides that "SGR 31" means "red foreground." The parser itself knows nothing about colors.

**`ESC [ ? 1 0 4 9 h` — enter the alternate screen** (file 24):
```
csi_dispatch final='h' private=? params=[1049]
```
The `?` is collected as a **private marker** at `csi entry`; the dispatcher then uses marker + final byte together to select "DEC private mode set." Without the marker, `CSI 1049 h` would be a different (and meaningless) request.

**`ESC [ 5 SP q` — change cursor shape** (the space is `0x20`, an *intermediate*):
```
csi_dispatch final='q' inter=" " params=[5]
```
Intermediates are *part of the function's identity* — `CSI 5 q` and `CSI 5 SP q` are different commands.

---

## 5. Error recovery is part of the specification

ANSI X3.64 deliberately leaves error handling undefined — a terminal is a "closed box that doesn't normally report errors in its input stream to the host," so DEC had to pick recovery rules, and Williams' document records them. Several behaviors, each **confirmed by running the parser:**

| Input | Behavior | Why it matters |
|---|---|---|
| `ESC [ 2` **LF** `C` | LF *executes* mid-sequence, then `C` dispatches: result is "cursor forward 2" **and** one line down | Williams' own footnote example: this "cursor moves two right, one down" behavior is *not* in X3.64 — it's simply what the VT100 does, and people unknowingly rely on it |
| `ESC [ 3 : 1 m` | Colon forces `csi ignore`; at the final byte, **nothing is dispatched** and the parser returns to ground | The sequence is swallowed, not printed as garbage |
| `ESC [ 5 SP 3 q` | A parameter digit *after* an intermediate is an error → `csi ignore` | Same swallow-and-discard |
| `ESC [ 3 ; 1` then a **new** `ESC [ 2 J` | `clear` on entry discards the half-built first sequence; `J` dispatches with `params=[2]`, **not** `[3,1,2]` | Aborted sequences must leave no residue |
| `ESC [ 3` **CAN** `1 m` | CAN cancels; the `1` and `m` are then printed as ordinary text | |
| `ESC [ ; 5 H` | `params=[0,5]` — a blank parameter and a zero are both "use the default" | Many functions default to `1`, not `0` — a common implementer misconception |

Two further DEC rules: **at most 16 parameters** are kept (extras silently dropped), and **later parameters override earlier ones** — `CSI 7;0 m` does *not* leave reverse video on, because the `0` resets what the `7` set.

> **Remember**
> Williams' lesson from his footnote: people who write terminal programs don't always read the manual, and may depend on observed behavior without realizing it's an *error-recovery* artifact. His conclusion — "if you're going to emulate a real terminal, you should match all observable behavior" — is why this state machine has to be exact rather than "close enough."

---

## 6. The cascade: why one missing byte can ruin a screen

Take `ESC [ 3 1 m Hello` and **delete only the final `m`**. The parser is now in `csi param` when `H` (`0x48`) arrives — and `0x48` is in the final-byte range `40`-`7E`. Verified output:

```
csi_dispatch final='H' params=[31]     ← "move the cursor to row 31"
print 'e'  print 'l'  print 'l'  print 'o'
```
`Hello` is no longer text. The `H` was *consumed as the command that ends a cursor-positioning sequence*, and `ello` is printed somewhere else on the screen entirely. One missing byte did not lose one character — it **changed the meaning of the byte after it**.

A second, different failure: delete the *reset* `ESC [ 0 m` instead, and the red style set by the first sequence simply **never gets cleared**, tinting every later character. The damage comes from state that persists and was never undone.

This is exactly why the cut-and-replay experiment in Section 12 produces a wrecked screen rather than a slightly damaged one.

---

## 7. Strings: DCS, OSC, and APC

Not everything is a short sequence. Some functions carry arbitrary payloads, introduced by a start code and ended by **ST** (String Terminator, `ESC \` in 7-bit form or `9C`):

| Introducer | Name | Examples |
|---|---|---|
| `ESC P` / `90` | DCS, Device Control String | Sixel graphics; tmux's passthrough wrapper (file 46) |
| `ESC ]` / `9D` | OSC, Operating System Command | Setting the window title; hyperlinks; clipboard access |
| `ESC _` / `9F` | APC, Application Program Command | The Kitty graphics protocol |
| `ESC ^` / `9E`, `ESC X` / `98` | PM, SOS | Privacy message; start of string |

**DCS** works in two parts: the header is parsed like a CSI sequence, and when its final byte arrives the `hook` action selects a handler; every following byte goes to that handler via `put` until the string ends, when `unhook` tells it to finish. This is the design that lets *complicated* protocols plug in without bloating the main machine.

**The ESC-then-backslash subtlety.** ESC cancels a string in progress, so the `\` that follows it is effectively a no-op. Williams spent a long time unsure whether the recognizer needed one byte of lookahead to tell if `ESC \` was coming — and settled it empirically: on a real VT330 running ReGIS, sending a lone ESC made the graphics cursor vanish *immediately*, proving the string had already ended before `\` arrived.

> **Under the hood — why unsupported graphics fail silently**
> DEC's VT500 *ignores* APC, PM, and SOS strings entirely — everything between the introducer and ST is discarded. That rule is exactly why a terminal that doesn't understand the Kitty graphics protocol (APC-based) shows *nothing* for an image rather than spewing base64 across the screen. Conversely, an emulator that *fails* to follow this rule bleeds control strings into visible output. Mitchell Hashimoto's libghostty announcement cites Apple's Terminal.app as exhibiting exactly that for DCS sequences, and JetBrains' `jediterm` as mishandling *intermediates* so that the widely supported "change cursor shape" sequence swallowed a character (his post doesn't name the sequence; the standard cursor-shape function, DECSCUSR, is `CSI Ps SP q` — an intermediate-bearing sequence like Section 4's third trace — so that is the likely mechanism, but treat that link as my inference). These are his reports about those products — worth knowing as illustrations of how easily this machine is implemented subtly wrong, not as verified current behavior.

---

## 8. Where real terminals go beyond the DEC machine

The VT500 diagram is the *baseline*. Modern emulators deliberately extend it — which is why "matches the DEC diagram" and "matches xterm/Ghostty/Kitty" are not the same claim:

- **Colon sub-parameters.** DEC's rule sends `:` to `csi ignore` (Section 5). Modern emulators instead accept it as a sub-parameter separator — used for styled underlines (`CSI 4:3 m`) and colon-form RGB (`CSI 38:2::R:G:B m`). A strict DEC parser would silently drop both.
- **BEL (`0x07`) as an OSC terminator.** In the DEC diagram, C0 controls inside an OSC string are *ignored*, so BEL would not end it; xterm and many others accept BEL anyway, and countless programs rely on that (`printf '\033]0;title\007'`).
- **xterm-originated private modes.** Alternate screen (`1049`), bracketed paste (`2004`), SGR mouse reporting (`1006`) are not DEC inventions; they are conventions that grew up around xterm and became de facto standards.

Hashimoto's point in announcing libghostty is that even "style-only" parsing is vastly more intricate than the `\e[31m`-and-`\e[41m` regex most web consoles implement — RGB alone appears in several formats, and he notes general-purpose terminals (Ghostty itself, nine months before that post) have also gotten parts of it wrong.

> **Common mistake**
> "Arrow keys are parsed by this machine." They travel the *opposite* direction. This machine parses bytes the **terminal receives** (program output). Key presses are bytes the terminal **sends**, and are interpreted by the *application's own* input handling — Readline, `vim` (file 23). One link between them: an application can send `CSI ? 1 h` (DEC private mode 1, "application cursor keys") to the terminal, which then changes what the Up arrow sends — `ESC O A` instead of the normal-mode `ESC [ A`. Try it in Section 11.

---

## 9. Terminal state — and why a terminal is deterministic

As the parser consumes bytes, its actions mutate the **terminal state**:

- the grid of cells, each holding a character and its style
- cursor position and the currently active style ("pen")
- which modes are on (alternate screen, cursor-key mode, and so on)
- the scrollback — lines that have scrolled off the top
- charset mappings, tab stops, scroll margins

**What you see in the window is a rendering of that state at an instant.**

How big is it? For a 110×26 terminal at a deliberately generous 16 bytes per cell: 110 × 26 = 2,860 cells × 16 B = **45,760 B** ≈ 46 KB for the visible screen. With 10,000 lines of scrollback at 110 cells per line: 10,000 × 110 × 16 B = **17,600,000 B** ≈ 17.6 MB. Total ≈ **18 MB** — a rough upper bound; real emulators compress heavily, since most cells are blank and most lines share styling.

**Determinism.** The parser has no randomness and no clock. Feed the same bytes, in the same order, to the same initial state, at the same geometry and configuration, and you get **exactly the same final state**. So a recorded byte stream is a *log*, and a screen is a *replay* of that log.

> **Interview answer**
> "A terminal emulator is a deterministic state machine: its state is the grid, cursor, styles, modes, and scrollback, and its input is the byte stream. Identical input to identical initial state yields identical state — which means a recording of the bytes can be replayed into any number of terminals and they will all converge on the same screen, with no synchronization between them. That property is the foundation of how modern multiplexers can replicate terminal state across clients (file 47)."

**What determinism does *not* promise** — worth being exact about:

- **Geometry.** Replaying the same bytes into a *different* number of columns gives a different screen: wrapping and cursor-relative output depend on width.
- **Initial state.** Replay must start from the same state (a blank screen, or an agreed snapshot).
- **Queries and replies.** Programs sometimes *ask* the terminal questions (cursor position, device attributes) and react to the *replies* — which travel the input direction. Replaying only the output does not reproduce the replies' influence on the original program's behavior.
- **Implementation differences.** "Same bytes → same state" is guaranteed *within one implementation*. Two emulators agree only to the extent both implement a sequence identically — see Section 8.

---

## 10. A working parser (the source of every trace above)

This is a faithful **subset** of Williams' machine: 7-bit input; `ground`, `escape`, `escape intermediate`, and all four CSI states. It deliberately does **not** implement DCS/OSC/SOS/PM/APC strings or the 8-bit C1/GR ranges — those are left as an extension.

```c
/* Subset of Paul Flo Williams' VT500 parser: 7-bit input; ground, escape,
 * escape-intermediate, csi entry/param/intermediate/ignore.
 * NOT implemented: DCS/OSC/SOS/PM/APC strings, 8-bit C1 (80-9F), GR (A0-FF). */
#include <stdio.h>

enum st { GROUND, ESCAPE, ESC_INTER, CSI_ENTRY, CSI_PARAM, CSI_INTER, CSI_IGNORE };
static const char *names[] = {"ground","escape","escape-intermediate",
                              "csi-entry","csi-param","csi-intermediate","csi-ignore"};
static enum st state = GROUND;
static int params[16], nparams, cur, seen;
static char inter[4]; static int ninter; static char priv;

static void clear(void) { nparams = cur = seen = ninter = 0; priv = 0; }
static void push(void)  { if (nparams < 16) params[nparams++] = cur; cur = 0; }
static void collect(unsigned char c) {
    if (c >= 0x3C && c <= 0x3F) priv = c;               /* private marker */
    else if (ninter < 3) inter[ninter++] = c;
}
static void go(enum st s) { state = s; }
static int is_c0(unsigned char c) { return c <= 0x17 || c == 0x19 || (c >= 0x1C && c <= 0x1F); }

static void csi_dispatch(unsigned char f) {
    if (seen) push();
    printf("  csi_dispatch final='%c' private=%c inter=\"%.*s\" params=[", f,
           priv ? priv : '-', ninter, inter);
    for (int i = 0; i < nparams; i++) printf(i ? ",%d" : "%d", params[i]);
    printf("]  (0 = default)\n");
}

static void feed(unsigned char c) {
    if (c == 0x18 || c == 0x1A) { printf("  CAN/SUB: cancel -> ground\n"); go(GROUND); return; }
    if (c == 0x1B)               { go(ESCAPE); clear(); return; }   /* ESC cancels anything */
    if (c >= 0x80) return;                                            /* subset: ignore 8-bit */
    switch (state) {
    case GROUND:
        if (is_c0(c)) printf("  execute 0x%02X\n", c);
        else if (c >= 0x20 && c <= 0x7E) printf("  print '%c'\n", c);
        break;                                                        /* 7F ignored */
    case ESCAPE:
        if (is_c0(c)) printf("  execute 0x%02X\n", c);
        else if (c >= 0x20 && c <= 0x2F) { collect(c); go(ESC_INTER); }
        else if (c == '[') { go(CSI_ENTRY); clear(); }
        else if (c == 'P' || c == ']' || c == 'X' || c == '^' || c == '_')
            { printf("  (string introducer 0x%02X - not in this subset)\n", c); go(GROUND); }
        else if (c >= 0x30 && c <= 0x7E) { printf("  esc_dispatch final='%c'\n", c); go(GROUND); }
        break;
    case ESC_INTER:
        if (is_c0(c)) printf("  execute 0x%02X\n", c);
        else if (c >= 0x20 && c <= 0x2F) collect(c);
        else if (c >= 0x30 && c <= 0x7E) { printf("  esc_dispatch final='%c' inter=\"%.*s\"\n", c, ninter, inter); go(GROUND); }
        break;
    case CSI_ENTRY: case CSI_PARAM: case CSI_INTER: case CSI_IGNORE:
        if (is_c0(c)) { printf("  execute 0x%02X   (C0 runs MID-sequence)\n", c); break; }
        if (c == 0x7F) break;
        if (state == CSI_IGNORE) { if (c >= 0x40 && c <= 0x7E) { printf("  (sequence ignored, no dispatch) -> ground\n"); go(GROUND); } break; }
        if (c >= 0x40 && c <= 0x7E) { csi_dispatch(c); go(GROUND); }
        else if (c >= 0x20 && c <= 0x2F) { if (state == CSI_INTER || state == CSI_ENTRY || state == CSI_PARAM) { collect(c); go(CSI_INTER); } }
        else if (c == 0x3A) go(CSI_IGNORE);
        else if (c >= 0x3C && c <= 0x3F) { if (state == CSI_ENTRY) { collect(c); go(CSI_PARAM); } else go(CSI_IGNORE); }
        else if ((c >= 0x30 && c <= 0x39) || c == 0x3B) {
            if (state == CSI_INTER) { go(CSI_IGNORE); break; }       /* param after intermediate = error */
            go(CSI_PARAM); seen = 1;
            if (c == 0x3B) push(); else { cur = cur * 10 + (c - '0'); if (cur > 16383) cur = 16383; }
        }
        break;
    }
}

int main(void) {
    int c, last = -1;
    while ((c = getchar()) != EOF) {
        enum st before = state;
        if (c >= 0x20 && c < 0x7F) printf("0x%02X '%c'  [%s]\n", c, c, names[before]);
        else                       printf("0x%02X      [%s]\n", c, names[before]);
        feed((unsigned char)c); (void)last;
    }
    return 0;
}
```

Compile and feed it any of Section 5's cases: `gcc -o vtparse vtparse.c && printf '\033[31Hello' | ./vtparse`.

---

## 11. Common misconceptions

| Misconception | Reality |
|---|---|
| "Parsing ANSI colors is just matching `\e[...m`." | Colors are one tiny corner of a machine with 14 states, intermediates, private markers, cancellation, and string modes — a regex has no notion of state or cancellation. |
| "Terminal input has a command layer and a data layer, like HTTP." | One undifferentiated byte stream; only parser *state* distinguishes control from text. |
| "A truncated sequence just loses a character." | It changes the meaning of what follows — Section 6's `Hello` became a cursor move. |
| "The VT500 diagram is exactly what Ghostty/xterm/Kitty implement." | It's the baseline; real emulators extend it (colon sub-parameters, BEL termination, xterm private modes). |
| "Blank and zero parameters are different, and zero is the default." | Both mean "use the default," and that default is frequently `1`. |
| "Same bytes always means same screen." | Only for the same initial state, geometry, and implementation — and only for the output direction. |
| "Arrow-key sequences are parsed by the terminal's parser." | They flow the other way and are parsed by the *application*. |

---

## 12. Try this

> **Try this — see an escape sequence as bytes**
> ```
> cat -v
> ```
> Press the Up arrow: `^[[A` appears — `^[` is ESC (`0x1B`). Press `Ctrl+D` to exit.

> **Try this — change what the Up arrow sends**
> ```bash
> printf '\033[?1h'      # application cursor keys ON (a DEC private mode set)
> cat -v                  # now press Up: you see ^[OA, not ^[[A
> printf '\033[?1l'      # turn it back OFF — run this after Ctrl+D
> ```
> The *program output* `CSI ? 1 h` was parsed by your terminal's state machine and flipped a mode; that mode then altered what your *keyboard* sends. If arrow keys behave oddly afterward, the final `printf` resets them.

> **Try this — record, replay, then corrupt**
> ```bash
> script -q /tmp/rec          # do a few colored commands (ls --color, etc.), then 'exit'
> cat /tmp/rec                # replay into this terminal
> # open a second terminal window and run 'cat /tmp/rec' there too — identical screens
> head -c 200 /tmp/rec > /tmp/cut; tail -c +260 /tmp/rec >> /tmp/cut   # remove ~60 bytes from the middle
> cat /tmp/cut                # the damage is rarely local
> reset                       # restore your terminal afterward
> ```

> **Try this — build the parser**
> Compile Section 10's program and reproduce every row of Section 5's table. Then extend it: add the `osc string` state (remember that C0 controls are *ignored* there per DEC, and decide whether to accept BEL as a terminator as xterm does).

---

## 13. macOS note

The parsing model is operating-system independent — it is a property of terminal *emulators*, not kernels. Differences are per emulator (Terminal.app, iTerm2, Ghostty, Kitty), not per OS. Full comparison: file 43.

---

## 14. Sources

Paul Flo Williams, *A parser for DEC's ANSI-compatible video terminals*, vt100.net/emu/dec_ansi_parser (the state diagram, byte classes, action definitions, and the X3.64 error-recovery cases); Mitchell Hashimoto, *Libghostty Is Coming* (September 22, 2025) for the implementation-bug examples and libghostty-vt's scope (a zero-dependency C library for parsing sequences and maintaining terminal state, then in alpha with a Zig API available and the C API still being defined — check its current status before depending on it). All parser traces were produced by the Section 10 program.

---

## 15. Summary

- Terminal control and data share one byte stream, so only parser *state* distinguishes them.
- Williams' VT500 machine has 14 states and 14 actions, specifies every byte in every state, and *includes error recovery* — which real programs unknowingly depend on.
- Final bytes (`40`-`7E`) dispatch; private markers, intermediates, and parameters are collected first; ESC/CAN cancel; C0 controls execute even mid-sequence.
- A truncated sequence doesn't lose a byte — it can turn the next byte into a command.
- Real emulators extend the DEC baseline (colon sub-parameters, BEL termination, xterm private modes).
- A terminal is deterministic — same initial state, geometry, and bytes yield the same screen — with specific, nameable limits.

---

## What's next

`46_MULTIPLEXER_INTERNALS.md` applies this directly: a multiplexer is itself a full terminal emulator, running this state machine on every pane's output — and what that costs, and who ends up owning what, is the story of tmux and zellij.
