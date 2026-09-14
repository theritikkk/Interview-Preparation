# Hands-On Network Programming with C — Complete Study Notes

### Built around your book's official, MIT-licensed example code, with every program actually compiled and run

---

## What this is, and a quick note on sources

You own *Hands-On Network Programming with C* (Lewis Van Winkle, Packt, 2019). This is the same kind of deep, code-heavy study pack I built for Beej's Guide — except this book is a commercial, copyrighted textbook, so it needed a different approach to build honestly:

- **The explanations in these notes are written entirely from scratch, in my own words.** None of the book's prose is reproduced or paraphrased — everything here is my own original explanation of the same underlying (and very well-documented, standard) networking concepts: TCP/UDP sockets, DNS, HTTP, TLS, SSH, and so on.
- **The code is pulled directly from the book's own official GitHub repository** — [`PacktPublishing/Hands-On-Network-Programming-with-C`](https://github.com/PacktPublishing/Hands-On-Network-Programming-with-C) — which the book's own appendix and README both confirm is released under the **MIT License** (Copyright (c) 2019 Lewis Van Winkle). Every source file in that repo carries the full MIT license text at its top, which is preserved intact wherever the code appears below. MIT is about as permissive as licenses get: you can use, copy, modify, and redistribute this code freely, including commercially, as long as the copyright notice stays attached — which it does, throughout.
- **Every program was actually compiled with `gcc -Wall` and run for real** in a Linux sandbox — real TCP/UDP traffic, a real self-signed-certificate TLS handshake, a real local SSH server. Where the sandbox's network restrictions or missing IPv6 support get in the way of a live demo (a couple of cases — flagged clearly when they come up), I say so directly rather than faking output.

## Table of contents

| File | Covers |
|---|---|
| `01_Networking_Fundamentals.md` | OSI/TCP-IP layers, addresses, subnetting, listing your own network adapters |
| `02_Socket_API_Basics.md` | Cross-platform (Winsock + Berkeley) socket setup, your first working TCP server |
| `03_TCP_Deep_Dive.md` | Blocking vs. non-blocking, `select()`, a real multi-client TCP chat room |
| `04_UDP_Connections.md` | Connectionless sockets, UDP client/server pairs |
| `05_DNS_and_Hostname_Resolution.md` | `getaddrinfo()`/`getnameinfo()`, then the raw DNS wire protocol by hand |
| `06_HTTP_Client.md` | Building an HTTP client from raw sockets, URL parsing, GET and POST |
| `07_HTTP_Server.md` | A real multi-client HTTP file server using `select()` |
| `08_Sending_Email_SMTP.md` | The SMTP protocol, talking to a mail server by hand |
| `09_TLS_and_HTTPS_Client.md` | Encryption fundamentals, OpenSSL, a real HTTPS client |
| `10_HTTPS_Server.md` | Self-signed certificates, a real TLS server — client and server both run live |
| `11_SSH_with_libssh.md` | The SSH protocol, `libssh`, connecting to a **real local SSH server** |
| `12_Network_Monitoring_and_Security.md` | Raw sockets, traceroute mechanics, packet capture — concepts (no book code for this chapter) |
| `13_Pitfalls_and_Tips.md` | Every gotcha the book warns about, each one reproduced live: crashed servers, Nagle's algorithm, `SIGPIPE`, and more |
| `14_IoT_Networking.md` | Connectivity options, hardware choices, protocols for embedded/IoT — concepts (no book code for this chapter) |
| `15_FINAL_REVISION_CHEATSHEET.md` | The whole book compressed, plus a self-test |

Chapters 12 and 14 don't have accompanying code in the book's own repository (they're conceptual/survey chapters), so those two files are written the same way as everything else — original explanation — just without a "compile and run" section.

## One more environment note, carried over from last time

This sandbox has **no working IPv6 stack** (no `::1` loopback, `socket(AF_INET6, ...)` fails outright). A few of this book's examples are IPv6-specific or IPv6-dual-stack demos; where that's the case, I show you the real error this environment produces, explain exactly why, and describe what you'd see instead on a normal machine (almost any real computer has working IPv6 loopback, so this is purely a sandbox quirk, not a code problem).

Onward to `01_Networking_Fundamentals.md`.