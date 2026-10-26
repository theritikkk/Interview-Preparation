#!/usr/bin/env python3
"""Render the vector diagrams used by the AI Infrastructure Engineering notes.

The diagrams are intentionally generated from code so they can be edited and
regenerated. Run from this directory:

    python3 render_diagrams.py

Output: ../*.svg
"""

from pathlib import Path

import matplotlib.pyplot as plt
from matplotlib.patches import FancyArrowPatch, FancyBboxPatch, Rectangle

OUT = Path(__file__).resolve().parent.parent

# ---------------------------------------------------------------- palette
INK = "#0f172a"
MUTED = "#475569"
LINE = "#94a3b8"
BLUE = "#1d4ed8"
BLUE_F = "#dbeafe"
TEAL = "#0f766e"
TEAL_F = "#ccfbf1"
AMBER = "#b45309"
AMBER_F = "#fef3c7"
RED = "#b91c1c"
RED_F = "#fee2e2"
PURPLE = "#6d28d9"
PURPLE_F = "#ede9fe"
SLATE = "#334155"
SLATE_F = "#f1f5f9"
GREEN = "#15803d"
GREEN_F = "#dcfce7"
WHITE = "#ffffff"


def canvas(w=14, h=8):
    fig, ax = plt.subplots(figsize=(w, h))
    fig.patch.set_facecolor("#fbfbfd")
    ax.set_facecolor("#fbfbfd")
    ax.set_xlim(0, w)
    ax.set_ylim(0, h)
    ax.axis("off")
    return fig, ax


def box(ax, x, y, w, h, text, fc=WHITE, ec=SLATE, fs=10, bold=False,
        color=INK, lw=1.2, radius=0.08, align="center"):
    patch = FancyBboxPatch(
        (x, y), w, h,
        boxstyle=f"round,pad=0.03,rounding_size={radius}",
        facecolor=fc, edgecolor=ec, linewidth=lw, zorder=2,
    )
    ax.add_patch(patch)
    ax.text(
        x + w / 2 if align == "center" else x + 0.12,
        y + h / 2, text,
        ha="center" if align == "center" else "left",
        va="center", fontsize=fs, color=color,
        fontweight="bold" if bold else "normal", zorder=3,
        linespacing=1.25,
    )


def arrow(ax, x1, y1, x2, y2, color=SLATE, lw=1.3, style="-|>", rad=0.0,
          ls="-", z=1):
    ax.add_patch(FancyArrowPatch(
        (x1, y1), (x2, y2),
        arrowstyle=style, mutation_scale=11,
        linewidth=lw, color=color, linestyle=ls,
        connectionstyle=f"arc3,rad={rad}", zorder=z,
        shrinkA=1, shrinkB=1,
    ))


def label(ax, x, y, text, fs=9, color=MUTED, ha="center", bold=False):
    ax.text(x, y, text, fontsize=fs, color=color, ha=ha, va="center",
            fontweight="bold" if bold else "normal")


# ---------------------------------------------------------------- 1. map
def curriculum_map():
    fig, ax = canvas(15, 9)
    ax.text(0.45, 8.55, "AI Infrastructure Engineering: prerequisite-first map",
            fontsize=18, color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 8.18, "Each column depends on the columns to its left. Do not skip a column; deepen it.",
            fontsize=10, color=MUTED, ha="left")

    cols = [
        ("FOUNDATIONS", "#e0f2fe", BLUE, [
            ("Computer architecture", "CPU, memory, I/O, interrupts"),
            ("Operating systems", "processes, virtual memory, syscalls"),
            ("Linux userspace", "files, pipes, permissions, signals"),
            ("TCP/IP + Ethernet", "packets, addresses, routes"),
        ]),
        ("OPERATIONS", "#ccfbf1", TEAL, [
            ("Linux administration", "systemd, logs, disks, packages"),
            ("Virtualization", "KVM, QEMU, libvirt, images"),
            ("Storage", "block, file, object, Ceph"),
            ("Automation", "Bash, Python, Ansible, CI/CD"),
        ]),
        ("PLATFORMS", "#ede9fe", PURPLE, [
            ("Containers", "namespaces, cgroups, images"),
            ("Kubernetes", "API, scheduler, kubelet, CNI, CSI"),
            ("Cloud / AWS", "VPC, IAM, EC2, EBS, EKS"),
            ("Private cloud", "OpenStack: Nova, Neutron, Cinder"),
        ]),
        ("RELIABILITY", "#fef3c7", AMBER, [
            ("Observability", "metrics, logs, traces"),
            ("Incident response", "evidence, RCA, runbooks"),
            ("Performance", "USE, queueing, capacity"),
            ("Security", "RBAC, IAM, firewalls, isolation"),
        ]),
        ("AI PLATFORM", "#fce7f3", "#9d174d", [
            ("GPU systems", "SM, HBM, CUDA"),
            ("Scheduling", "device plugin, MIG, Slurm"),
            ("Distributed training", "NCCL, RDMA, checkpoints"),
            ("MLOps / serving", "MLflow, vLLM, Triton"),
        ]),
    ]

    x0, y0, cw, gap = 0.35, 1.15, 2.72, 0.35
    for ci, (title, fill, edge, items) in enumerate(cols):
        x = x0 + ci * (cw + gap)
        ax.add_patch(Rectangle((x, 6.95), cw, 0.55, facecolor=edge,
                               edgecolor="none", zorder=1, alpha=0.95))
        ax.text(x + cw / 2, 7.22, title, ha="center", va="center",
                fontsize=11, color=WHITE, fontweight="bold", zorder=2)
        for ri, (head, sub) in enumerate(items):
            y = 5.6 - ri * 1.28
            box(ax, x, y, cw, 0.95, f"{head}\n{sub}", fc=fill, ec=edge,
                fs=8.6, color=INK)
        if ci < len(cols) - 1:
            arrow(ax, x + cw + 0.03, 7.22, x + cw + gap - 0.03, 7.22,
                  color=LINE, lw=1.4)

    # the vertical prerequisite spine
    ax.annotate("", xy=(0.18, 1.25), xytext=(0.18, 7.45),
                arrowprops=dict(arrowstyle="-|>", color=BLUE, lw=2,
                                mutation_scale=14))
    ax.text(0.06, 4.3, "depth first", rotation=90, fontsize=9,
            color=BLUE, ha="center", va="center", fontweight="bold")
    label(ax, 7.5, 0.55,
          "Minimum viable interview set: Foundations + Linux administration + TCP/IP + one platform + troubleshooting evidence.",
          fs=10, color=INK, bold=True)
    fig.savefig(OUT / "curriculum-map.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 2. linux layers
def linux_layers():
    fig, ax = canvas(13.5, 8.6)
    ax.text(0.5, 8.15, "What a Linux system actually is", fontsize=18,
            color=INK, fontweight="bold", ha="left")
    ax.text(0.5, 7.78, "Every troubleshooting command reads or changes one of these layers.",
            fontsize=10, color=MUTED, ha="left")

    # user space
    ax.add_patch(Rectangle((0.5, 5.35), 8.45, 2.05, facecolor=BLUE_F,
                           edgecolor=BLUE, lw=1.2, zorder=1))
    ax.text(0.7, 7.12, "USER SPACE", fontsize=10, color=BLUE, fontweight="bold")
    apps = [
        (0.75, 6.25, "shell / bash", "commands, scripts"),
        (2.85, 6.25, "systemd", "PID 1, services"),
        (4.95, 6.25, "sshd / cron", "remote + scheduled"),
        (7.05, 6.25, "your app", "node, python, java"),
    ]
    for x, y, t, s in apps:
        box(ax, x, y, 1.85, 0.78, f"{t}\n{s}", fc=WHITE, ec=BLUE, fs=8.5)
    box(ax, 0.75, 5.55, 8.1, 0.52,
        "libraries: glibc, OpenSSL, libcurl    |    tools: ps, ss, ip, journalctl",
        fc="#eff6ff", ec=BLUE, fs=8.2, color=BLUE)

    # syscall boundary
    ax.plot([0.5, 12.9], [5.25, 5.25], color=RED, lw=2.2, ls="--")
    ax.text(4.55, 5.04, "SYSTEM CALL BOUNDARY  —  read() write() open() fork() execve() mmap() socket()",
            fontsize=7.2, color=RED, ha="center", fontweight="bold")

    # kernel
    ax.add_patch(Rectangle((0.5, 2.35), 8.45, 2.45, facecolor=TEAL_F,
                           edgecolor=TEAL, lw=1.2, zorder=1))
    ax.text(0.7, 4.58, "KERNEL", fontsize=10, color=TEAL, fontweight="bold")
    subs = [
        (0.75, 3.65, "process scheduler", "CFS, run queue"),
        (2.85, 3.65, "virtual memory", "page tables, page cache"),
        (4.95, 3.65, "VFS + filesystems", "ext4, XFS, overlayfs"),
        (7.05, 3.65, "network stack", "sockets, TCP/IP, nft"),
        (0.75, 2.5, "drivers + interrupts", "block, NIC, NVMe"),
        (2.85, 2.5, "cgroups + namespaces", "isolation, limits"),
        (4.95, 2.5, "security", "caps, LSM, seccomp"),
        (7.05, 2.5, "KVM", "virtualization"),
    ]
    for x, y, t, s in subs:
        box(ax, x, y, 1.85, 0.88, f"{t}\n{s}", fc=WHITE, ec=TEAL, fs=8.1)

    # hardware
    ax.add_patch(Rectangle((0.5, 0.75), 8.2, 1.25, facecolor=SLATE_F,
                           edgecolor=SLATE, lw=1.2, zorder=1))
    ax.text(0.7, 1.78, "HARDWARE", fontsize=10, color=SLATE, fontweight="bold")
    box(ax, 0.75, 0.9, 1.75, 0.6, "CPU + caches", fc=WHITE, ec=SLATE, fs=8.6)
    box(ax, 2.7, 0.9, 1.75, 0.6, "RAM", fc=WHITE, ec=SLATE, fs=8.6)
    box(ax, 4.65, 0.9, 1.75, 0.6, "disk / NVMe", fc=WHITE, ec=SLATE, fs=8.6)
    box(ax, 6.6, 0.9, 1.95, 0.6, "NIC + GPU", fc=WHITE, ec=SLATE, fs=8.6)

    # side observation rail
    ax.add_patch(Rectangle((9.45, 0.75), 4.15, 6.65, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(9.65, 7.1, "WHERE TO LOOK", fontsize=10, color=INK, fontweight="bold")
    rows = [
        ("Process state", "/proc/<pid>/, ps, top"),
        ("Memory pressure", "/proc/meminfo, vmstat, cgroup"),
        ("Disk + inodes", "df -i, lsblk, findmnt"),
        ("Service failures", "systemctl status, journalctl"),
        ("Sockets + routes", "ss -tulpn, ip route"),
        ("Packet loss", "ip -s link, ethtool -S, tcpdump"),
        ("Kernel events", "dmesg, /var/log/kern.log"),
    ]
    for i, (a, b) in enumerate(rows):
        y = 6.55 - i * 0.82
        ax.text(9.65, y, a, fontsize=8.6, color=INK, fontweight="bold")
        ax.text(9.65, y - 0.26, b, fontsize=7.7, color=MUTED, family="monospace")

    fig.savefig(OUT / "linux-layers.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 3. packet journey
def packet_journey():
    fig, ax = canvas(15, 8.4)
    ax.text(0.45, 7.95, "The journey of one packet: process to process", fontsize=18,
            color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 7.6, "A request is not one event. It is a chain of lookups, queues, and handoffs.",
            fontsize=10, color=MUTED, ha="left")

    box(ax, 0.35, 5.15, 1.95, 1.55,
        "CLIENT PROCESS\n\nsocket()\nconnect()\nwrite()", fc=BLUE_F, ec=BLUE, fs=9)
    box(ax, 11.75, 5.15, 2.9, 1.55,
        "SERVER PROCESS\n\nsocket()\nbind()\nlisten()\naccept()", fc=GREEN_F, ec=GREEN, fs=9)

    steps = [
        ("TCP/IP stack\nsrc port -> dst port\nseq/ack, checksum", BLUE_F, BLUE),
        ("Routing decision\nlongest prefix\nmatch", SLATE_F, SLATE),
        ("ARP / neighbor\nIP -> MAC\n(virtual if routed)", AMBER_F, AMBER),
        ("NIC TX queue\nDMA + driver\noffloads", TEAL_F, TEAL),
        ("Switch / router\nVLAN tag, TTL--\nrewrite L2", PURPLE_F, PURPLE),
    ]
    for i, (t, fc, ec) in enumerate(steps):
        x = 2.55 + i * 1.78
        box(ax, x, 5.25, 1.68, 1.35, t, fc=fc, ec=ec, fs=7.9)
    for x in (2.31, 4.09, 5.87, 7.65, 9.43, 11.21):
        arrow(ax, x, 5.92, x + 0.2, 5.92, color=LINE)

    # response travels the other way
    ax.annotate("", xy=(2.35, 4.95), xytext=(11.7, 4.95),
                arrowprops=dict(arrowstyle="-|>", color=GREEN, lw=1.6,
                                mutation_scale=12, connectionstyle="arc3,rad=0.16"))
    ax.text(7.0, 4.05, "response follows the reverse path; state must exist at every hop (conntrack, ARP, routes, sockets)",
            fontsize=8.6, color=GREEN, ha="center")

    ax.add_patch(Rectangle((0.35, 0.6), 14.3, 2.95, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(0.6, 3.25, "FAILURE DOMAINS ALONG THE PATH", fontsize=10,
            color=INK, fontweight="bold")
    domains = [
        ("Name", "DNS, /etc/hosts, search domains", "dig, getent hosts"),
        ("Route", "no route, wrong gateway, asym path", "ip route get, traceroute"),
        ("Filter", "firewall drop, security group, NACL", "nft list ruleset, tcpdump"),
        ("Socket", "not listening, wrong bind address", "ss -tulpn, lsof -i"),
        ("Transport", "MSS/MTU, retransmit, conntrack full", "ss -ti, nstat, dmesg"),
        ("Application", "HTTP 5xx, TLS, auth, timeout", "curl -v, openssl s_client"),
    ]
    for i, (h, s, c) in enumerate(domains):
        x = 0.65 + (i % 3) * 4.75
        y = 2.7 - (i // 3) * 1.05
        ax.text(x, y, h, fontsize=8.8, color=RED, fontweight="bold")
        ax.text(x, y - 0.32, s, fontsize=8.0, color=INK)
        ax.text(x, y - 0.62, c, fontsize=7.6, color=MUTED, family="monospace")

    fig.savefig(OUT / "packet-journey.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 4. kubernetes
def kubernetes_arch():
    fig, ax = canvas(14.5, 9)
    ax.text(0.45, 8.55, "Kubernetes: desired state, reconciliation, and the data path",
            fontsize=17, color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 8.2, "The control plane stores intent. Node agents make reality match it. Traffic uses a separate path.",
            fontsize=9.8, color=MUTED, ha="left")

    # control plane
    ax.add_patch(Rectangle((0.45, 4.85), 8.6, 3.1, facecolor=PURPLE_F,
                           edgecolor=PURPLE, lw=1.3))
    ax.text(0.7, 7.65, "CONTROL PLANE", fontsize=11, color=PURPLE, fontweight="bold")
    cps = [
        (0.7, 6.55, "kube-apiserver", "authn / authz\nadmission\nvalidation"),
        (2.85, 6.55, "etcd", "the only source\nof cluster state\nquorum"),
        (5.0, 6.55, "kube-scheduler", "filter -> score\n-> bind"),
        (7.15, 6.55, "controller-manager", "reconcile loops\ndeployments, nodes"),
    ]
    for x, y, t, s in cps:
        box(ax, x, y, 1.85, 1.05, f"{t}\n{s}", fc=WHITE, ec=PURPLE, fs=8.4)
    box(ax, 0.7, 5.2, 8.3, 0.85,
        "cloud-controller-manager  |  aggregated APIs  |  admission webhooks  |  CRDs",
        fc="#f5f3ff", ec=PURPLE, fs=8.8, color=PURPLE)

    # nodes
    ax.add_patch(Rectangle((0.45, 0.55), 8.6, 3.75, facecolor=TEAL_F,
                           edgecolor=TEAL, lw=1.3))
    ax.text(0.7, 4.05, "WORKER NODE", fontsize=11, color=TEAL, fontweight="bold")
    nodes = [
        (0.7, 2.9, "kubelet", "pod specs\nfrom API\nprobes, status"),
        (2.85, 2.9, "container runtime", "containerd\nrunc / CRI"),
        (5.0, 2.9, "CNI plugin", "veth, bridge\npod CIDR"),
        (7.15, 2.9, "kube-proxy / eBPF", "Service VIP\nDNAT rules"),
    ]
    for x, y, t, s in nodes:
        box(ax, x, y, 1.85, 1.0, f"{t}\n{s}", fc=WHITE, ec=TEAL, fs=8.4)
    box(ax, 0.7, 1.25, 3.95, 1.15,
        "Pod A\npause + app containers\nrequests / limits / QoS",
        fc=WHITE, ec=TEAL, fs=9)
    box(ax, 4.95, 1.25, 3.95, 1.15,
        "Pod B\nshared network namespace\nper-pod cgroup",
        fc=WHITE, ec=TEAL, fs=9)
    ax.text(0.7, 0.85, "kernel: namespaces + cgroups + netfilter + filesystem", fontsize=8.4,
            color=TEAL, family="monospace")

    # arrows
    arrow(ax, 2.55, 6.5, 2.55, 5.55, color=PURPLE, lw=1.4)
    arrow(ax, 4.7, 6.5, 4.7, 5.55, color=PURPLE, lw=1.4)
    arrow(ax, 6.85, 6.5, 6.85, 5.55, color=PURPLE, lw=1.4)
    arrow(ax, 2.55, 5.15, 2.55, 3.95, color=PURPLE, lw=1.4, style="<|-|>")
    ax.text(2.65, 4.55, "pod spec / status", fontsize=7.4, color=PURPLE)

    # traffic path
    ax.add_patch(Rectangle((9.55, 0.55), 4.45, 7.4, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(9.8, 7.55, "REQUEST PATH", fontsize=10, color=INK, fontweight="bold")
    path = [
        ("Client", "DNS: api.example.com"),
        ("Ingress / LB", "TLS, host/path route"),
        ("Service", "ClusterIP -> EndpointSlice"),
        ("Pod IP", "CNI route / overlay"),
        ("Container", "app listens :8080"),
    ]
    for i, (h, s) in enumerate(path):
        y = 6.55 - i * 1.12
        box(ax, 9.85, y, 3.85, 0.78, f"{h}\n{s}", fc=WHITE, ec=SLATE, fs=8.5)
        if i < len(path) - 1:
            arrow(ax, 11.75, y - 0.02, 11.75, y - 0.34, color=SLATE)
    ax.text(9.85, 0.85, "Service VIP is virtual.\nNo process listens on it.", fontsize=8.2,
            color=RED, style="italic")

    fig.savefig(OUT / "kubernetes-architecture.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 5. container internals
def container_internals():
    fig, ax = canvas(14, 8.4)
    ax.text(0.45, 7.95, "A container is not a tiny VM", fontsize=18,
            color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 7.58, "It is a normal Linux process with a changed view of the system and enforced resource limits.",
            fontsize=10, color=MUTED, ha="left")

    # host
    ax.add_patch(Rectangle((0.45, 0.65), 13.1, 6.55, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.2))
    ax.text(0.7, 6.9, "HOST KERNEL (one kernel, shared by every container)",
            fontsize=10, color=SLATE, fontweight="bold")

    # container box
    ax.add_patch(Rectangle((0.85, 1.05), 7.5, 5.35, facecolor=BLUE_F,
                           edgecolor=BLUE, lw=1.3))
    ax.text(1.05, 6.05, "CONTAINER PROCESS", fontsize=10, color=BLUE, fontweight="bold")
    box(ax, 1.1, 4.75, 6.95, 0.95,
        "app binary + libraries + entrypoint\n(PID 1 inside the container, a normal PID on the host)",
        fc=WHITE, ec=BLUE, fs=8.8)
    box(ax, 1.1, 3.55, 3.3, 0.85, "root filesystem\noverlayfs: lower + upper", fc=WHITE, ec=BLUE, fs=8.2)
    box(ax, 4.7, 3.55, 3.35, 0.85, "writable layer\ncopy-on-write", fc=WHITE, ec=BLUE, fs=8.2)
    box(ax, 1.1, 2.35, 6.95, 0.85,
        "mount namespace: sees only its rootfs + explicit mounts",
        fc="#eff6ff", ec=BLUE, fs=8.4)
    box(ax, 1.1, 1.35, 6.95, 0.7,
        "pid / net / uts / ipc / user / cgroup namespaces",
        fc="#eff6ff", ec=BLUE, fs=8.4)

    # limits
    ax.add_patch(Rectangle((8.75, 1.05), 4.45, 5.35, facecolor=AMBER_F,
                           edgecolor=AMBER, lw=1.3))
    ax.text(8.95, 6.05, "CGROUPS v2", fontsize=10, color=AMBER, fontweight="bold")
    rows = [
        ("cpu.max", "quota / period"),
        ("memory.max", "hard limit -> OOM"),
        ("memory.high", "throttle before OOM"),
        ("pids.max", "fork bomb guard"),
        ("io.max", "block I/O throttle"),
        ("cpuset.cpus", "CPU pinning"),
    ]
    for i, (a, b) in enumerate(rows):
        y = 5.4 - i * 0.66
        ax.text(8.95, y, a, fontsize=8.6, color=INK, fontweight="bold", family="monospace")
        ax.text(10.25, y, b, fontsize=8.4, color=MUTED)
    ax.text(8.95, 1.35, "Kernel enforces limits.\nDocker/K8s only write files.", fontsize=8.2,
            color=AMBER, style="italic")

    # bottom annotations
    ax.text(0.65, 0.3, "Namespaces change what the process can see.  cgroups change what it can use.  Capabilities/seccomp change what it can do.",
            fontsize=8.8, color=INK, ha="left")
    fig.savefig(OUT / "container-internals.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 6. openstack flow
def openstack_flow():
    fig, ax = canvas(14.5, 8.8)
    ax.text(0.45, 8.35, "OpenStack VM creation: from API call to running domain",
            fontsize=17, color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 8.0, "Follow the request. Every service owns one decision, one record, or one resource.",
            fontsize=9.8, color=MUTED, ha="left")

    steps = [
        (0.45, 6.55, "1  CLI / API\nopenstack server create", BLUE_F, BLUE),
        (2.55, 6.55, "2  Keystone\ntoken + project\n+ roles", PURPLE_F, PURPLE),
        (4.65, 6.55, "3  Nova API\nvalidate quota,\ncreate DB record", TEAL_F, TEAL),
        (6.75, 6.55, "4  Placement\nresource inventory\nallocation", AMBER_F, AMBER),
        (8.85, 6.55, "5  Nova scheduler\nfilter hosts\nweight + pick", RED_F, RED),
        (10.95, 6.55, "6  Nova conductor\n+ compute\nRPC to host", GREEN_F, GREEN),
    ]
    for x, y, t, fc, ec in steps:
        box(ax, x, y, 1.85, 1.05, t, fc=fc, ec=ec, fs=7.9)
    for x in (2.33, 4.43, 6.53, 8.63, 10.73):
        arrow(ax, x, 7.07, x + 0.18, 7.07, color=LINE)

    # side services
    side = [
        (0.45, 4.65, "Glance\nimage -> qcow2/raw\nstored in backend", BLUE_F, BLUE),
        (2.55, 4.65, "Neutron\nport, subnet, SG\nML2 + OVS/OVN", TEAL_F, TEAL),
        (4.65, 4.65, "Cinder\nvolume -> backend\nLVM / Ceph", PURPLE_F, PURPLE),
        (6.75, 4.65, "libvirt\nXML domain\ndefinition", AMBER_F, AMBER),
        (8.85, 4.65, "QEMU/KVM\nvirtual CPU,\nmemory, devices", RED_F, RED),
        (10.95, 4.65, "Guest OS\nboots, cloud-init\nconfigures network", GREEN_F, GREEN),
    ]
    for x, y, t, fc, ec in side:
        box(ax, x, y, 1.85, 1.15, t, fc=fc, ec=ec, fs=7.9)
    for x in (1.35, 3.45, 5.55, 7.65, 9.75, 11.85):
        arrow(ax, x, 6.5, x, 5.85, color=LINE, ls="--")

    # data path
    ax.add_patch(Rectangle((0.45, 1.0), 12.35, 2.65, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(0.7, 3.35, "WHAT ACTUALLY CROSSES THE NETWORK", fontsize=9.5,
            color=INK, fontweight="bold")
    box(ax, 0.75, 1.55, 2.6, 1.3,
        "image bytes\nGlance -> compute\n(large, once)", fc=BLUE_F, ec=BLUE, fs=8.5)
    box(ax, 3.65, 1.55, 2.6, 1.3,
        "control RPC\nNova/Neutron\nRabbitMQ", fc=TEAL_F, ec=TEAL, fs=8.5)
    box(ax, 6.55, 1.55, 2.6, 1.3,
        "block I/O\nCinder volume\nover network", fc=PURPLE_F, ec=PURPLE, fs=8.5)
    box(ax, 9.45, 1.55, 2.85, 1.3,
        "tenant traffic\nVXLAN/GRE/VLAN\n+ floating IP NAT", fc=AMBER_F, ec=AMBER, fs=8.5)

    ax.text(0.7, 0.45, "If the VM is ERROR: identify which step failed before restarting anything. The API record, the scheduler, the port binding, and the libvirt domain are separate evidence sources.",
            fontsize=8.4, color=RED)
    fig.savefig(OUT / "openstack-vm-flow.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 7. gpu hierarchy
def gpu_hierarchy():
    fig, ax = canvas(14, 8.8)
    ax.text(0.45, 8.35, "Why GPU infrastructure is still infrastructure", fontsize=18,
            color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 7.98, "Fast compute does not remove the laws of memory, I/O, scheduling, and failure.",
            fontsize=10, color=MUTED, ha="left")

    # host side
    ax.add_patch(Rectangle((0.45, 4.75), 5.0, 2.95, facecolor=SLATE_F,
                           edgecolor=SLATE, lw=1.2))
    ax.text(0.7, 7.35, "HOST", fontsize=10, color=SLATE, fontweight="bold")
    box(ax, 0.7, 6.0, 2.05, 0.85, "CPU + RAM\ndata loader, orchestration", fc=WHITE, ec=SLATE, fs=8.1)
    box(ax, 3.0, 6.0, 2.2, 0.85, "storage\ncheckpoints, dataset", fc=WHITE, ec=SLATE, fs=8.1)
    box(ax, 0.7, 5.0, 4.5, 0.65, "PCIe / NVLink / InfiniBand / RoCE", fc="#f1f5f9", ec=SLATE, fs=8.4)

    # gpu
    ax.add_patch(Rectangle((5.95, 4.35), 7.6, 3.35, facecolor=PURPLE_F,
                           edgecolor=PURPLE, lw=1.3))
    ax.text(6.2, 7.35, "GPU", fontsize=11, color=PURPLE, fontweight="bold")
    box(ax, 6.2, 6.25, 3.5, 0.8, "HBM (high bandwidth memory)\nweights + activations", fc=WHITE, ec=PURPLE, fs=8.0)
    box(ax, 9.9, 6.25, 3.4, 0.8, "L2 cache\nshared across SMs", fc=WHITE, ec=PURPLE, fs=8.0)
    box(ax, 6.2, 5.15, 2.15, 0.8, "SM\nCUDA + Tensor\ncores", fc=WHITE, ec=PURPLE, fs=7.9)
    box(ax, 8.65, 5.15, 2.15, 0.8, "SM\nwarp scheduler\nregisters", fc=WHITE, ec=PURPLE, fs=7.9)
    box(ax, 11.1, 5.15, 2.2, 0.8, "SM\nshared memory\nL1", fc=WHITE, ec=PURPLE, fs=7.9)
    box(ax, 6.2, 4.5, 7.1, 0.42, "warps -> blocks -> grid   |   kernel = one function on thousands of threads",
        fc="#f5f3ff", ec=PURPLE, fs=7.9)
    ax.text(5.95, 3.95, "100% utilization is not progress: a GPU can be busy waiting on memory.",
            fontsize=8.2, color=PURPLE, style="italic")

    ax.add_patch(Rectangle((0.45, 0.55), 13.1, 3.15, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(0.7, 3.35, "THE FOUR BOTTLENECKS YOU WILL ACTUALLY DEBUG", fontsize=10,
            color=INK, fontweight="bold")
    items = [
        ("HOST -> DEVICE", "PCIe bandwidth, pageable\nmemory, small batches", "nvidia-smi topo -m\nnsys profile"),
        ("MEMORY", "HBM capacity, fragmentation,\nactivation spikes", "nvidia-smi dmon\nPyTorch memory summary"),
        ("DATA", "dataloader workers, storage\nIOPS, network filesystem", "iostat, fio\nGPU idle gaps"),
        ("SCHEDULING", "topology, NUMA, MIG,\ngang scheduling, stragglers", "kubectl describe\nsinfo / squeue"),
    ]
    for i, (h, s, c) in enumerate(items):
        x = 0.7 + i * 3.18
        ax.text(x, 2.85, h, fontsize=8.7, color=RED, fontweight="bold")
        ax.text(x, 2.3, s, fontsize=7.9, color=INK, linespacing=1.3)
        ax.text(x, 1.45, c, fontsize=7.3, color=MUTED, family="monospace", linespacing=1.3)

    ax.text(0.7, 0.95, "The AI platform engineer's job: keep accelerators fed, isolated, observable, and replaceable.",
            fontsize=8.8, color=INK, fontweight="bold")
    fig.savefig(OUT / "gpu-hierarchy.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 8. training loop
def training_loop():
    fig, ax = canvas(14, 8.6)
    ax.text(0.45, 8.2, "The training pipeline: the GPU is rarely the first bottleneck",
            fontsize=17, color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 7.85, "Every stage can starve the next. Measure the gaps, not just the utilization.",
            fontsize=9.6, color=MUTED, ha="left")

    stages = [
        ("STORAGE\ndataset / shards", "IOPS, throughput,\nremote filesystem", "fio, iostat"),
        ("CPU LOADER\ndecode, augment", "worker count, GIL,\nCPU saturation", "pidstat, py-spy"),
        ("TRANSFER\nH2D copy", "PCIe/NVLink,\nsmall batches", "nvidia-smi, nsys"),
        ("GPU MEMORY\nweights + activations", "capacity, fragmentation,\nOOM", "torch memory summary"),
        ("KERNELS\nforward / backward", "occupancy, tensor cores,\narithmetic intensity", "ncu, nsys, dmon"),
        ("CHECKPOINT\nstate + optimizer", "write bandwidth,\nstall time", "dcgm, iostat"),
    ]
    for i, (title, bottleneck, cmd) in enumerate(stages):
        x = 0.45 + i * 2.2
        box(ax, x, 5.55, 1.95, 1.05, title, fc=BLUE_F if i < 3 else PURPLE_F,
            ec=BLUE if i < 3 else PURPLE, fs=7.8, bold=True)
        ax.text(x + 0.97, 4.95, bottleneck, fontsize=6.8, color=INK, ha="center",
                va="center", linespacing=1.3)
        ax.text(x + 0.97, 4.25, cmd, fontsize=6.4, color=MUTED, ha="center",
                family="monospace")
        if i < len(stages) - 1:
            arrow(ax, x + 1.97, 6.07, x + 2.17, 6.07, color=LINE)

    # training loop
    ax.add_patch(Rectangle((0.45, 1.55), 13.1, 2.2, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(0.7, 3.45, "THE STEP LOOP", fontsize=9.5, color=INK, fontweight="bold")
    loop = [
        (0.9, "FORWARD\nlogits", BLUE_F, BLUE),
        (3.5, "LOSS\nscalar", AMBER_F, AMBER),
        (6.1, "BACKWARD\ngradients", RED_F, RED),
        (8.7, "ALL-REDUCE\nif distributed", TEAL_F, TEAL),
        (11.3, "OPTIMIZER\nupdate", GREEN_F, GREEN),
    ]
    for x, t, fc, ec in loop:
        box(ax, x, 2.25, 2.1, 0.8, t, fc=fc, ec=ec, fs=7.8)
    for x in (3.0, 5.6, 8.2, 10.8):
        arrow(ax, x, 2.65, x + 0.45, 2.65, color=LINE)
    arrow(ax, 12.35, 2.2, 12.35, 1.85, color=GREEN)
    arrow(ax, 12.35, 1.85, 1.95, 1.85, color=GREEN, rad=0.0)
    arrow(ax, 1.95, 1.85, 1.95, 2.2, color=GREEN)
    ax.text(7.0, 1.72, "next batch", fontsize=7.4, color=GREEN, ha="center")

    ax.text(0.7, 1.15, "If utilization is low, find the stage that is waiting: storage, loader, transfer, memory, or the collective.",
            fontsize=8.4, color=INK, fontweight="bold")
    ax.text(0.7, 0.82, "A faster GPU does not fix a slow input pipeline; it just idles faster.",
            fontsize=8.2, color=MUTED, style="italic")
    fig.savefig(OUT / "training-loop.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 9. parallelism
def parallelism():
    fig, ax = canvas(14, 8.8)
    ax.text(0.45, 8.4, "Four ways to split a model — and what each one communicates",
            fontsize=17, color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 8.05, "The communication pattern, not the diagram, decides whether it scales.",
            fontsize=9.6, color=MUTED, ha="left")

    # Data parallel
    ax.add_patch(Rectangle((0.45, 5.55), 4.05, 2.25, facecolor=BLUE_F, edgecolor=BLUE, lw=1.1))
    ax.text(0.7, 7.5, "DATA PARALLEL", fontsize=9.2, color=BLUE, fontweight="bold")
    for i in range(3):
        box(ax, 0.7 + i * 1.25, 6.35, 1.05, 0.8, f"GPU {i}\nfull model", fc=WHITE, ec=BLUE, fs=6.8)
    ax.text(2.45, 5.95, "all-reduce gradients every step", fontsize=7.4, color=INK, ha="center")
    ax.text(2.45, 5.72, "scales throughput; model must fit", fontsize=6.8, color=MUTED, ha="center")

    # Tensor parallel
    ax.add_patch(Rectangle((4.75, 5.55), 4.05, 2.25, facecolor=PURPLE_F, edgecolor=PURPLE, lw=1.1))
    ax.text(5.0, 7.5, "TENSOR PARALLEL", fontsize=9.2, color=PURPLE, fontweight="bold")
    for i in range(3):
        box(ax, 5.0 + i * 1.25, 6.35, 1.05, 0.8, f"GPU {i}\nshard of layer", fc=WHITE, ec=PURPLE, fs=6.8)
    ax.text(6.75, 5.95, "all-reduce / all-gather activations", fontsize=7.4, color=INK, ha="center")
    ax.text(6.75, 5.72, "splits one layer; latency-sensitive", fontsize=6.8, color=MUTED, ha="center")

    # Pipeline parallel
    ax.add_patch(Rectangle((9.05, 5.55), 4.5, 2.25, facecolor=TEAL_F, edgecolor=TEAL, lw=1.1))
    ax.text(9.3, 7.5, "PIPELINE PARALLEL", fontsize=9.2, color=TEAL, fontweight="bold")
    for i in range(3):
        box(ax, 9.3 + i * 1.4, 6.35, 1.2, 0.8, f"stage {i}\nlayers", fc=WHITE, ec=TEAL, fs=6.8)
    ax.text(11.3, 5.95, "point-to-point activations between stages", fontsize=7.2, color=INK, ha="center")
    ax.text(11.3, 5.72, "splits depth; bubbles need microbatches", fontsize=6.8, color=MUTED, ha="center")

    # 3D
    ax.add_patch(Rectangle((0.45, 2.65), 13.1, 2.3, facecolor="#f8fafc", edgecolor=LINE, lw=1.0, ls="--"))
    ax.text(0.7, 4.6, "3D PARALLELISM", fontsize=9.5, color=INK, fontweight="bold")
    ax.text(0.7, 4.15, "data × tensor × pipeline × expert  →  the topology decides what is cheap:", fontsize=8.4, color=INK)
    facts = [
        ("NVLink / NVSwitch", "GPUs in one node; tensor parallel is cheap."),
        ("InfiniBand / RoCE", "GPUs across nodes; data parallel all-reduce is the common path."),
        ("PCIe only", "Avoid chatty tensor parallel across PCIe; batch and pipeline instead."),
        ("Failure domain", "One rank down can stall the whole collective; checkpoint and elastic restart."),
    ]
    for i, (h, s) in enumerate(facts):
        x = 0.7 + (i % 2) * 6.4
        y = 3.55 - (i // 2) * 0.62
        ax.text(x, y, h, fontsize=7.9, color=PURPLE, fontweight="bold")
        ax.text(x + 1.95, y, s, fontsize=7.2, color=INK)

    ax.text(0.7, 2.15, "If the interconnect is the bottleneck, more GPUs make the job slower, not faster.",
            fontsize=8.4, color=RED, fontweight="bold")
    ax.text(0.7, 1.82, "Measure bus bandwidth and collective time before choosing a strategy.",
            fontsize=8.0, color=MUTED, style="italic")
    fig.savefig(OUT / "parallelism.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 10. gpu scheduling
def gpu_scheduling():
    fig, ax = canvas(14, 8.7)
    ax.text(0.45, 8.3, "GPU scheduling: Kubernetes and Slurm solve the same problem differently",
            fontsize=16.5, color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 7.95, "The scheduler must know the resource exists, isolate it, and place the work near the right topology.",
            fontsize=9.3, color=MUTED, ha="left")

    # Kubernetes panel
    ax.add_patch(Rectangle((0.45, 3.55), 6.35, 4.0, facecolor=BLUE_F, edgecolor=BLUE, lw=1.1))
    ax.text(0.7, 7.25, "KUBERNETES", fontsize=10, color=BLUE, fontweight="bold")
    box(ax, 0.7, 6.2, 2.2, 0.7, "Pod\nnvidia.com/gpu: 1", fc=WHITE, ec=BLUE, fs=7.2)
    box(ax, 3.15, 6.2, 1.8, 0.7, "scheduler\nfilter + score", fc=WHITE, ec=BLUE, fs=7.2)
    box(ax, 5.2, 6.2, 1.3, 0.7, "kubelet\n+ plugin", fc=WHITE, ec=BLUE, fs=7.2)
    arrow(ax, 2.92, 6.55, 3.13, 6.55, color=LINE)
    arrow(ax, 4.97, 6.55, 5.18, 6.55, color=LINE)

    ax.add_patch(Rectangle((0.7, 4.0), 5.8, 1.85, facecolor=WHITE, edgecolor=BLUE, lw=1.0))
    ax.text(0.9, 5.6, "NODE  (8 GPUs)", fontsize=8.0, color=BLUE, fontweight="bold")
    for i in range(8):
        col = i % 4
        row = i // 4
        box(ax, 0.95 + col * 1.32, 4.85 - row * 0.55, 1.15, 0.4,
            f"GPU{i}", fc=PURPLE_F, ec=PURPLE, fs=6.4)
    ax.text(0.9, 4.25, "taints, labels, MIG partitions, topology", fontsize=6.7, color=MUTED)
    ax.text(0.9, 3.75, "Device plugin advertises capacity; it does not schedule.", fontsize=7.0, color=RED, fontweight="bold")

    # Slurm panel
    ax.add_patch(Rectangle((7.0, 3.55), 6.55, 4.0, facecolor=TEAL_F, edgecolor=TEAL, lw=1.1))
    ax.text(7.25, 7.25, "SLURM", fontsize=10, color=TEAL, fontweight="bold")
    box(ax, 7.25, 6.2, 1.8, 0.7, "sbatch\njob script", fc=WHITE, ec=TEAL, fs=7.2)
    box(ax, 9.25, 6.2, 2.0, 0.7, "slurmctld\npriority + backfill", fc=WHITE, ec=TEAL, fs=7.2)
    box(ax, 11.45, 6.2, 1.8, 0.7, "slurmd\nGRES = gpu:8", fc=WHITE, ec=TEAL, fs=7.2)
    arrow(ax, 9.07, 6.55, 9.23, 6.55, color=LINE)
    arrow(ax, 11.27, 6.55, 11.43, 6.55, color=LINE)

    ax.add_patch(Rectangle((7.25, 4.0), 6.05, 1.85, facecolor=WHITE, edgecolor=TEAL, lw=1.0))
    ax.text(7.45, 5.6, "PARTITION  gpu-a100  (4 nodes × 8 GPUs)", fontsize=7.6, color=TEAL, fontweight="bold")
    for i in range(4):
        box(ax, 7.45 + i * 1.45, 4.75, 1.3, 0.55, f"node{i}\n8 GPUs", fc=PURPLE_F, ec=PURPLE, fs=6.5)
    ax.text(7.45, 4.25, "gres.conf, cgroup plugin, MIG binding, accounting", fontsize=6.7, color=MUTED)
    ax.text(7.45, 3.75, "The queue is a policy engine; the node is the isolation boundary.", fontsize=7.0, color=RED, fontweight="bold")

    ax.text(0.7, 3.05, "Same questions in both systems: Does the resource exist? Is it isolated? Is the work placed near the data and the interconnect?",
            fontsize=8.2, color=INK, fontweight="bold")
    ax.text(0.7, 2.65, "A GPU that is advertised but not isolated is a shared failure domain. A GPU that is isolated but not observable is a mystery.",
            fontsize=8.0, color=MUTED, style="italic")

    cmds = [
        ("K8s evidence", "kubectl describe node | grep -A8 Capacity\nkubectl get pods -o json | jq '.items[].spec.containers[].resources'"),
        ("Slurm evidence", "sinfo -o '%P %n %G %t %C'\nsqueue -o '%.10i %.9P %.20j %.8T %.10M %.6D %R'"),
    ]
    for i, (h, c) in enumerate(cmds):
        x = 0.7 + i * 6.5
        ax.text(x, 1.95, h, fontsize=8.2, color=RED, fontweight="bold")
        ax.text(x, 1.35, c, fontsize=7.0, color=INK, family="monospace", linespacing=1.3)
    fig.savefig(OUT / "gpu-scheduling.svg", bbox_inches="tight")
    plt.close(fig)


# ---------------------------------------------------------------- 11. troubleshooting loop
def troubleshoot_loop():
    fig, ax = canvas(14, 8.4)
    ax.text(0.45, 7.95, "The incident loop: evidence before change", fontsize=18,
            color=INK, fontweight="bold", ha="left")
    ax.text(0.45, 7.58, "Stabilize first. Then narrow the fault. Change one variable at a time.",
            fontsize=10, color=MUTED, ha="left")

    steps = [
        (2.0, 5.7, "1  OBSERVE\nsymptom, scope,\nblast radius", BLUE_F, BLUE),
        (5.0, 5.7, "2  STABILIZE\nstop the bleeding,\npreserve evidence", GREEN_F, GREEN),
        (8.0, 5.7, "3  GATHER\nlogs, metrics,\nstate, timeline", TEAL_F, TEAL),
        (11.0, 5.7, "4  BOUND\nlayer, host, path,\ntime window", AMBER_F, AMBER),
        (11.0, 3.35, "5  HYPOTHESIZE\nfalsifiable causes,\nrank by likelihood", PURPLE_F, PURPLE),
        (8.0, 3.35, "6  TEST\none command per\nhypothesis", RED_F, RED),
        (5.0, 3.35, "7  FIX\nsmallest safe change,\nrollback ready", GREEN_F, GREEN),
        (2.0, 3.35, "8  VERIFY\nsymptom gone,\nmetrics normal", BLUE_F, BLUE),
    ]
    for x, y, t, fc, ec in steps:
        box(ax, x, y, 2.45, 1.25, t, fc=fc, ec=ec, fs=8.7)
    for x in (4.5, 7.5, 10.5):
        arrow(ax, x, 6.32, x + 0.45, 6.32, color=LINE)
    arrow(ax, 12.2, 5.65, 12.2, 4.65, color=LINE)
    arrow(ax, 10.95, 3.97, 10.5, 3.97, color=LINE)
    for x in (7.45, 4.45):
        arrow(ax, x, 3.97, x - 0.45, 3.97, color=LINE)
    arrow(ax, 3.2, 4.6, 3.2, 5.65, color=GREEN, lw=1.6)
    ax.text(3.35, 5.1, "new evidence", fontsize=7.6, color=GREEN)

    # lower rules
    ax.add_patch(Rectangle((0.45, 0.6), 13.1, 2.05, facecolor="#f8fafc",
                           edgecolor=LINE, lw=1.0, ls="--"))
    rules = [
        ("Preserve", "Never delete logs, restart blindly, or resize before capturing state."),
        ("Separate", "What is broken? What is merely slow? What is normal but looks scary?"),
        ("Correlate", "Compare a working host/pod/time window with the broken one."),
        ("Falsify", "A good test can prove your hypothesis wrong. A bad test only confirms it."),
    ]
    for i, (h, s) in enumerate(rules):
        x = 0.7 + (i % 2) * 6.5
        y = 2.15 - (i // 2) * 0.78
        ax.text(x, y, h, fontsize=9, color=RED, fontweight="bold")
        ax.text(x + 1.15, y, s, fontsize=7.8, color=INK)

    ax.text(0.7, 0.85, "Write the timeline as you go. The postmortem starts during the incident, not after it.",
            fontsize=8.6, color=MUTED, style="italic")
    fig.savefig(OUT / "troubleshooting-loop.svg", bbox_inches="tight")
    plt.close(fig)


if __name__ == "__main__":
    curriculum_map()
    linux_layers()
    packet_journey()
    kubernetes_arch()
    container_internals()
    openstack_flow()
    gpu_hierarchy()
    training_loop()
    parallelism()
    gpu_scheduling()
    troubleshoot_loop()
    print("wrote diagrams to", OUT)
