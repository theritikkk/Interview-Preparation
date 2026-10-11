// profworkload is a small, real, continuously CPU- and allocation-heavy Go
// service whose only purpose is to be a genuine profiling target: it wires
// in the standard library's net/http/pprof (which is exactly what Parca's
// own scrape_configs scrapes — see https://www.parca.dev/docs/parca-agent
// and the pprof_config block in the real upstream parca.yaml), and runs two
// deliberately distinct, named hot paths so a resulting flamegraph has real,
// recognizable structure to verify against:
//
//   - hashBurn: SHA-256 hashing in a tight loop (CPU-bound, no allocation
//     growth — should dominate the CPU profile)
//   - allocBurn: repeated map/slice growth and string building (allocation-
//     heavy — should dominate the heap profile)
//
// Nothing here is a stand-in for a real workload: this is a real compiled
// Go binary, serving real net/http/pprof endpoints, doing real CPU and
// memory work, scraped by a real Parca server.
package main

import (
	"crypto/sha256"
	"fmt"
	"log"
	"math/rand"
	"net/http"
	_ "net/http/pprof" // registers /debug/pprof/* on the default mux
	"sync/atomic"
	"time"
)

var (
	hashesDone  int64
	allocsDone  int64
)

// hashBurn repeatedly SHA-256-hashes a growing buffer. Pure CPU work,
// intentionally named so it's unmistakable in a CPU flamegraph.
func hashBurn() {
	buf := make([]byte, 4096)
	rand.Read(buf)
	for {
		sum := buf
		for i := 0; i < 2000; i++ {
			h := sha256.Sum256(sum)
			sum = h[:]
		}
		atomic.AddInt64(&hashesDone, 1)
	}
}

// allocBurn repeatedly builds and discards maps and strings. Allocation-
// heavy work, intentionally named so it's unmistakable in a heap/alloc
// profile.
func allocBurn() {
	for {
		m := make(map[string]string, 1000)
		for i := 0; i < 1000; i++ {
			k := fmt.Sprintf("key-%d-%d", i, time.Now().UnixNano())
			v := fmt.Sprintf("value-%d-%s", i, k)
			m[k] = v
		}
		_ = m
		atomic.AddInt64(&allocsDone, 1)
	}
}

func main() {
	addr := ":9999"
	log.Printf("profworkload: starting, pprof endpoints under http://127.0.0.1%s/debug/pprof/", addr)

	// Run several hashBurn and allocBurn goroutines so there's always real
	// CPU activity for the profiler to catch, independent of scrape timing.
	for i := 0; i < 4; i++ {
		go hashBurn()
	}
	for i := 0; i < 2; i++ {
		go allocBurn()
	}

	http.HandleFunc("/healthz", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintf(w, "ok hashes=%d allocs=%d\n",
			atomic.LoadInt64(&hashesDone), atomic.LoadInt64(&allocsDone))
	})

	log.Fatal(http.ListenAndServe(addr, nil))
}
