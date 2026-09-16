# TCP/IP Architecture, Design, and Implementation in Linux — Complete Study Notes

### Original explanations of Linux kernel networking internals, book's chapter structure as the guide, kernel 2.4.20 (the book's own target) explained first in every topic, followed by what's changed since

---

## What this is, and how it differs from the last two study packs

You own *TCP/IP Architecture, Design, and Implementation in Linux* (Sameer Seth & M. Ajaykumar Venkatesulu, Wiley/IEEE Computer Society Press, 2008). This is a commercially published, copyrighted technical book, so — same principle as the *Hands-On Network Programming with C* notes — everything below is written fresh, in my own words, rather than copied or closely paraphrased from the book's text.

Two things make this project genuinely different from the last two, worth understanding upfront:

1. **This book is about kernel internals, not userspace programs.** Every earlier study pack in this series had real code you could compile with `gcc` and run to see real output. Kernel networking code doesn't work that way — you can't just compile a kernel function and call it from a normal program; it lives inside the kernel itself, compiled as part of the kernel image or a loadable module, running in a completely different execution context (interrupt handlers, softIRQs, kernel process context) with its own rules. So instead of "compile and run," these notes give you **accurate, detailed walkthroughs of real kernel data structures and control flow**, written from established, well-documented knowledge of how the Linux networking stack actually works — the same kind of knowledge base as *Understanding Linux Network Internals* (O'Reilly), the kernel's own `Documentation/networking/` tree, and the kernel source itself.

2. **The book targets kernel 2.4.20 specifically** — released in 2002, meaning the book is explaining a kernel that is, as of these notes, **over 20 years old**. A great deal has changed since: RCU-based locking replacing many of that era's spinlocks/semaphores, network namespaces (allowing multiple isolated network stacks in one kernel — the foundation of every container runtime today), nftables largely superseding the netfilter/iptables framework this book describes at the kernel level, substantial `sk_buff` and TCP internals reshuffling, and modern congestion-control algorithms (BBR) alongside the Reno/Vegas-era algorithms this book covers. Per your choice, **every chapter explains the book's own 2.4.20-era design first — matching what you're actually reading — and then adds a clearly separated "What's Changed Since" section** noting the significant, well-documented shifts. The historical sections are the main event; the "what's changed" sections are meant to stop you from walking away thinking 20-year-old internals are still exactly how a current kernel works.

## A honest note on precision

Kernel source at this level of detail runs to hundreds of thousands of lines, and this book itself spends entire chapters on single subsystems. These notes aim for **genuine architectural accuracy** — real function names, real data structure fields, real control flow, real algorithms — at the depth a strong systems engineer needs to actually understand *how* and *why* the stack works the way it does. They are not a line-by-line reproduction of kernel source (which would either mean copying the book's own specific walkthrough, or reproducing large stretches of the actual kernel — neither of which is the goal here). Where a specific claim is about current (not 2.4.20-era) kernel behavior and benefits from double-checking against a live source, I've verified it against the real, current Linux kernel source on GitHub rather than relying purely on memory.

## Table of contents

| File | Covers |
|---|---|
| `01_Kernel_Basics_and_TCPIP_System_Calls.md` | Kernel architecture basics, process/interrupt context, the socket-related system call interface |
| `02_Protocols_TCP_IP_ARP_ICMP.md` | TCP/IP protocol fundamentals, ARP, ICMP — the wire formats the rest of the book implements |
| `03_BSD_Socket_Layer_Implementation.md` | How `socket()` hooks into the VFS layer, and how protocols register themselves under the generic socket interface |
| `04_Connection_Setup_Kernel_Implementation.md` | What `bind()`/`listen()`/`accept()`/`connect()` actually do inside the kernel, SYN queue vs. accept queue |
| `05_sk_buff.md` | The `sk_buff` structure — how Linux represents a packet in memory as it moves through the stack |
| `06_Packet_Movement_Across_Layers.md` | The complete journey of a packet, both directions, socket to wire and back |
| `07_TCP_Receive.md` | The TCP receive path in detail |
| `08_TCP_Send.md` | The TCP send path, segmentation, PMTU/ICMP interaction |
| `09_TCP_Memory_Management.md` | Per-socket memory accounting, receive-queue collapsing |
| `10_TCP_Timers.md` | Retransmission, delayed ACK, keepalive, zero-window-probe timers |
| `11_TCP_Core_Processing_State_Machine.md` | The TCP state machine, sliding window, Nagle's algorithm, SACK/D-SACK |
| `12_TCP_Congestion_Control.md` | Slow start, congestion avoidance, Reno/Vegas — and modern BBR |
| `13_Netlink_Sockets.md` | The kernel/userspace control-plane interface behind `netstat`, `ip route`, routing daemons |
| `14_IP_Routing_Table_FIB.md` | The routing table (FIB) implementation, multipathing, policy routing |
| `15_IP_QoS.md` | Queuing disciplines — PFIFO, CBQ, and the modern `tc`/HTB landscape |
| `16_Netfilter_Framework.md` | Netfilter hooks, and the historical iptables → modern nftables transition |
| `17_SoftIRQ_Implementation.md` | Why packet processing splits across interrupt and softIRQ context, and how it scales across CPUs |
| `18_Link_Layer_and_DMA_Ring_Buffers.md` | Device driver packet reception/transmission, DMA ring buffers, interrupt routines |
| `19_Debugging_TCP_IP_Stack.md` | The book's `lkcd` crash-dump debugging approach — and the modern tools (`crash`, `drgn`, `ftrace`, eBPF) that have largely superseded it |
| `20_FINAL_REVISION_CHEATSHEET.md` | The whole book, compressed, plus a self-test |

Onward to `01_Kernel_Basics_and_TCPIP_System_Calls.md`.