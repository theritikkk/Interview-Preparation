[← Table of Contents](../README.md) · [← Lesson 30](lesson-30-prach-timing-advance.md)

---

# Lesson 31 — Building the Pipeline: A Scoped LTE PSS/SSS/PBCH Receiver in C++

*Stage 9, synthesis lesson. Every lesson since Lesson 19 built one correct piece in isolation. This lesson is about what happens when you connect them — and it turns out "each piece works" and "the pipeline works" are genuinely different claims, in a way no single-stage unit test can ever catch.*

### 1. Intuition

Lessons 1–30 each built one thing, tested in isolation, in its own small example. Real engineering is integration: taking many separately-correct pieces and making them work *together*, where the interface between stage A and stage B has to be exactly right, not just each stage individually. This lesson builds one real C++ program — the direct fulfillment of Project 7 — that takes a signal and walks it through PSS detection, SSS detection, frame-timing resolution, and PBCH resource-element extraction and demodulation, using Lesson 19's math and Lessons 22–27's implementation techniques together for the first time.

### 2. The engineering problem that forces this

Each stage of a receiver pipeline was built and tested against its own idealized inputs and outputs. But stage A's *actual* output — a detected timing offset, say — has to be consumed by stage B in *exactly* the convention stage A intended, and if that convention was never written down precisely (offset from what reference point, specifically?), two individually-correct pieces can still fail together. This is a distinct category of bug from anything a single-stage unit test can ever catch, because a unit test only ever exercises a stage against its own author's assumptions.

### 3. Mathematical foundation

Reusing everything: Lesson 19's Zadoff-Chu matched filtering (PSS/SSS), Lesson 23's streaming buffer pattern, Lesson 22's memory layout. One genuinely new, small piece: LTE PBCH is QPSK-modulated, and a real receiver produces **soft bits (LLRs)** for a channel decoder rather than hard 0/1 decisions — worth deriving properly since it connects directly back to Lesson 7's Gaussian channel model.

### 4. Formal definition

The pipeline has four explicit stages with explicit contracts: **Stage A** (PSS detect) outputs `{sample_offset, N_ID2}`. **Stage B** (SSS detect) consumes that offset, outputs `{N_ID1, half_frame_boundary}`. **Stage C** (PBCH RE extraction) consumes both, maps the correct resource elements (center 72 subcarriers, 4 specific OFDM symbols, **excluding** REs reserved for cell-specific reference signals) to a linear QPSK symbol stream. **Stage D** (QPSK→LLR) converts each symbol to two soft bits. Writing these contracts down explicitly, in exactly this much detail, is itself the point of this lesson — not an afterthought.

### 5. Derivation — QPSK soft-bit (LLR) computation, from Lesson 7's Gaussian model

For a received value $r = a + n$ (one dimension, real or imaginary part of a QPSK symbol; recall Lesson 8 §5(a) proved I and Q decisions are exactly independent BPSK problems), with transmitted amplitude $\pm a$ and Gaussian noise $n\sim N(0,\sigma^2)$, the log-likelihood ratio for the corresponding bit is:
$$\text{LLR} = \ln\frac{P(r\mid \text{bit}=1)}{P(r\mid \text{bit}=0)} = \ln\frac{\exp(-(r-a)^2/2\sigma^2)}{\exp(-(r+a)^2/2\sigma^2)} = \frac{(r+a)^2-(r-a)^2}{2\sigma^2} = \frac{4ar}{2\sigma^2} = \frac{2ar}{\sigma^2}$$
This is the standard Gaussian-channel LLR formula, and because Lesson 8 already proved QPSK's I and Q are exactly independent BPSK-equivalent decisions, it applies separately and directly to the real part (one bit) and imaginary part (the other bit) of every received PBCH symbol — no new derivation needed beyond combining two results you already have.

### 6. Visualization

```
[ raw IQ stream ]
        ↓
[ Stage A: PSS matched filter (Lesson 19) ]  → {sample_offset, N_ID2}
        ↓
[ Stage B: SSS matched filter (Lesson 19) ]  → {N_ID1, half_frame_boundary}
        ↓
[ Stage C: PBCH RE extraction ]              → QPSK symbol stream (CRS REs excluded)
        ↓
[ Stage D: QPSK → LLR (this lesson, §5) ]    → soft bits, ready for a channel decoder
```

### 7. Worked example

PBCH occupies the center 72 subcarriers across 4 OFDM symbols (the first 4 symbols of slot 1, subframe 0) — but for a 2-antenna-port configuration, some of those resource elements within that span are reserved for cell-specific reference signals and must be *excluded* from the PBCH symbol stream entirely, not demodulated as data. Getting this exclusion pattern wrong doesn't crash anything — it silently feeds reference-signal energy into the data stream as if it were PBCH content, corrupting every subsequent bit.

### 8. Code

```cpp
#include <vector>
#include <complex>
#include <cmath>
#include <cstdio>

struct PssResult   { size_t sample_offset; int n_id2; float peak_score; };
struct SssResult   { int n_id1; bool second_half_frame; };

// Stage A: reuses Lesson 19's matched-filter-bank approach directly
PssResult detect_pss(const std::vector<std::complex<float>>& rx,
                      const std::vector<std::vector<std::complex<float>>>& pss_refs) {
    PssResult best{0, -1, -1.0f};
    for (int nid2 = 0; nid2 < 3; ++nid2) {
        // correlate rx against pss_refs[nid2] (conjugate, time-reversed -- Lesson 6)
        // ... (correlation loop omitted here; identical structure to Lesson 19 Sec 8)
        float score = 0.0f; size_t pos = 0;  // placeholder for the real correlation result
        if (score > best.peak_score) best = {pos, nid2, score};
    }
    return best;
}

// Stage D: the new piece this lesson derives
struct SoftBits { float llr_i, llr_q; };

SoftBits qpsk_to_llr(std::complex<float> r, float noise_variance) {
    const float a = 1.0f / std::sqrt(2.0f);         // per-dimension QPSK amplitude
    return { 2.0f * a * r.real() / noise_variance,
             2.0f * a * r.imag() / noise_variance };
}

// Stage C: EXPLICIT contract -- offset is measured from the PSS correlation peak,
// which per Lesson 19's derivation corresponds to the LAST sample of the PSS symbol.
// Every downstream stage must agree on this exact reference point, not assume one.
std::vector<std::complex<float>> extract_pbch_res(
        const std::vector<std::complex<float>>& rx,
        size_t pss_peak_offset, int n_id1, bool second_half) {
    std::vector<std::complex<float>> pbch_syms;
    // ... resource-grid indexing using pss_peak_offset as the anchor, skipping
    //     CRS-reserved REs per Sec 7 -- structure shown, full RE table omitted
    //     deliberately (out of scope for a learning implementation, Sec 12)
    return pbch_syms;
}
```

### 9. Experiment

Build a simulated end-to-end test signal: generate PSS (known $N_{ID}^{(2)}$), SSS (known $N_{ID}^{(1)}$), and PBCH symbols carrying known bits, concatenate with correct timing, run it through all four stages, and confirm the final LLRs have the **correct sign** for every transmitted bit (positive LLR → bit more likely 1, matching what was actually sent) — the full pipeline validated end-to-end, not stage by stage.

### 10. Expected output

Correct $N_{ID}^{(2)}$, correct $N_{ID}^{(1)}$, and LLR signs matching the known transmitted bits for every PBCH symbol in the noiseless case; LLR *magnitude* should shrink as you add noise, while signs should remain mostly correct until SNR gets genuinely poor — a soft-output pipeline degrading gracefully, exactly as Lesson 7's Gaussian model predicts.

### 11. Debugging exercise

Symptom, no cause given: Stage A's PSS detector passes its own unit test perfectly. Stage B's SSS detector passes its own unit test perfectly, given a correct timing offset as input. Stage C and D pass their own unit tests given correctly-positioned resource elements as input. Run all four stages *together* on the exact same underlying test signal, and PBCH output is garbage.

Every individual piece is provably correct. What category of problem does that leave?

**Root cause (revealed):** an **interface contract mismatch**, not an algorithmic bug in any single stage — precisely the failure mode this lesson opened with. A very common concrete version: Stage A reports `sample_offset` referenced to one point (say, the *start* of the correlation window) while Stage C's RE-extraction code assumes it's referenced to a *different* point (say, the *last sample* of the PSS symbol, as §8's code comment explicitly specifies) — an off-by-a-fixed-amount error, structurally identical to Lesson 9 §11's group-delay bug, but now occurring *between modules* instead of within one filter. Each stage's unit test only exercised that stage against its *own* author's assumed convention, so neither test could ever catch a disagreement about what the convention actually was. The fix is exactly what §4 modeled: write the exact contract down, explicitly, for every stage boundary — and add integration tests that feed one real stage's actual output into the next stage, not just synthetic, already-correctly-formatted inputs.

### 12. Common mistakes

- Assuming "each piece has a passing unit test" implies the assembled pipeline works — the precise trap in §11; integration testing is a genuinely distinct activity from unit testing, not a formality once units pass.
- Leaving a stage's output convention (offset from what point, exactly) undocumented or implicit — the single most common root cause of the exact bug class in §11.
- Attempting full, spec-exact PBCH resource-element mapping (every antenna-port CRS-puncturing pattern) as a first-pass implementation instead of a clearly scoped, explicitly labeled simplification — scope creep that turns an achievable learning project into an unfinishable one; §8's code deliberately flags this boundary rather than pretending completeness.

### 13. Real RF/cellular application

This exact integration discipline — precise, written stage-to-stage contracts, and end-to-end tests beyond isolated unit tests — is how real PHY codebases like srsRAN are actually structured and validated, and "each module passes its own tests, the pipeline still fails" is a genuinely common, real bug category in production modem software, closely tied to the source-code-reading skills flagged early in this curriculum's roadmap. This lesson is Project 7, substantially complete.

### 14. Interview question

*"You've unit-tested every stage of a receiver pipeline individually, and each one passes. Why might the full end-to-end pipeline still fail, and how would you debug it?"* Answer from §11: interface/contract mismatches between stages are invisible to isolated unit tests by definition, since each test only checks a stage against its own author's assumed convention. Debug by writing down each stage's exact output convention explicitly and adding integration tests that chain real stage outputs into the next stage's input, rather than only testing each stage against idealized, pre-formatted data.

### 15. Active recall

1. Why can every individual stage of a pipeline be provably correct while the assembled pipeline still fails?
2. Derive, in one line, why the QPSK LLR formula in §5 needs no new derivation beyond Lesson 7 and Lesson 8.
3. Why does §8's code include an explicit comment about what `pss_peak_offset` is measured from, rather than leaving it implicit?

### 16. Mini assignment

Deliberately introduce the exact mismatch from §11 into your own pipeline (change Stage C's assumed reference point by a small fixed offset relative to what Stage A actually reports) and confirm it reproduces the "each stage passes alone, the pipeline fails together" signature — causing and observing the bug yourself, not just reading about it.

### 17. Exit test

1. State, in your own words, the difference between a unit test and an integration test, using this lesson's pipeline as the example.
2. True or false, with a one-line reason: "If every stage of a pipeline has a passing unit test, the assembled pipeline is guaranteed to work correctly."
3. Your reasoning from §11 about the interface-mismatch bug, if you haven't sent it.

---

**Next:** Lesson 32 reuses this same pipeline architecture for NR: SSB detection generalized to handle Lesson 28's configurable numerology — the direct fulfillment of Project 8, and a genuine test of whether this lesson's stage-contract discipline actually generalizes or was accidentally LTE-specific.

---

[← Lesson 30](lesson-30-prach-timing-advance.md) · [Table of Contents](../README.md) · [Lesson 32 →](lesson-32-nr-ssb-msequence.md)