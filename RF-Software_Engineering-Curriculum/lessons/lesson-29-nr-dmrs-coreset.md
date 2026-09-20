[← Table of Contents](../README.md) · [← Lesson 28](lesson-28-nr-numerology-ssb.md)

---

# Lesson 29 — 5G NR: DMRS Channel Estimation and PDCCH's CORESET — Reference Signals as a Configurable Resource

*Stage 8 — 5G NR, continued. Lesson 11's channel-estimation math and Lesson 20's control-channel concepts, both generalized — and DMRS density turns out to be a direct, derivable consequence of Lesson 10's coherence time, not an arbitrary spec choice.*

### 1. Intuition

LTE's CRS (cell-specific reference signals) are always-on: dense, transmitted in every subframe across the whole bandwidth, whether or not any UE actually has data scheduled there — simple, but genuinely wasteful overhead, and a real source of inter-cell interference since neighboring cells' CRS can collide on the same resource elements. NR's answer is **DMRS** (demodulation reference signals): UE-specific, transmitted **only** within the time-frequency resources actually scheduled for that UE's transmission — exactly the same "make it configurable instead of fixed" philosophy from Lesson 28's numerology, now applied to reference signals themselves.

### 2. The engineering problem that forces this

Two consequences follow directly from making reference signals sparse and scheduled rather than dense and always-on. First: Lesson 11's toy example assumed a pilot at *every* subcarrier — unrealistic for real DMRS density, so a receiver must now **interpolate** channel estimates across data-bearing resource elements that have no pilot of their own, a genuinely new step Lesson 11 didn't need. Second: how sparse can DMRS be *before* interpolation starts producing real, systematic error rather than just added noise — and does that limit depend on the UE, the deployment, or something else entirely?

### 3. Mathematical foundation

Same Lesson 11 machinery, unchanged: $\hat H[k] = Y[k]/P[k]$ at each pilot-bearing resource element. New requirement: **interpolate** $\hat H$ across the data-bearing REs between pilots, in frequency and (depending on configuration) in time.

### 4. Formal definition

**DMRS mapping type** (Type 1/Type 2) controls the comb pattern and density of pilot subcarriers within a symbol. **Additional DMRS symbols** are extra reference-signal symbols placed later within a slot, beyond the default front-loaded position, configured specifically for faster-varying channels. **CORESET** (CONtrol REsource SET) is a configurable time/frequency region — scoped to a BWP, unlike LTE's fixed "first few symbols across the whole bandwidth" — within which PDCCH candidates can appear; the **search space** (Lesson 20's blind-decoding concept) then defines *which specific candidates within a CORESET* a given UE actually checks. CORESET and search space are related but distinct: one is "where control information could physically be," the other is "which of those locations this UE bothers to try."

### 5. Derivation — DMRS density is forced by Lesson 10's coherence time and bandwidth, not chosen arbitrarily

For interpolation between pilots to be *valid* rather than merely hopeful, the channel must not have changed uncorrelatedly between the two pilot points being interpolated. In **frequency**: if DMRS pilot spacing exceeds the channel's coherence bandwidth $B_c$ (Lesson 10 §3, $B_c\approx1/\tau_{spread}$), the channel's frequency response has already decorrelated between adjacent pilots — interpolation is no longer smoothing over a slowly-varying quantity, it's guessing across a genuine discontinuity, producing **systematic** estimation error, not just added noise. In **time**: if DMRS symbols are spaced (within or across slots) further apart than the channel's coherence time $T_c$ (Lesson 10 §3, $T_c\approx1/\text{Doppler spread}$), the same problem occurs along the time axis.

This gives a precise, physically-grounded design rule — and it directly explains "additional DMRS symbols" as a feature rather than an arbitrary option: Doppler spread scales with UE velocity ($f_d = v f_c/c$, the standard Doppler formula), so faster UEs have proportionally *shorter* coherence time $T_c$. A default, front-loaded, single-symbol DMRS configuration that's perfectly adequate for a slow-moving UE can leave the channel free to decorrelate significantly by the end of a slot for a fast-moving one — precisely why the network can configure additional DMRS symbols later in the slot specifically for higher-mobility scenarios. This is not a separate mobility feature bolted onto NR; it's Lesson 10's coherence-time math, applied directly to a real configuration decision.

### 6. Visualization

```
LTE CRS (always-on, dense, whole bandwidth, every subframe):
[R . . R . . R . . R . . R . . R . .]  ← reference REs scattered everywhere, always

NR DMRS (only within THIS UE's scheduled allocation):
[. . . . . . . . . . . . . . . . . .]  ← unscheduled REs: nothing transmitted here for this UE
        [R . . R . .]                   ← DMRS only inside the scheduled PRBs, only when needed

CORESET vs Search Space:
CORESET:      [ region where PDCCH COULD be, scoped to this BWP ]
Search space:  [ x ]      [ x ]              [ x ]   ← the specific candidates THIS UE checks
```

### 7. Worked example

UE at highway speed, $v=30$ m/s, carrier $f_c=3.5$ GHz: Doppler spread $f_d = vf_c/c = 30\times3.5\times10^9/3\times10^8 = 350$ Hz. Using the common rough approximation $T_c\approx1/(4f_d)$: $T_c\approx1/1400\approx0.71$ ms. At numerology $\mu=1$ (30 kHz, 0.5 ms slots), a single front-loaded DMRS symbol leaves the channel to potentially decorrelate meaningfully by slot's end relative to this $T_c$ — borderline enough to justify configuring additional DMRS. Now the same UE at $f_c=28$ GHz (FR2 mmWave): $f_d = 30\times28\times10^9/3\times10^8=2800$ Hz — **8× worse** for the *identical* physical velocity, purely because Doppler scales with carrier frequency. This is a real, often underappreciated FR2 characteristic: mobility support gets genuinely harder at mmWave for reasons with nothing to do with bandwidth or propagation loss.

### 8. Code

```python
import numpy as np

N = 64
h_ch = np.array([1.0, 0.5*np.exp(1j*np.pi/3), 0.2*np.exp(-1j*np.pi/4)])
H_true = np.fft.fft(h_ch, N)

pilot_spacing = 4                          # DMRS every 4th subcarrier (sparse, realistic)
pilot_idx = np.arange(0, N, pilot_spacing)
pilots = np.ones(len(pilot_idx), dtype=complex)

noise_var = 0.01
Y_pilot = H_true[pilot_idx]*pilots + np.sqrt(noise_var/2)*(np.random.randn(len(pilot_idx))+1j*np.random.randn(len(pilot_idx)))
H_est_at_pilots = Y_pilot / pilots

# Linear interpolation across the un-piloted subcarriers -- the new step vs Lesson 11
H_interp = np.interp(np.arange(N), pilot_idx, H_est_at_pilots.real) + \
       1j*np.interp(np.arange(N), pilot_idx, H_est_at_pilots.imag)

error = np.abs(H_interp - H_true)
print("mean interpolation error:", np.mean(error), " max:", np.max(error))
```

### 9. Experiment

Run §8's code at increasing `pilot_spacing` (2, 4, 8, 16) against the same fixed multipath channel, and plot mean interpolation error against spacing. Separately compute the channel's actual coherence bandwidth from its delay spread (Lesson 10's formula). You should see error stay low while pilot spacing (converted to Hz, via subcarrier spacing × spacing-in-subcarriers) remains well under $B_c$, then rise sharply once it exceeds it — direct, numeric confirmation of §5's derivation.

### 10. Expected output

A clear knee in the error-vs-spacing curve located near the channel's coherence bandwidth, not a smooth, gradual degradation — matching §5's claim that exceeding $B_c$ produces qualitatively different (systematic) error, not just proportionally more noise.

### 11. Debugging exercise

Symptom, no cause given: your NR channel estimator performs well in a stationary/low-mobility test case, but for a simulated high-mobility UE (highway speed), equalized symbol error grows specifically toward the **end** of each slot — worst far from the DMRS symbol, better near it.

Using §5 and §7's numbers: what's decorrelating, and why would the error specifically grow with *distance from the DMRS symbol in time*, rather than being uniform across the slot?

**Root cause (revealed):** the channel's coherence time $T_c$, shortened by this UE's high Doppler spread, is being exceeded by the *time* gap between the (single, front-loaded) DMRS symbol and later symbols in the slot — exactly §7's borderline-or-worse scenario. The channel estimate from the DMRS symbol is accurate *at that instant*, but becomes progressively less representative of the true channel the further in time you are from it, since the channel has had more time to decorrelate — hence error growing specifically with distance from the pilot, not uniformly. The fix is precisely what §5 predicts: configure additional DMRS symbols later in the slot for this UE's mobility profile, giving the estimator fresher channel information closer to the symbols that need it.

### 12. Common mistakes

- Assuming NR channel estimation is "the same as LTE's, just with fewer pilots" — the interpolation requirement is a genuinely new, necessary step that Lesson 11's dense-pilot toy example never needed to address.
- Treating DMRS spacing as an arbitrary configuration knob rather than one governed by real coherence bandwidth/time constraints (§5) — it's tunable, but not arbitrarily so; exceeding the physical coherence limits breaks the underlying assumption interpolation relies on, regardless of how the parameter is configured.
- Conflating **CORESET** (where PDCCH candidates could physically be) with **search space** (which specific candidates a given UE actually checks, Lesson 20's blind-decoding concept) — related, but genuinely distinct configuration objects, easy to blur together in casual discussion.

### 13. Real RF/cellular application

DMRS configuration (density, additional-symbol placement, mapping type) is a real, live RRC-configured parameter, tuned per-UE based on its measured or expected mobility — exactly the kind of dynamic scheduling decision a real gNB makes continuously. CORESET flexibility (versus LTE's one fixed PDCCH region) is central to how NR supports multiple BWPs and numerologies simultaneously on a single carrier. This lesson's synthesis — Lesson 10's coherence physics, Lesson 11's estimation math, and Lesson 20's control-channel concepts, all specialized for NR's configurability — is exactly the kind of "why is this parameter configured this way for this UE" reasoning that shows up directly in real PHY/L1 engineering and RF conformance work.

### 14. Interview question

*"Why does 5G NR support configurable 'additional DMRS symbols' rather than always using one fixed reference-signal density?"* Answer from §5 and §7: required DMRS density is governed by the channel's coherence time, which itself depends on UE velocity (via Doppler spread) — a single fixed density would either waste overhead for slow/stationary UEs or fail to track a genuinely fast-varying channel for high-mobility ones. Configurability lets the network match reference-signal overhead to actual channel dynamics instead of over- or under-provisioning for every UE universally.

### 15. Active recall

1. Why does NR channel estimation need an interpolation step that Lesson 11's original example didn't?
2. In one sentence, what physical quantity from Lesson 10 sets the limit on how sparse DMRS can safely be, in frequency and in time respectively?
3. What's the precise difference between a CORESET and a search space?

### 16. Mini assignment

Compute Doppler spread and approximate coherence time for pedestrian (~1.5 m/s), urban vehicular (~15 m/s), and highway (~30 m/s) UE speeds, at both 3.5 GHz and 28 GHz carrier frequencies — six combinations total — and determine for each whether single-symbol DMRS is likely adequate or additional DMRS would be warranted, paying particular attention to how dramatically the FR2 (28 GHz) column changes the picture for the *same* physical speeds.

### 17. Exit test

1. Write the Doppler-spread formula and compute it for a 20 m/s UE at 2 GHz.
2. True or false, with a one-line reason: "DMRS pilot spacing can be made arbitrarily sparse without consequence, as long as some pilots are present."
3. Your reasoning from §11 about error growing with distance from the DMRS symbol, if you haven't sent it.

---

**Next:** Lesson 30 completes the essential NR arc by returning to **PRACH** — NR's random-access preamble, and how Zadoff-Chu sequences (Lesson 19's proof) get reused a second time for an entirely different purpose: an unscheduled UE announcing its presence for the very first time.

---

[← Lesson 28](lesson-28-nr-numerology-ssb.md) · [Table of Contents](../README.md) · [Lesson 30 →](lesson-30-prach-timing-advance.md)
