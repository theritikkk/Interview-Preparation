# Glossary — Every Term in the Series, One Line Each

> **How to use this file**
> Terms are grouped by layer, alphabetical within each group, and every line ends with the file where the term is explained in depth. The bare numbers (`01`–`09`) are the numbered files of this series; `R` is the [README](README.md). Keep this open beside file [09](09-synthesis-strategy-safety-interview-qa.md) during revision.

---

## 1. The agent and its loop

| Term | One-line definition | Where |
|---|---|---|
| **Action space** | The vocabulary of calls the model may emit: raw input primitives, structured element actions, shell/file tools, composites. | 01 |
| **Action budget** | A per-task cap on steps/time after which the agent stops cleanly with a state report instead of looping. | 09 |
| **Agent loop** | The repeating cycle: harness feeds an observation, model replies with one action, executor applies it, fresh observation follows. | 01 |
| **Ask-the-user policy** | An explicit contract for when the agent stops guessing and asks (ambiguity, irreversible actions) instead of choosing silently. | 09 |
| **Channel ladder** | The ordered fallback list of perception/action channels — DOM → tree refs → tree+SoM → SoM → pure grounding → shell. | 09 |
| **Composite call** | Several primitive steps inside one atomic tool call (a drag is press→move→release with no model turn between). | 01 |
| **Control problem** | Question two for every agent: how to turn knowledge of the screen into an action the OS honors. | R |
| **Conversation as memory** | The agent's only memory is the token context; the machine keeps nothing, so history design *is* memory design. | 01 |
| **Cost tiers** | The observation budget hierarchy: text cheapest, annotated images next, raw pixels expensive, zoom crops targeted, VM per task. | 09 |
| **Executor** | The harness component that validates, translates, executes, settles, and records every action — and exclusively owns coordinate mapping. | 01 |
| **Failure taxonomy** | The four debug layers: perception, control, planning/memory, environment. | 01 |
| **Four horsemen** | The long-horizon failure modes: constraints lost, late information missed, guessing instead of asking, verification skipped. | 09 |
| **Generation counter** | A monotonically increasing snapshot id; refs carry it, and a mismatch forces refresh-and-retry instead of blind execution. | 04/06 |
| **Grounder** | A small specialist VLM doing one conversion: "(description, screenshot) → point". | 03 |
| **Harness** | The program wrapping the model: builds prompts, parses tool calls, owns loop state and policy. | 01 |
| **Hybrid reference architecture** | The convergent production skeleton: planner + grounder + executor + verifier, routed by the channel ladder. | 09 |
| **Long-horizon task** | A task spanning hundreds of steps/hours (OSWorld 2.0 territory), where planning and memory failures dominate. | 06 |
| **Machine (agent computer)** | The real computer at the far end of the executor — VM or desktop — that the model never directly touches. | 01 |
| **Observation space** | The set of input channels an agent is given (screenshot, tree, DOM, shell output) — a deliberate choice that defines the agent's character. | 01 |
| **Perception problem** | Question one for every agent: what is on the screen right now, and where. | R |
| **Pixel path** | Producing observations as screenshots and delivering actions as synthetic input events. | 02 |
| **Planner** | The big text-first LLM that reasons over observations, plans, and decides what to do next. | 06 |
| **Raw input primitives** | The lowest-level action vocabulary: coordinate clicks, key presses, scrolls, drags — universal but fingerprint-y. | 01 |
| **Shell/file tools** | Terminal and filesystem access for the agent — strictly more powerful than clicking, strictly more dangerous. | 01 |
| **Split-brain architecture** | The planner/grounder split: reasoning over text in a big model, pointing over pixels in a small one. | 06 |
| **Structured element actions** | Acting by naming an element (`click(ref=e12)`, `set_value(...)`) rather than pointing at coordinates. | 01 |
| **Verification discipline** | Predict the expected change, then confirm it with the cheapest channel that can observe it — writes always, reads until contradicted. | 09 |

## 2. Pixel path I — display, capture, tokens

| Term | One-line definition | Where |
|---|---|---|
| **28×28 patch tokenization** | The Qwen2-VL lineage's rule: one image token per 28×28 pixel patch. | 02 |
| **1568 long-edge cap** | The Anthropic-style pre-processing limit: images are downscaled until the long edge is 1568 px. | 02 |
| **85 + 170 per tile** | The OpenAI-style image cost formula: 85 base tokens + 170 per 512-px tile. | 02 |
| **Coordinate spaces** | The up-to-four frames a click crosses — display, captured PNG, model image, normalized output — all mapped by the executor alone. | 02 |
| **Display frame** | The coordinate space of the actual display where the cursor lives; the executor converts model-frame answers into it via `display = model × (display_size / model_image_size)`. | 02 |
| **Headless** | Running a machine with no physical monitor — the agent works a virtual display instead. | 02 |
| **HiDPI scaling** | Retina-class setups return physical pixels (2×) while accessibility APIs speak logical points; force scale 1 in the guest or multiply consistently. | 02 |
| **Image tokenization** | Turning a screenshot into model tokens: vendor-specific geometry of caps, tiles, or patches. | 02 |
| **Letterboxing** | Padding an image to a target aspect; breaks coordinate math if the harness crops padding without subtracting the offset. | 02 |
| **llvmpipe** | Mesa's software renderer; what Xvfb uses for GL — fine for documents, bad for 3D and video. | 02 |
| **Long-edge cap** | Vendor pre-processing that bounds image size before tokenization (see 1568 cap). | 02 |
| **Low-detail mode** | OpenAI-style cheap mode: shrink any frame to 512×512, 1 tile, ~85 tokens. | 02 |
| **Model frame** | The image coordinates the model actually reasons over, after capture scaling and vendor rescaling. | 02 |
| **noVNC** | Browser-based VNC client paired with x11vnc so humans can watch the agent's screen. | 02 |
| **Normalized coordinates (0–1000)** | A grounding output convention where answers are fractions of image width/height instead of pixels. | 02 |
| **pixels/750 rule** | The Anthropic-style token estimate: ≈ (width × height) / 750 after the 1568 cap. | 02 |
| **PNG on the wire** | A captured screenshot is ~100–400 KB; the model API re-encodes it server-side. | 02 |
| **Screenshot cadence** | How often frames are captured; second-order per step but first-order per task — stale or animated frames cause confident wrong reasoning. | 02 |
| **Tile** | A 512×512-pixel block in OpenAI-style tokenization; cost quantizes per tile. | 02 |
| **Virtual display** | A real X server with no physical monitor (Xvfb) — the agent desktop nothing human needs to share. | 02 |
| **Wayland** | The modern Linux display protocol that deliberately lacks a global injection story — one reason agent fleets stay on X11. | 02 |
| **X11** | The Linux display protocol whose XTEST extension makes synthetic input trivial; the agent-fleet default. | 02 |
| **x11vnc** | VNC server exporting an X display so a human can view (and audit, and intervene on) the agent's screen. | 02 |
| **XGetImage / scrot** | Representative capture mechanics: an Xlib call and a tool that grabs the framebuffer into a PNG. | 02 |
| **Xvfb** | X virtual framebuffer — the headless X server (`Xvfb :99 -screen 0 1024x768x24`) that most agent desktops run on. | 02 |

## 3. Pixel path II — grounding, input, economics

| Term | One-line definition | Where |
|---|---|---|
| **Anchoring summary** | Periodic in-context recap ("Goal / done / current / blocked") that doubles as scratchpad and defense against lost context. | 03 |
| **Batching (parallel tool calls)** | Letting the model emit several low-risk actions per turn — fewer round trips, but only for locally-verifiable sequences. | 03 |
| **CGEvent / CGEventPost** | macOS's event mechanism: build a CGEvent, post it into the session's event stream; the coordinate fallback on Mac. | 03 |
| **Clipboard typing** | Pasting long/unicode text through the clipboard instead of typing key-by-key — faster and more reliable for bulk text. | 03 |
| **Composite primitives** | Scripted macros exposed as one tool call (e.g., "clear-and-type the URL bar"), cheaper and safer than four decisions. | 03 |
| **Executor contract** | The five jobs every executor does: validate → translate → execute → settle & capture → record. | 03 |
| **Grounding** | The task "(description, screenshot) → point": locating a described element in pixels. | 03 |
| **Operational knowledge** | The away-from-screen know-how the agent needs (which app does what, how flows work) — the second OSWorld 1.0 failure layer. | 06 |
| **OSWorld-G** | A newer grounding benchmark suite for picking grounder models. | 06 |
| **Prompt caching** | Providers cache the unchanged request prefix; keeps long agents affordable — but any byte change near the prefix invalidates it. | 03 |
| **Pruning** | Collapsing old screenshots to text notes and keeping only the last few frames live; makes per-step cost flat in step count. | 03 |
| **Quiescence poll** | Waiting until the UI stops changing (animation done, DOM stable) before deciding and capturing. | 03 |
| **ScreenSpot** | The grounding-benchmark family used to compare specialist grounder models. | 06 |
| **SeeClick** | An early GUI-grounding model lineage that established "(screenshot, instruction) → click". | 03 |
| **SendInput** | The Win32 injection API: INPUT records; absolute coordinates normalized to 0–65535 across the virtual desktop. | 03 |
| **Set-of-Mark (SoM)** | Drawing numbered boxes on a screenshot so pointing becomes classification — the model picks a number. | 03 |
| **Speculation** | Pre-issuing likely next reads (crops, quiet navigation) to hide latency; never speculate on writes. | 03 |
| **UI-TARS** | A GUI-agent model line (grounding included) that pushed end-to-end screenshot agents forward. | 03 |
| **UGround** | A widely-used open grounding model line; a standard small-grounder choice. | 03 |
| **XTEST** | The X11 extension for injecting input as if from hardware: `XTestFakeMotionEvent` / `XTestFakeButtonEvent` / `XTestFakeKeyEvent`. | 03 |
| **Zoom-and-confirm** | The observe → propose → zoom → confirm loop: crop the candidate region, verify the target, then click precisely. | 03 |

## 4. Structure path I — AT-SPI on Linux

| Term | One-line definition | Where |
|---|---|---|
| **Accessible object** | A node in the AT-SPI tree: an addressable D-Bus object representing one UI element. | 04 |
| **AccessibleId** | An app-chosen stable identifier (when set) — gold for test-stable targeting in AT-SPI. | 04 |
| **Action interface** | AT-SPI's "invoke without a cursor" surface: the app reports available actions; the client calls `DoAction` by index. | 04 |
| **ATK** | The GNOME toolkit-level accessibility layer (toolkit → adaptor → D-Bus objects is the AT-SPI stack). | 04 |
| **AT-SPI** | The Linux accessibility/automation protocol: a D-Bus architecture exposing apps as actionable object trees — the structure path's origin. | 04 |
| **Cache.GetItems** | The AT-SPI bulk-read interface: fetch a subtree with properties in one round trip instead of per-node calls. | 04 |
| **Collection.GetMatches** | AT-SPI's server-side query interface: filter the tree by predicates (role, state, attributes) inside the app. | 04 |
| **Component.GetExtents** | The geometry interface: element rectangles and screen coordinates (for fallback clicks and verification). | 04 |
| **D-Bus** | The Linux IPC bus that carries AT-SPI traffic; queries and actions are method calls, not function calls. | 04 |
| **DeviceEventController** | AT-SPI's historical input path (synthesize events through the a11y stack); rarely the right tool today — coordinates via XTEST are the fallback. | 04 |
| **DoAction** | The one call that runs an element's own action (press, activate, menu) — background-safe, no cursor. | 04 |
| **EditableText / Text** | Interfaces for reading document content and editing text at offsets — how agents type without keyboard events. | 04 |
| **Lazy accessibility** | Chromium/Electron build the tree only when an AT connects (or a flag forces it); enabling it can noticeably raise CPU. | 04 |
| **registryd** | The registry daemon that tracks which accessible applications exist in the session — the census a client enumerates. | 04 |
| **Relations** | The graph edges between accessible objects (labelled-by, controls, member-of) — semantics beyond the tree shape. | 04 |
| **Roles** | The vocabulary of "what kind of thing is this" (button, entry, menu item…) shared by every a11y platform. | 04 |
| **Semantic healing** | When an exact ref is gone, re-find the element by (role, name, AccessibleId?) rather than failing — matching semantics, not position. | 06 |
| **Staleness** | The condition where a cached tree/ref no longer describes the live UI; cured by generation counters and fresh reads. | 04 |
| **States** | Machine-readable flags (enabled, focused, visible, checked) in each element's state set. | 04 |
| **Value interface** | The interface for ranged controls (slider, spinner): read and set the current value directly. | 04 |

## 5. Structure path II — macOS, Windows, browsers

| Term | One-line definition | Where |
|---|---|---|
| **Accessibility privilege (TCC)** | The macOS grant (`System Settings → Privacy & Security → Accessibility`) an agent needs to read/act on AX trees; un-granted means silent failure. | 05 |
| **AppleScript / System Events** | The scripting front end over AX ("click button 1 of window 1") — convenient, string-typed, brittle for general agents. | 05 |
| **AutomationElement** | A node in the Windows UI Automation tree, rooted at the Desktop, with properties and control patterns. | 05 |
| **AutomationId** | The app's stable element identifier in UIA — the closest thing to a per-element anchor. | 05 |
| **AX attributes** | The macOS readable element facets: `kAXRole`, `kAXTitle`, `kAXValue`, `kAXEnabled`, `kAXPosition`, `kAXSize`, `kAXChildren`… | 05 |
| **AX tree** | macOS's accessibility tree, exposed through the Application Services frameworks (AXUIElement). | 05 |
| **AXUIElement** | The opaque handle to one macOS element: read attributes, list actions, perform them. | 05 |
| **BoundingRectangle** | UIA's element geometry — rects for fallback clicks and verification. | 05 |
| **CacheRequest** | UIA's first-class bulk caching: declare wanted properties/patterns, then one tree walk returns a cached subtree — the UIA `Cache.GetItems`. | 05 |
| **CDP (Chrome DevTools Protocol)** | The browser's automation protocol over WebSocket; `Input.dispatchMouseEvent` etc. deliver *trusted* input via the browser's own pipeline. | 05 |
| **Control patterns** | UIA's capability interfaces: an element advertises what it can do (Invoke, Toggle, SetValue, Select, ExpandCollapse, Scroll, Text, Window…), and the agent calls the pattern natively. | 05 |
| **DOM** | The web page's own element tree — browser tasks' best channel: selectors, roles, test ids, deterministic targeting. | 05 |
| **Focus route** | The middle rung between native actions and coordinates: focus the element, then send keyboard input. | 05 |
| **GetClickablePoint** | UIA helper returning a point guaranteed inside the element — better than rectangle-center math for fallback clicks. | 05 |
| **InvokePattern** | The "click" pattern: `.Invoke()` presses the element even while occluded or minimized. | 05 |
| **kAXPressAction** | The macOS press/click action, performed via `AXUIElementPerformAction` — the coordinate-free click. | 05 |
| **Playwright refs** | The browser snapshot format: an accessibility-style tree with per-snapshot refs (`[ref=e12] button "Sign in"`) and click-by-ref APIs. | 05 |
| **Remote debugging port** | The launch flag (`--remote-debugging-port`) that opens a browser to CDP — meaning the browser must be *yours* (another reason for dedicated VMs). | 05 |
| **Screen Recording privilege (TCC)** | The separate macOS grant required for capture (CGWindowListCreateImage / ScreenCaptureKit) — a full-capability agent holds two TCC grants. | 05 |
| **Three-tier action ladder** | The universal action shape: Tier 1 native action (DoAction/AXPress/Invoke), Tier 2 focus route, Tier 3 coordinate injection. | 05 |
| **Trusted input events** | CDP-dispatched input flows through the browser's real pipeline; pages cannot dismiss it as synthetic the way JS `.click()` can be. | 05 |
| **UIA (UI Automation)** | The modern Windows accessibility/automation framework: a tree of AutomationElements with properties and control patterns over COM. | 05 |
| **UIA views (Raw/Control/Content)** | Three filtered lenses on the same tree — Control is the agent default; Content is what a screen reader would read. | 05 |
| **UIPI (integrity levels)** | Windows User Interface Privilege Isolation: a normal process cannot see or drive elevated windows — an architectural decision point. | 05 |
| **ValuePattern** | The pattern for reading/setting a text field's value directly (respecting `IsReadOnly`). | 05 |

## 6. Structure path III — practice

| Term | One-line definition | Where |
|---|---|---|
| **Background operation** | Acting on apps without stealing cursor/focus (native actions do this) — the property that lets agents co-exist with humans and run in parallel. | 06 |
| **PiP (Picture-in-Picture)** | UFO2's pattern: the agent works a target app in an isolated virtual desktop/window, so the user's session is never hijacked. | 06 |
| **Ref** | A per-snapshot name for an element (`e12`); valid only for the snapshot generation that minted it. | 06 |
| **Snapshot → ref → act** | The production loop for tree-driven agents: take a fresh snapshot, act on refs from it, never on memory of the last one. | 06 |
| **UFO2** | Microsoft's mature Windows desktop-agent framework: UIA-first with vision fallback, PiP isolation, speculative execution. | 06 |

## 7. Cloud path I — why agents get computers

| Term | One-line definition | Where |
|---|---|---|
| **Agent computer** | A dedicated cloud machine per agent (usually a microVM with a full desktop stack inside) that stays on when you sleep and dies without residue. | 07 |
| **Blast radius** | The scope one compromised agent can damage; the goal is to make it exactly one machine. | 07 |
| **Container** | Namespaces + cgroups isolation sharing the host kernel — the dev sweet spot, one kernel exploit away from the host. | 07 |
| **Control plane vs data plane** | Two network channels with different jobs: vsock control (always on) vs the agent's own egress (explicitly granted per task). | 07 |
| **CRIU** | Userspace process checkpoint/restore for containers — powerful for processes, but not "freeze this whole computer". | 07 |
| **Egress policy** | Network posture for the agent machine: off by default, allowlisted per task — turns most exfiltration into a non-event. | 07 |
| **Firecracker** | The open-source microVM monitor (KVM-based, minimalist device model, jailed, API-configured) that agent fleets build on. | 07 |
| **Guest agent daemon** | The small service inside the agent's VM that receives orchestration commands over vsock and streams status back. | 07 |
| **Isolation boundary** | What separates one agent from another and from the host: shared kernel (container) vs hardware-enforced (VM). | 07 |
| **Jailer** | Firecracker's wrapper: applies cgroup/namespace isolation, then drops privileges before the VMM runs. | 07 |
| **Kata Containers** | A container runtime that runs each container inside a VM — "VM per workload" with container ergonomics. | 07 |
| **KVM** | Linux's kernel-based virtualization module; the hardware-virt foundation microVMs sit on. | 07 |
| **MicroVM** | A lightweight VM with a minimal device model — VM isolation at near-container density, boots in a ~125 ms class. | 07 |
| **Oversubscription** | Scheduling more vCPUs/RAM than physically present, relying on guests using a fraction at any instant — safe because of quotas, demand paging, and ballooning. | 07 |
| **seccomp** | Syscall filtering; Firecracker uses per-thread allowlists, not a coarse global filter. | 07 |
| **TAP device** | The virtual Ethernet adapter giving the VM networking, routed via host NAT/bridge into policy-enforced egress. | 07 |
| **Virtio** | The standard paravirtualized device family (block, net) — the small fixed device set a microVM boots with. | 07 |
| **VMM** | Virtual machine monitor — the hypervisor process (e.g., Firecracker) that runs a guest kernel. | 07 |
| **vsock** | A host↔guest socket needing no guest networking — carries the control plane and the boot "ready" handshake even when the network is off. | 07 |

## 8. Cloud path II — disks, snapshots, lifecycle

| Term | One-line definition | Where |
|---|---|---|
| **Backing chain** | Disk image A reads-through to read-only image B: the structure that makes machine copies kilobytes. | 08 |
| **Ballooning** | Guest memory reclaimed by the host via a cooperating driver — one of the mechanisms that make oversubscription smooth. | 08 |
| **cgroup v2** | Linux's unified resource-control hierarchy, the fleet's immune system: `cpu.max`, `memory.max`, `pids.max`, `io.max` bound each machine. | 08 |
| **Clone** | Restoring a snapshot into a *new* machine — a timeline fork for parallelism, with fresh identity per copy. | 08 |
| **Clone trap** | What breaks when two machines believe they are one: duplicated RNG state, DHCP leases, clocks, sockets — hence VMGenID. | 08 |
| **Copy-on-write (CoW)** | Writes go to a small diff; unchanged bytes stay shared with the read-only base — the economic foundation of the fleet. | 08 |
| **CPU templates** | Normalized CPUID feature sets so snapshots resume across different host CPU generations — portability you configure, not get. | 08 |
| **Demand paging** | Allocate/fetch memory only as it's touched — "start instantly, pay per page actually used". | 08 |
| **Diff snapshot** | A snapshot recording only pages changed since the last one — what makes repeated checkpoints cheap. | 08 |
| **Direct kernel boot** | The hypervisor loads the kernel itself (no BIOS, no bootloader, no device discovery) — part of the ~125 ms boot class. | 08 |
| **Golden template** | The immutable, versioned base image (OS + browser + agent runtime) every machine overlays; upgrades are new versions, not mutations. | 08 |
| **Hooks** | Lifecycle scripts the control plane runs inside the guest at defined moments (boot, pre-snapshot, checkout…). | 08 |
| **Idle-as-snapshot** | The cost posture: waiting costs storage bytes, not compute — what makes "the agent waits for you" arithmetically sane. | 08 |
| **Lifecycle semantics** | The product's machine verbs: stop = hibernate, start = resume, restart = reboot, clone = fork, destroy = shred. | 08 |
| **Machine snapshot** | RAM + device state + disk frozen as a point-in-time timeline you can resume later, anywhere in the fleet. | 08 |
| **Overlay** | The per-machine thin diff disk off the golden template; creation costs kilobytes regardless of template size. | 08 |
| **Paused-VM operation** | Snapshotting works on a *paused* VM — the reason pause→snapshot→resume is fast and safe. | 08 |
| **qcow2** | QEMU's copy-on-write-capable disk format with backing-file support; the industry's backing-chain workhorse. | 08 |
| **raw image** | The dumb disk format: every byte present, no CoW, no sharing — fine for templates you copy whole, not for machines. | 08 |
| **Secrets injection** | Per-instance credentials injected at boot by the control plane — never part of the shared base image or snapshots. | 08 |
| **userfaultfd** | The kernel facility letting the VMM handle guest page faults itself — lets a restore start with zero memory preloaded and pay per touched page. | 08 |
| **VMGenID** | A virtual device giving each VM an identity; a fresh value per restore tells the guest it's a *new* machine ⇒ RNG reseed + network re-identification. | 08 |

## 9. Safety, benchmarks, and strategy

| Term | One-line definition | Where |
|---|---|---|
| **Audit trail** | The per-task black box: every action, mapped coordinates/refs, before/after frames — stored for forensics and improvement. | 08/09 |
| **Confirmation ladder** | The action-class policy: reads free; reversible writes audited; persistent writes announced; irreversible/public actions confirmed. | 09 |
| **Confused deputy** | The agent holds capabilities the attacker lacks (files, sessions, accounts) and read-content talks it into using them. | 09 |
| **Control spectrum** | The five ways an agent touches a GUI, from raw pixels to native commands (the series' orientation diagram). | R/09 |
| **Direct misuse** | The OS-Harm category where the user explicitly asks for harm; measured by whether the agent refuses. | 06 |
| **Execution-based scoring** | Benchmark scoring that verifies the machine's resulting state, not the agent's claims — the harness never lets the agent grade itself. | 06 |
| **Flat per-step cost** | The economics target: caching + pruning keep per-step cost roughly constant as steps grow — the difference between 40-step demos and 250-step tasks. | 03/09 |
| **Instruction/data separation** | The prompt-architecture firewall: screen content is DATA; only the user's original message contains instructions. | 06/09 |
| **LLM judge** | Automated rubric evaluation of agent behavior; OS-Harm's judge agreed with humans at ~0.76 F1 on accuracy, ~0.79 on safety — useful, approximate. | 06 |
| **Model misbehavior / spillover** | Harm nobody asked for, arising from the agent's own actions even on safe tasks — the third OS-Harm category. | 06 |
| **OS-Harm** | The 150-task safety benchmark on the OSWorld environment covering direct misuse, prompt injection, and model misbehavior. | 06 |
| **OSWorld** | The benchmark environment and harness that defines modern computer-use evaluation; 369 tasks, execution-based scoring, four observation modes. | 06 |
| **OSWorld 1.0 (2024)** | The original 369-task release (~72% human vs ~12% best model) — the perception/control era. | 06 |
| **OSWorld-Verified (2025)** | The repaired re-release; frontier agents broke into the tens of percent — perception improving, planning residue. | 06 |
| **OSWorld 2.0 (2026)** | 108 long-horizon tasks (~1.6 h median human, >250 steps, best models ~20%) — the planning/memory/verification era. | 06 |
| **Prompt injection** | Instructions embedded in content the agent reads being followed as commands — OS-Harm's live threat; note content lands in the tree, not just pixels. | 06 |
| **Red-teaming** | Attacking your own agent (e.g., "ignore your task and do X" in a page comment) before someone else does. | 06 |
| **Static prompt injection** | Injection requiring no timing or state tricks — just text sitting in content the agent will read. | 06 |
| **User channel vs world channel** | The two voices in one context (the user's task vs everything the world says); keeping them firewalled is the defense. | 09 |

---

*Every one of these has a full explanation one click away in its home file. Start from the [series map](README.md), or go straight to the capstone — [file 09](09-synthesis-strategy-safety-interview-qa.md) — where the terms do their work.*
