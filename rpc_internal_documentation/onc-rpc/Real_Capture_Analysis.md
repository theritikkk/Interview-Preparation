# Analysis of a Real Capture: Your Own `tcpdump` Run

### Every RPC packet from your actual `rpc_capture.pcap`, decoded and verified — plus one important lesson about noise in real captures

---

## First: your capture contains traffic that ISN'T the RPC demo — and that's a real, useful lesson

Look at the very first packets in your dump: a 4011-byte and a 3113-byte exchange between `localhost.64216` and `localhost.64747`, using TLS record headers (`17 03 03 ...`), starting *before* your programs even ran, and continuing after they finished. That's **some other application on your Mac** using a local loopback socket during the same window `tcpdump` was capturing — almost certainly a browser helper process, a sync client, or an editor extension, not anything to do with `portmapper`/`demo_service`/`onc_client`.

`tcpdump -i lo0` captures *every* connection on the loopback interface, indiscriminately — it has no idea which traffic you actually care about. **This is exactly why real-world packet analysis always starts with a filter.** Next time, capture only what you want:
```bash
sudo tcpdump -i lo0 -w rpc_capture.pcap 'port 9111 or portrange 49269-49999'
```
(adjust the port range to whatever your service's port turns out to be — or just filter on `port 9111` and manually note the service's ephemeral port from its own printed output, then re-filter when reading back with `-r rpc_capture.pcap port 9111 or port <that port>`). For this analysis, I filtered by hand instead — real RPC traffic only ever touches port `9111` or the ephemeral ports your programs report — everything else in your dump was ignored.

## The four real connections, decoded and verified

Once the noise is filtered out, your capture contains exactly four real TCP connections — matching exactly the four connections the demo's own source code makes. Every value below was checked with an actual byte-decoder, not read by eye.

### Connection A — the service registers itself (`:49270 → :9111`)

```
CALL:  xid=1  program=0x186a0(100000)  version=2  procedure=1(PMAPPROC_SET)
       args: prog=0x20001  vers=1  port=49269
REPLY: xid=1  status=0(SUCCESS)  result=1 (bool true)
```
This is your `demo_service` telling the real portmapper program (`100000`, exactly RFC 1833's number) "register me at port 49269" — and `49269` is sitting right there in the raw argument bytes, not guessed.

### Connection B — the client asks where the service is (`:49271 → :9111`)

```
CALL:  xid=7  program=0x186a0(100000)  version=2  procedure=3(PMAPPROC_GETPORT)
       args: prog=0x20001  vers=1
REPLY: xid=7  status=0  result=49269
```
Same portmapper, different procedure (`3` = `GETPORT`, not `SET`) — and the reply's result is `49269`, **the identical number** Connection A registered. Two completely independent connections, at two different real times, agreeing on the same number — this is the portmapper mechanism working, captured directly on the wire.

### Connection C — the client connects DIRECTLY to the service (`:49272 → :49269`)

```
CALL:  xid=8  program=0x20001  version=1  procedure=1(PROC_ADD)
       args: a=3  b=4
REPLY: xid=8  status=0  result=7
```
Notice the destination port: `49269` — the exact number Connection B just returned, and nowhere else in the program's source. `3 + 4 = 7`, confirmed in the raw reply bytes.

### Connection D — the fragmented call (`:49273 → :49269`)

```
Fragment 1 header: 00 00 00 10  → top bit CLEAR, length 16
Fragment 1 body:   xid=99  CALL  program=0x20001  version=1        (16 bytes)
Fragment 2 header: 80 00 00 10  → top bit SET,   length 16
Fragment 2 body:   procedure=1  arg_len=8  a=111  b=222             (16 bytes)

Reassembled:  xid=99  CALL  program=0x20001  version=1  procedure=1(PROC_ADD)  args: a=111 b=222
REPLY:        xid=99  status=0  result=333
```
`111 + 222 = 333`, and the two fragment headers are **exactly** the two bit patterns RFC 5531 describes: identical except for that one top bit.

## The 200ms gap — measured cleanly, with zero ambiguity

The earlier trace-level analysis (built from two separate `strace` processes) had to hedge about comparing timestamps across independently-clocked traces. **Your capture doesn't have that problem** — `tcpdump` timestamps every packet with the same clock, so this comparison is completely unambiguous:

```
13:11:01.420820   fragment 1's body leaves the wire
13:11:01.620985   fragment 2's header appears on the wire
                  ────────────────────────────────────
                  gap = 0.200165 seconds
```

That's your client's `usleep(200000)` (200,000 microseconds = 200ms), measured directly on the actual network, down to the microsecond — not inferred, not estimated. This is genuinely better evidence than the sandbox version of this analysis had, specifically because a single-machine packet capture only ever has one clock.

## What this confirms end-to-end

Every single number your programs printed — the ephemeral port, the two portmapper answers, `ADD(3,4)=7`, and `111+222=333` — is independently verifiable in the raw bytes that actually crossed your loopback interface. Nothing in the printed output was assumed or hidden; it's a direct, honest reflection of what actually went out and came back on the wire, on your own machine.

---

## Quick recap (for revision)

- **Always filter a real capture** — `tcpdump -i lo0` with no filter captures every loopback connection on the machine, not just the one you're studying; unrelated background traffic (here, an unrelated TLS exchange on two unrelated ports) will show up mixed in.
- The service's `PMAPPROC_SET` and the client's `PMAPPROC_GETPORT` independently agree on port `49269` — verified directly in the raw argument/result bytes of two separate real TCP connections, not just in the programs' own print statements.
- The direct `ADD` call connects straight to `49269` — a number that exists nowhere in the client's source code, only in the portmapper's real reply.
- The fragmented call's two headers differ by exactly one bit (`0x00000010` vs. `0x80000010`) — RFC 5531's entire "more fragments" vs. "last fragment" signal, seen directly in two real packets.
- A single-source capture (one `tcpdump`, one clock) gives you an unambiguous, precise timing measurement in a way that comparing two independently-launched traced processes cannot — worth remembering as a general principle: prefer one shared clock over cross-referencing several whenever you can get it.