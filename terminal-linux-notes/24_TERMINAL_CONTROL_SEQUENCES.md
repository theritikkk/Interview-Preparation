# 24 — Terminal Control Sequences

> **Where this fits in the stack**
> ```
> User  →  [ TERMINAL EMULATOR interprets ]  →  PTY  →  Shell  →  Programs write plain bytes
>            ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
>            this file
> ```
> File 01 established that the terminal emulator renders pixels, not the kernel and not the shell. This file covers the actual *language* it's interpreting to know what to draw beyond plain text — and closes the loop on how `vim`, `less`, and `tmux` leave your terminal exactly as they found it on exit.

---

## 1. What is an escape sequence?

A specific byte sequence — starting with the **ESC** character (`0x1B`, octal `033`, written `\033` or `\e`) — that the terminal emulator recognizes as a **command**, not literal text to display. The convention traces directly to the VT100 and its predecessors (file 01): ESC signals "what follows is control, not printable text," and different sequences after it mean different things. **ANSI** formalized much of this, which is why the umbrella term is "ANSI escape codes."

---

## 2. Why `printf("\033[31mHello\033[0m")` changes color, byte by byte

```
\033 [ 3 1 m          Hello          \033 [ 0 m
 ESC CSI  SGR-red      literal text    ESC CSI  SGR-reset
```

- `ESC [` is the **CSI** (Control Sequence Introducer).
- `31` is a parameter meaning "set foreground color to red," in the **SGR** (Select Graphic Rendition) sub-language.
- `m` terminates an SGR sequence specifically.
- `Hello` is plain printable text, displayed normally — but *after* the terminal emulator has already updated its internal rendering state to "foreground = red," because of the sequence that just arrived.
- `\033[0m` resets all SGR attributes back to default.

> **Remember**
> `printf` here is **just writing bytes** to stdout — exactly like any other `write()` (files 09, 11). It has zero special knowledge that these particular bytes mean "color." All interpretation happens downstream, entirely inside the **terminal emulator**, reading these bytes off the PTY master (file 02). The same bytes redirected to a file (`command > file.txt`) are written completely literally, with no interpretation happening anywhere — which is exactly why `cat`-ing such a file in a context with no terminal emulator reading it (a plain text editor, for instance) shows garbled control-character junk instead of colors.

---

## 3. The SGR parameter table

| Code | Effect |
|---|---|
| `0` | Reset all attributes |
| `1` | Bold |
| `3` | Italic |
| `4` | Underline |
| `30`-`37` | Foreground: black/red/green/yellow/blue/magenta/cyan/white |
| `40`-`47` | Background, same color order |
| `90`-`97` | Bright foreground variants |
| `100`-`107` | Bright background variants |
| `38;5;N` / `48;5;N` | 256-color mode, foreground/background |
| `38;2;R;G;B` / `48;2;R;G;B` | 24-bit true-color RGB, foreground/background |

---

## 4. Cursor movement and screen clearing

| Sequence | Effect |
|---|---|
| `ESC[H` | Cursor to home (top-left, row 1, col 1) |
| `ESC[{row};{col}H` | Cursor to a specific position |
| `ESC[{n}A`/`B`/`C`/`D` | Cursor up/down/forward/back `n` cells |
| `ESC[2J` | Clear the entire screen (doesn't necessarily move the cursor — often paired with `ESC[H`) |
| `ESC[K` | Clear from cursor to end of the current line |
| `ESC[?1049h` / `ESC[?1049l` | **Enter / leave the alternate screen buffer** |

**The last pair is exactly what closes file 23's loop:** this is precisely the sequence `vim`, `less`, `tmux`, and `top` send on startup and exit, swapping to a separate virtual "page" so your original terminal content sits preserved underneath, untouched, and reappears the instant they exit and send the "leave" half of the pair.

---

## 5. Why the terminal emulator interprets these, not the kernel

Recap and sharpen files 01-02: the PTY simply carries bytes between programs and the terminal emulator. The kernel's involvement ends once it has moved bytes from the PTY slave to the master (or vice versa for input) — it does not parse ANSI sequences, and has no concept of "color" or "cursor position" for a PTY at all.

> **Common mistake**
> "The kernel draws characters on my screen." False for the overwhelmingly common case — every terminal emulator window, every SSH session — where 100% of the interpretation happens in the terminal emulator process, in userspace, with the kernel acting as a neutral byte-pipe. **The one genuine exception**, worth scoping precisely: the Linux **virtual console** (`/dev/tty1`-`6`, file 01) has no separate terminal-emulator process at all — there, the kernel's own console driver directly interprets a limited subset of these sequences itself, since there's nothing else to do it. This is the narrow, specific case where "the kernel draws the terminal" is actually true.

---

## 6. UTF-8, locale, and cursor-position math

A single **displayed** character doesn't always correspond to one byte, or even one Unicode codepoint. Multi-byte UTF-8 sequences (`é` as two bytes, many emoji as four) represent one visual character, and some characters — East Asian wide characters, many CJK glyphs — are conventionally rendered as **two terminal columns wide** despite being a single codepoint.

This means any program doing manual cursor-positioning math — the terminal emulator itself, or Readline (file 23), or `vim` — needs locale/encoding-aware logic to correctly compute "how many *columns* does this text occupy," not simply "how many bytes or characters." Getting this wrong silently desyncs cursor math from the actual visual layout — a real, recurring category of bug in terminal programs handling international text or emoji. `LANG` (file 17) is part of what determines this encoding-aware behavior.

---

## 7. Common misconceptions

| Misconception | Reality |
|---|---|
| "The kernel draws characters on my screen." | Only true for the Linux virtual console specifically; false for every PTY-based terminal — which is essentially all of them. |
| "`printf` has special code to handle color escape sequences." | It writes ordinary bytes; all interpretation happens downstream, in the terminal emulator. |
| "Redirecting colored output to a file preserves the visible color." | The raw escape bytes are written literally — visible color only reappears if that file's content is later sent to an actual interpreting terminal emulator again. |
| "One character always occupies one terminal column." | Wide CJK characters and multi-byte encodings break this assumption — column count and character count are genuinely different quantities. |

---

## 8. Try this

> **Try this**
> ```bash
> printf '\033[31mRed Text\033[0m\n'
> printf '\033[1mBold\033[0m \033[4mUnderline\033[0m\n'
> ```

> **Try this — proving it's about the reader, not the file**
> ```bash
> printf '\033[31mHello\033[0m' > out.txt
> cat out.txt        # shows the raw escape bytes as literal garbage-looking text
>                     # in some views, but rendered as RED in an actual terminal —
>                     # because THIS TIME cat's output IS going to a real terminal
>                     # emulator, which interprets the exact same bytes it
>                     # received literally from the file
> ```
> The bytes in `out.txt` never changed between these two views — only whether something downstream was interpreting them.

---

## 9. Summary

- Escape sequences are byte sequences starting with ESC that the terminal emulator — not the kernel, not the shell — interprets as commands rather than text.
- SGR codes control color/style; cursor-movement and screen-clearing sequences control the rest of the visible display.
- The alternate screen buffer (`ESC[?1049h`/`l`) is exactly how full-screen programs restore your original terminal content on exit.
- The kernel draws terminal output only for the Linux virtual console specifically — everywhere else, it's a neutral byte-pipe, and the terminal emulator does 100% of the interpretation.
- Wide/multi-byte characters mean column count and character count genuinely diverge — cursor-positioning code has to account for this explicitly.

---

## What's next

`25_INTERACTIVE_TERMINAL_PROGRAMS.md` pulls files 23 and 24 together: exactly why `vim`, `top`, `less`, and `ssh` need raw mode *and* the alternate screen *and* signal handling all at once, in a way `ls`/`cat` simply never do.