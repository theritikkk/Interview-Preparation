// netchaos is a real, minimal network-chaos injector: it adds a genuine
// Linux netem qdisc (delay + jitter + packet loss, enforced by the kernel's
// own traffic-control subsystem) to a network interface, via real netlink
// syscalls — the exact kernel mechanism chaosd/Chaos Mesh's NetworkChaos
// wraps, used here directly since neither the `tc` CLI (iproute2 isn't
// installed in this sandbox) nor chaosd's own binary could be obtained
// (see ../ANALYSIS.md). github.com/vishvananda/netlink is a thin,
// widely-used wrapper over the real RTM_NEWQDISC/RTM_DELQDISC netlink
// messages — not a simulation of netem, a real binding to it.
//
// Usage:
//
//	netchaos inject <iface> <delay_ms> <jitter_ms> <loss_pct> <duration_s>
//	netchaos recover <iface>
//	netchaos show <iface>
package main

import (
	"fmt"
	"os"
	"strconv"
	"time"

	"github.com/vishvananda/netlink"
)

func netemQdisc(link netlink.Link, delayMs, jitterMs int, lossPct float64) *netlink.Netem {
	attrs := netlink.QdiscAttrs{
		LinkIndex: link.Attrs().Index,
		Handle:    netlink.MakeHandle(1, 0),
		Parent:    netlink.HANDLE_ROOT,
	}
	nattrs := netlink.NetemQdiscAttrs{
		Latency: uint32(delayMs * 1000), // netem wants microseconds
		Jitter:  uint32(jitterMs * 1000),
		Loss:    float32(lossPct),
		Limit:   1000,
	}
	return netlink.NewNetem(attrs, nattrs)
}

func inject(iface string, delayMs, jitterMs int, lossPct float64, durationS int) error {
	link, err := netlink.LinkByName(iface)
	if err != nil {
		return fmt.Errorf("link %q: %w", iface, err)
	}

	q := netemQdisc(link, delayMs, jitterMs, lossPct)
	fmt.Printf("[netchaos] adding real netem qdisc on %s: delay=%dms jitter=%dms loss=%.1f%%\n",
		iface, delayMs, jitterMs, lossPct)
	if err := netlink.QdiscAdd(q); err != nil {
		return fmt.Errorf("QdiscAdd: %w", err)
	}
	fmt.Printf("[netchaos] injected. Verify independently with: ip -s qdisc show dev %s (or netchaos show %s)\n", iface, iface)

	if durationS > 0 {
		fmt.Printf("[netchaos] holding for %ds, then auto-recovering...\n", durationS)
		time.Sleep(time.Duration(durationS) * time.Second)
		return recover_(iface)
	}
	return nil
}

func recover_(iface string) error {
	link, err := netlink.LinkByName(iface)
	if err != nil {
		return fmt.Errorf("link %q: %w", iface, err)
	}
	qdiscs, err := netlink.QdiscList(link)
	if err != nil {
		return fmt.Errorf("QdiscList: %w", err)
	}
	removed := 0
	for _, q := range qdiscs {
		if _, ok := q.(*netlink.Netem); ok {
			if err := netlink.QdiscDel(q); err != nil {
				return fmt.Errorf("QdiscDel: %w", err)
			}
			removed++
		}
	}
	fmt.Printf("[netchaos] recovered %s: removed %d netem qdisc(s)\n", iface, removed)
	return nil
}

func show(iface string) error {
	link, err := netlink.LinkByName(iface)
	if err != nil {
		return fmt.Errorf("link %q: %w", iface, err)
	}
	qdiscs, err := netlink.QdiscList(link)
	if err != nil {
		return fmt.Errorf("QdiscList: %w", err)
	}
	for _, q := range qdiscs {
		fmt.Printf("%s: %s %s\n", iface, q.Type(), q)
	}
	return nil
}

func main() {
	if len(os.Args) < 3 {
		fmt.Fprintln(os.Stderr, "usage: netchaos inject <iface> <delay_ms> <jitter_ms> <loss_pct> <duration_s>")
		fmt.Fprintln(os.Stderr, "       netchaos recover <iface>")
		fmt.Fprintln(os.Stderr, "       netchaos show <iface>")
		os.Exit(2)
	}
	cmd, iface := os.Args[1], os.Args[2]
	var err error
	switch cmd {
	case "inject":
		if len(os.Args) != 7 {
			fmt.Fprintln(os.Stderr, "usage: netchaos inject <iface> <delay_ms> <jitter_ms> <loss_pct> <duration_s>")
			os.Exit(2)
		}
		delayMs, _ := strconv.Atoi(os.Args[3])
		jitterMs, _ := strconv.Atoi(os.Args[4])
		lossPct, _ := strconv.ParseFloat(os.Args[5], 64)
		durationS, _ := strconv.Atoi(os.Args[6])
		err = inject(iface, delayMs, jitterMs, lossPct, durationS)
	case "recover":
		err = recover_(iface)
	case "show":
		err = show(iface)
	default:
		fmt.Fprintf(os.Stderr, "unknown command %q\n", cmd)
		os.Exit(2)
	}
	if err != nil {
		fmt.Fprintf(os.Stderr, "[netchaos] error: %v\n", err)
		os.Exit(1)
	}
}
