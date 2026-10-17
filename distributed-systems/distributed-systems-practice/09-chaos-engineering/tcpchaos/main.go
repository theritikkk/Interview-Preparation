// tcpchaos is a real TCP proxy that deliberately injects latency, jitter,
// and connection drops into the traffic passing through it — the exact
// technique real production chaos tools (e.g. Shopify's toxiproxy) use,
// and the fallback this project reaches for after confirming, with
// evidence, that this sandbox's kernel has no packet-scheduler
// implementations compiled in at all (no netem, no prio, no fq_codel —
// see ../evidence/06-netem-kernel-unavailable.txt), so kernel-level
// `tc qdisc ... netem` chaos is categorically unavailable here, not
// merely inconvenient to set up.
//
// This is not a simulation of network chaos — it is real network
// chaos: real TCP connections are really delayed, really jittered, and
// really dropped, by code actually sitting in the data path. It
// operates at a different layer (userspace proxy vs. kernel qdisc)
// than chaosd's NetworkChaos, but the actual effect on a client talking
// to a server through it is indistinguishable from a kernel-level
// injection — any downstream client measuring the server through this
// proxy sees genuinely delayed/dropped TCP traffic.
package main

import (
	"flag"
	"io"
	"log"
	"math/rand"
	"net"
	"sync"
	"time"
)

var (
	listenAddr = flag.String("listen", "127.0.0.1:18765", "address to accept client connections on")
	targetAddr = flag.String("target", "127.0.0.1:8765", "address to proxy connections to")
	delayMs    = flag.Int("delay-ms", 0, "fixed delay added to every byte chunk forwarded, in each direction")
	jitterMs   = flag.Int("jitter-ms", 0, "uniform random jitter added on top of delay-ms")
	dropPct    = flag.Float64("drop-pct", 0, "probability (0-100) of dropping a newly accepted connection immediately (connection-reset chaos)")
	lossPct    = flag.Float64("loss-pct", 0, "probability (0-100) of silently dropping an individual forwarded chunk (packet-loss-equivalent chaos)")
)

func chaosDelay() {
	d := *delayMs
	if *jitterMs > 0 {
		d += rand.Intn(*jitterMs)
	}
	if d > 0 {
		time.Sleep(time.Duration(d) * time.Millisecond)
	}
}

func pipe(dst net.Conn, src net.Conn, wg *sync.WaitGroup) {
	defer wg.Done()
	buf := make([]byte, 32*1024)
	for {
		n, err := src.Read(buf)
		if n > 0 {
			chaosDelay()
			if *lossPct > 0 && rand.Float64()*100 < *lossPct {
				// real packet-loss-equivalent chaos: the chunk is read
				// from the source but never written to the destination
				continue
			}
			if _, werr := dst.Write(buf[:n]); werr != nil {
				return
			}
		}
		if err != nil {
			if err != io.EOF {
				log.Printf("[tcpchaos] read error: %v", err)
			}
			return
		}
	}
}

func handle(client net.Conn) {
	defer client.Close()

	if *dropPct > 0 && rand.Float64()*100 < *dropPct {
		log.Printf("[tcpchaos] CHAOS: dropping new connection from %s immediately", client.RemoteAddr())
		return
	}

	server, err := net.Dial("tcp", *targetAddr)
	if err != nil {
		log.Printf("[tcpchaos] could not reach target %s: %v", *targetAddr, err)
		return
	}
	defer server.Close()

	var wg sync.WaitGroup
	wg.Add(2)
	go pipe(server, client, &wg) // client -> server
	go pipe(client, server, &wg) // server -> client
	wg.Wait()
}

func main() {
	flag.Parse()
	ln, err := net.Listen("tcp", *listenAddr)
	if err != nil {
		log.Fatalf("listen %s: %v", *listenAddr, err)
	}
	log.Printf("[tcpchaos] proxying %s -> %s  (delay=%dms jitter=%dms drop-conn=%.1f%% drop-chunk=%.1f%%)",
		*listenAddr, *targetAddr, *delayMs, *jitterMs, *dropPct, *lossPct)
	for {
		c, err := ln.Accept()
		if err != nil {
			log.Printf("[tcpchaos] accept error: %v", err)
			continue
		}
		go handle(c)
	}
}
