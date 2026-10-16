# 05 — Structure Path II: macOS, Windows, and the Browser Fast Lane

> **Where this file fits**
> ```
>   Linux   : AT-SPI over D-Bus            (file 04)
>   macOS   : AXUIElement over Mach IPC    ┐
>   Windows : UI Automation over COM       ├── this file
>   Browser : DOM/CDP over WebSocket       ┘   (the built-in fast lane)
> ```
> Every desktop OS grew the same organ for the same reason (assistive technology) — a structured, actionable model of the UI. The names differ; the shape does not. Learn the shape here, then map each platform's vocabulary.

---

## 1. macOS: the Accessibility (AX) API

### 1.1 The model

macOS apps expose an **AX tree** through the Application Services frameworks. An agent (or VoiceOver) connects to an app by process id and walks/reads/acts on elements:

```swift
let app = AXUIElementCreateApplication(pid)
var value: CFTypeRef?
AXUIElementCopyAttributeValue(app, kAXFocusedWindowAttribute as CFString, &value)
// → AXUIElementRef for the focused window; recurse via kAXChildrenAttribute…
AXUIElementPerformAction(buttonElement, kAXPressAction as CFString)   // press, no cursor
```

The vocabulary that matters:

| Piece | Examples |
|---|---|
| **Attributes** (read; some settable) | `kAXRole` (AXButton, AXTextField, AXWindow, AXMenuItem, AXStaticText, AXRow, AXLink…), `kAXSubrole`, `kAXTitle`, `kAXDescription`, `kAXValue` (text/selection/number), `kAXEnabled`, `kAXFocused`, `kAXPosition`, `kAXSize`, `kAXChildren`, `kAXSelectedTextRange` |
| **Actions** (`kAXActionsAttribute` lists them; `AXUIElementPerformAction` invokes) | `kAXPressAction` (click), `kAXIncrementAction`/`kAXDecrementAction` (steppers/sliders), `kAXShowMenuAction`, `kAXConfirmAction`, `kAXCancelAction`, `kAXPickAction` |
| **System-wide entry points** | `AXUIElementCreateSystemWide()` + `kAXFocusedUIElement` / `kAXFocusedWindow` — "what does the user's focus point at right now?" |
| **Settable attributes** | `kAXValue` (set a field's text), `kAXFocused` (move focus), `kAXSelectedTextRange` (place a caret) |

That inventory is the whole structure path on macOS: read role/title/value/geometry → decide → `AXPress` or set an attribute. The same three-rung ladder as Linux — *native action → focus route → coordinates* — maps to `AXPress` / `kAXFocused` + keystrokes / CGEvent clicks (file 03).

### 1.2 The realities: IPC, permissions, and quirks

- **It's IPC, not a library call.** Each attribute read is a synchronous round trip to the target app's accessibility server (the AXUIServer machinery behind the scenes). Reading a big tree attribute-by-attribute is slow; the standard mitigations are doing the same as elsewhere — fetch children in bulk where possible, scope searches to the focused window, and keep a client-side cache with a generation counter (same staleness discipline as file 04).
- **Permissions are TCC-mediated.** The controlling process needs the user-granted **Accessibility** privilege (System Settings → Privacy & Security → Accessibility). `AXIsProcessTrusted()` checks; `AXIsProcessTrustedWithOptions` can prompt. Un-granted → the API silently fails — a classic "works on my machine, does nothing on the demo machine" agent bug. **Sandboxed apps generally cannot hold this privilege**, and neither can most app-store shells; agents bind it to a helper process the user explicitly trusts.
- **Screen capture is a *different* permission.** Reading the AX tree is Accessibility; taking screenshots on modern macOS (CGWindowListCreateImage / ScreenCaptureKit) requires the **Screen Recording** privilege. A fully-capable macOS agent therefore holds two TCC grants, and the harness should *detect* both and degrade gracefully (tree-only mode when capture is denied).
- **A friendlier layer exists**: AppleScript/JavaScript-for-Automation driving `System Events` ("click button 1 of window 1 of process \"Safari\"") is the scripting front end over AX, plus Shortcuts as a sandbox-friendly action channel. Convenient for one-off automation; brittle for general agents (string-typed selectors), so production agents use AX directly.
- **Quirks worth knowing**: some apps expose `AXManualAccessibility`/lazy trees that populate on first request; Electron apps on macOS tend to have *thin first, rich after enablement* trees (mirrors Chromium's lazy a11y from file 04); and secure input mode can block synthetic keystrokes for password prompts (a feature — the agent should escalate to a human anyway).

---

## 2. Windows: UI Automation (UIA)

### 2.1 The model

UI Automation is the modern Windows accessibility/automation API: a **tree of `AutomationElement`s** rooted at the Desktop, with properties, control types, and — the special part — **control patterns**: capability interfaces that an element implements ("I can be invoked"; "I have a value you can set"). Clients live in any language that can talk COM; Python agents typically use `uiautomation` or `pywinauto`.

```python
from uiautomation import WindowControl   # (uiautomation package style)
win = WindowControl(searchDepth=1, Name="Calculator")
button = win.ButtonControl(Name="Seven")          # find by name + control type
button.GetInvokePattern().Invoke()                 # press it — no cursor, works behind windows
```

### 2.2 The tree and its views

UIA exposes the *same* underlying tree through three lenses — pick the right one and your dumps shrink dramatically:

| View | Content | Use |
|---|---|---|
| **Raw** | every element, including implementation detail | protocol spelunking, rarely agents |
| **Control** | elements that present info or are interactive | **the agent's default**: buttons, edits, menus — implementation panes partly filtered |
| **Content** | semantic content (what a screen reader would read) | reading tasks; merges/strips more chrome |

Properties agents lean on: `Name`, `AutomationId` (the app's stable identifier — gold for refs), `ControlType` (Button, Edit, CheckBox, ComboBox, ListItem, MenuItem, TabItem, Text, TreeItem, Slider, ScrollBar, …), `BoundingRectangle`, `IsEnabled`, `IsOffscreen`, `ProcessId`, `NativeWindowHandle`.

Search: `FindFirst`/`FindAll` with conditions — `PropertyCondition`s combined by `AndCondition`/`OrCondition` — scoped by `TreeScope` (`Children` vs `Descendants`, subtree vs subtree). The analog of AT-SPI's `Collection.GetMatches`.

### 2.3 Control patterns — the action layer

This is UIA's cleanest idea, and the concept agents should steal verbatim: an element advertises *what it can do* through pattern interfaces, and the agent calls the pattern instead of synthesizing input.

| Pattern | Action | Agent use |
|---|---|---|
| `InvokePattern` | `.Invoke()` | the "click" — works while the window is occluded/minimized |
| `TogglePattern` | `.Toggle()` | checkboxes, toggles; read `.Current.ToggleState` |
| `ValuePattern` | `.SetValue(text)`; check `.IsReadOnly` | set a text field directly; **respect read-only** |
| `SelectionItemPattern` | `.Select()` | pick a list/tab/radio item |
| `ExpandCollapsePattern` | `.Expand()` / `.Collapse()` | open trees and combo boxes; read `ExpandCollapseState` |
| `RangeValuePattern` | `.SetValue(number)` | sliders/spinners; `.LargeChange` for paging |
| `ScrollPattern` + `ScrollItemPattern` | `.Scroll(...)` / `.ScrollIntoView()` | scroll the container; bring an item into view without wheeling |
| `TextPattern` | document text by range | read documents/terminals robustly |
| `SelectionPattern` | read/verify selection in containers | verification |
| `WindowPattern` | `.Close()`, `.SetWindowVisualState(...)`, `WaitForInputIdle()` | window lifecycle; wait for the app to settle |
| `TransformPattern`, `GridPattern`, `LegacyIAccessiblePattern` | move/resize; tables; fallback | niche/fallback |

Plus two element-level conveniences with no AT-SPI analog: `element.SetFocus()` and `element.GetClickablePoint()` (a point you *know* is inside the element — much better than rectangle-center math for fallback clicks).

### 2.4 Realities

- **It's cross-process COM.** Every property fetch and pattern call is an IPC round trip through proxies; naive "walk and read everything" is slow exactly like on Linux. The cure is the same: **bulk caching**. UIA even has a first-class mechanism for it — `CacheRequest` (declare the properties/patterns you want, attach it to a tree walk, and receive a cached subtree in one pass). Use it; it's the UIA `Cache.GetItems`.
- **The provider quality spectrum is real.** Native WinForms/WPF/UWP apps expose rich UIA; classic Win32 apps ride a proxy; apps using custom-drawn controls may expose almost nothing; Java/legacy toolkits vary. Same conclusion as Linux: coverage is per-app.
- **Integrity levels (UIPI).** A normal-integrity client cannot see or drive windows of elevated (admin) processes. If the workflow needs an elevated app, the agent must run elevated (or not at all) — an architectural decision, not a runtime patch.
- **Background friendliness.** `Invoke` and friends are as background-safe as `DoAction` on Linux: no cursor move, no foreground grab. Menus and some frameworks still misbehave when not focused, so the *focus route* (SetFocus + keyboard) remains a common middle rung.
- **Tooling.** Microsoft's own Inspect.exe and Accessibility Insights are the debuggers of record for "why doesn't my selector match"; UFO2 (file 06) is the flagship proof that a UIA-first desktop agent is viable — notably it *does not* rely on UIA alone; it pairs it with vision.

---

## 3. The browser fast lane: DOM, CDP, Playwright

For web content — a huge fraction of real computer-use tasks — there is a *third* structured channel that beats both AT-SPI and UIA in ergonomics: the **DOM** itself, via the browser's automation protocols.

### 3.1 Why it's the best channel when available

| Property | Consequence |
|---|---|
| The DOM is the *source* of the accessibility tree | you can skip the OS tree and read/act at the origin |
| Stable, addressable elements | selectors, roles, text, test ids — deterministic targeting |
| CDP input events are **trusted** | `Input.dispatchMouseEvent` etc. flow through the browser's own input pipeline; pages can't dismiss them as synthetic the way they can with JS `element.click()` |
| Everything is scriptable | form filling, network interception, storage, download handling |
| Modern agent tooling speaks it natively | Playwright's AI-oriented snapshot: an accessibility-style tree with **refs** (`[ref=e12] button "Sign in"`), click-by-ref API |

Playwright's ref model is exactly the snapshot→ref→act loop of file 04, living inside the browser:

```
- heading "Sign in" [level=1]
- textbox "Email" [ref=e8]
- textbox "Password" [ref=e9]
- button "Sign in" [ref=e10]
```

…the model replies `click(e10)`; the runtime resolves the ref to the live DOM node and dispatches a trusted click. No coordinates, no screenshot, no OS permissions.

### 3.2 Caveats (the same three, again)

- **Refs go stale on navigation.** Same discipline as everywhere: refs belong to a snapshot generation; re-snapshot after navigation; act on fresh refs.
- **CDP requires access to the browser's debug port.** That means the browser is *yours* (launched with `--remote-debugging-port`, or via a driver). Detecting and attaching to "the user's existing browser" is messy — another reason agent computers are dedicated VMs.
- **It only covers the browser.** Half of real workflows leave the browser (file dialogs, messenger apps, terminals, spreadsheets) — so even web-first agents need the OS-channel fallback below them.

### 3.3 The three-channel ladder for a web task

1. **DOM/CDP/Playwright** — cheapest, most robust, no pixels;
2. **OS tree** (Chromium exposes AT-SPI/UIA/AX) — same semantics at OS granularity, robust to a missing debug port;
3. **Pixels** — canvas/WebGL apps, human-in-the-loop flows, screenshot-only visual checks.

---

## 4. Cross-platform comparison

| Dimension | Linux (AT-SPI) | macOS (AX) | Windows (UIA) | Browser (DOM/CDP) |
|---|---|---|---|---|
| Tree access | D-Bus interfaces | `AXUIElement` API (Mach IPC) | COM `AutomationElement` tree | DOM / CDP; Playwright refs |
| Native action | `Action.DoAction` | `AXUIElementPerformAction` | Control patterns (`Invoke`, `SetValue`…) | DOM events / CDP input |
| Geometry | `Component.GetExtents` | `kAXPosition`/`kAXSize` | `BoundingRectangle` | `getBoundingClientRect` |
| Bulk-read API | `Cache.GetItems` | (none first-class; cache client-side) | `CacheRequest` | trivial (JSON/scriptable) |
| Permissions | none to connect (session-private bus) | **Accessibility** (+ **Screen Recording** for capture) | integrity levels (UIPI) constrain visibility | debug-port access; launch flags |
| Background actions | yes (`DoAction`) | yes (`AXPress`) | yes (`Invoke`) | yes (headless/real browser) |
| Coordinate fallback | XTEST (X11) | CGEvent | SendInput | CDP input |
| Query/filter | `Collection.GetMatches` | manual walk | conditions (`FindAll`) | querySelector / locators |
| Lazy trees | Chromium-family: on-demand a11y | Electron: thin-until-enabled | UIA providers vary by app | N/A (DOM is always there) |
| Stable IDs | `AccessibleId` (when set) | (rare; use AXIdentifier when exposed) | `AutomationId` (common) | CSS selectors, test ids |

### 4.1 The universal shape (write this down once)

```
        ┌────────────────────────────────────────────────────────────┐
        │  TIER 1  native action      DoAction / AXPress / Invoke    │  no cursor, no foreground, run in background
        │  TIER 2  focus route        GrabFocus+keys / SetFocus+keys │  no cursor, needs focus semantics
        │  TIER 3  coordinates        XTEST / CGEvent / SendInput    │  the pixel path's hands (file 03)
        └────────────────────────────────────────────────────────────┘
```

Every platform's tree speaks the same concept language — **role, name, state, geometry, children** — with cosmetically different enums, and every platform gates the whole structure path behind **one permission or integrity check** that must be detected and handled up front, not discovered mid-task.

> **Interview angle**
> "You're building an agent that must work on Windows, macOS, and Linux — how do you structure the perception/control layer?" The senior answer: define an internal element model (id, role, name, states, rect, actions[]) and three adapters (UIA, AX, AT-SPI) that implement *find*, *invoke*, and *extents*; coordinate click injection as an explicitly separate capability with per-platform backends; and a snapshot generation counter that expires refs uniformly across platforms. The DOM adapter slots in as a fourth, richer backend when the target is a browser you launched.

---

## 5. Key ideas to carry forward

- macOS: `AXUIElement` gives the same role/title/value/geometry/action model; `AXPressAction` is the coordinate-free click; **Accessibility** and **Screen Recording** are two separate TCC grants the agent must detect and adapt to.
- Windows: UI Automation's **control patterns** are the cleanest formalization of "native actions" anywhere — `Invoke`, `Toggle`, `SetValue`, `Select`, `ExpandCollapse`, `ScrollIntoView` — backed by `CacheRequest` for performance and constrained by UIPI integrity levels.
- Browsers have a fast lane: DOM via CDP/Playwright, with trusted synthetic input and refs — the same snapshot→ref→act discipline, better ergonomics, browser-only scope.
- All four channels collapse into one three-tier action ladder (native → focus → coordinates) plus one element model; design the abstraction once and the per-platform code becomes a thin adapter.
- The failure and staleness disciplines from file 04 (fresh reads, semantic identity, generation counters, bulk reads, coverage gaps) are platform-independent — because the underlying economics and physics are.

Next: [06 — tree-first agents in practice, and the safety turn](06-structure-path-iii-practice-and-safety.md).
