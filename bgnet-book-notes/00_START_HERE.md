# Beej's Guide to Network Programming — Complete Study Notes

### Built from your uploaded PDF (v3.2.11, 126 pages), with every program actually compiled and run

---

## What this is

You uploaded a copy of **Beej's Guide to Network Programming Using Internet Sockets** — the single most-used free introduction to writing networked programs in C. These notes rebuild the entire guide as a study pack:

- Every idea explained from scratch, in plain English, assuming you know basic C (variables, functions, pointers, structs) but **nothing** about networking.
- Every code example the book contains, pulled directly out of your PDF.
- Every one of those programs **actually compiled with `gcc -Wall -Wextra` and run for real** in a Linux sandbox — client talking to server, over real TCP and UDP sockets on `localhost`. The "output" you'll see in these notes is not guessed or remembered — it's copy-pasted from real terminal sessions I ran while building this.
- A running list of **gotchas** — including a couple of real bugs/traps I hit while running the book's own examples, which are exactly the kind of thing that costs beginners hours.
- A final cheat-sheet + a short quiz for revision.

## A quick legal note

The book's own copyright page grants **all the C source code to the public domain** — you can use it in your own projects with zero restriction. The *prose explanations* in the original book are copyrighted (CC-BY-NC-ND), which is why every explanation in these notes is written fresh, in my own words, rather than copied from the PDF.

## How the files are organized

Read them in this order — each one builds on the last:

| File | Covers | Book chapters |
|---|---|---|
| `01_Sockets_Fundamentals.md` | What a socket actually is, stream vs. datagram, the layers of a network connection | Ch 1–2 |
| `02_IP_Addresses_and_Structs.md` | IPv4 vs IPv6, all the `sockaddr` struct types, byte order, address conversion | Ch 3–4 |
| `03_Core_System_Calls.md` | `getaddrinfo`, `socket`, `bind`, `connect`, `listen`, `accept`, `send`/`recv` — the actual function calls, with `showip.c` run live | Ch 5 |
| `04_TCP_Client_Server.md` | A full working TCP server + client (`server.c` / `client.c`), run against each other | Ch 6.1–6.2 |
| `05_UDP_Datagram_Sockets.md` | Connectionless sockets (`listener.c` / `talker.c`), run against each other, plus a real bug I hit and fixed | Ch 6.3 |
| `06_Advanced_IO_Multiplexing.md` | Handling many clients at once with `poll()` and `select()` — includes a real 3-client chat session | Ch 7.1–7.3 |
| `07_Serialization_and_Broadcast.md` | Partial sends, packing structured data for the wire, IEEE-754 float packing, UDP broadcast | Ch 7.4–7.7 |
| `08_Common_Questions.md` | The book's FAQ chapter, explained | Ch 8 |
| `09_ManPages_Quick_Reference.md` | Every socket function's signature, parameters, and return values in one place | Ch 9 |
| `10_FINAL_REVISION_CHEATSHEET.md` | The whole guide on 2–3 pages: diagrams, tables, common mistakes, self-test | Everything |

## Before you start: setting up to run C socket code yourself

You don't need anything exotic — this is plain C on Linux.

```bash
# You need a C compiler. On Debian/Ubuntu:
sudo apt-get install gcc

# Compile any example (always compile with warnings on — see below):
gcc -Wall -Wextra -o server server.c

# Run it:
./server
```

**Always compile with `-Wall -Wextra`.** These flags turn on warnings for things like unused variables, mismatched types, and format-string mistakes — exactly the class of bug that's easy to introduce in socket code and painful to debug later. Every program in these notes compiles with **zero warnings** under these flags; if you modify the code and suddenly see warnings, that's your first clue something's wrong.

**Testing client/server pairs on one machine:** every "network" demo in these notes runs both programs on the same machine, talking over `localhost` (`127.0.0.1`). You don't need two computers or an actual network to follow along — open two terminal windows (or run the server with `&` to background it), and you can reproduce every single run case yourself.

## One environment quirk you should know about upfront

The sandbox these notes were built in has **no working IPv6 loopback** (common in containerized environments). One of the book's own examples (`listener.c`/`talker.c`) defaults to IPv6 and fails here with `Address family not supported by protocol`. This isn't a bug in the book or in these notes — it's explained in full, with the fix, in `05_UDP_Datagram_Sockets.md`. I'm flagging it now so that if you see IPv6 failures on your own machine, you immediately know it's an environment issue, not a code issue, and where to look.

Onward to `01_Sockets_Fundamentals.md`.