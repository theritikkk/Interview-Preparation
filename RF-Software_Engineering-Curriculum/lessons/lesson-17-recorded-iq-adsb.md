[← Table of Contents](../README.md) · [← Lesson 16](lesson-16-gnu-radio-fm-demod.md)

---

# Lesson 17 — Recorded IQ Datasets: File Formats, SigMF, and Decoding ADS-B

*Stage 5 — Software-Defined Radio, continued. Where "I can process a signal I generated" becomes "I can correctly interpret a file someone else recorded, with no hand-holding" — and where Lesson 6's matched filter gets pointed at a real digital protocol for the first time.*

### 1. Intuition

So far you've either simulated signals or captured your own live from an SDR. There's a third, often faster, way to develop and test receiver code: work from a file someone else already recorded. This decouples "is my algorithm right" from "do I have the right hardware, antenna, location, and timing" — genuinely valuable for iterating quickly. But a raw IQ file is just a sequence of bytes. Without an agreed convention for what those bytes *mean* — data type, byte order, I/Q interleaving — and metadata about *how it was captured* — sample rate, center frequency — the file is meaningless. This lesson covers both halves: reading any real IQ file correctly, and a genuinely satisfying real decode target, ADS-B, to prove the whole pipeline end to end.

### 2. The engineering problem that forces this

A raw capture file carries no self-describing information about its own contents: is a sample an 8-bit unsigned integer, a 16-bit signed integer, or a 32-bit float? Are I and Q interleaved as I,Q,I,Q,... or stored as separate blocks? What sample rate and center frequency was it captured at — without that, you don't even know what real-world frequency range you're looking at, or what physical duration a "symbol period" corresponds to. This is precisely why a metadata standard exists, and precisely why blindly trusting an assumed format is the single most common way to waste an afternoon debugging code that was never actually broken.

### 3. Mathematical foundation

**Format conversion** for unsigned 8-bit IQ (e.g. RTL-SDR's native `cu8` format, offset-binary): 
$$\text{sample}_{float} = \frac{\text{sample}_{uint8} - 127.5}{127.5}$$
**Manchester/PPM bit slicing** (ADS-B): within each 1 µs data bit, compare the power in the first vs. second 0.5 µs half — whichever half has the pulse determines the bit.

### 4. Formal definition

**SigMF** (Signal Metadata Format) is an open, widely-adopted convention pairing a raw binary IQ file (`.sigmf-data`) with a companion JSON metadata file (`.sigmf-meta`) specifying sample rate, center frequency, datatype, and capture annotations — solving exactly the "what do these bytes mean" problem from §2. Common raw datatypes you'll encounter: `cu8` (interleaved unsigned 8-bit — RTL-SDR native), `cs16` (interleaved signed 16-bit — common for USRP/HackRF), `cf32` (interleaved 32-bit float — common in processed or simulation-exported files).

### 5. Derivation — ADS-B preamble detection is Lesson 6's matched filter, unchanged

ADS-B (aircraft transponder) transmissions begin with an **8 µs preamble** — a fixed, known pulse pattern (pulses roughly at 0, 1, 3.5, and 4.5 µs, each 0.5 µs wide) that **never varies**, transmitted before every single message. A perfectly known, fixed signal buried in noise is *exactly* the scenario Lesson 6 §5 proved the matched filter is optimal for — no new theory is needed here, only recognizing the pattern. Build a template matching the expected preamble's power envelope at your capture's sample rate, correlate the received signal's magnitude against it (Lesson 6 §3), and the correlation peak marks the exact start of a message with the same Cauchy–Schwarz-guaranteed optimality already proven once. After the preamble, each 1 µs data bit uses **pulse-position encoding**: a pulse in the first half-microsecond means bit 1, a pulse in the second half means bit 0 — decoded with a simple "which half has more power" slicer, structurally the same nearest-decision logic as Lesson 7's "which side of zero," just operating on power instead of phase.

### 6. Visualization

```
Raw file layout (interleaved cu8):        SigMF-style metadata (.sigmf-meta):
[I0][Q0][I1][Q1][I2][Q2]...                {
 each byte: 0-255, center = 127.5             "sample_rate": 2000000,
                                               "center_frequency": 1090000000,
                                               "datatype": "cu8"
                                            }

ADS-B preamble (8 µs, fixed pattern):
pulse: ▄ ▄       ▄ ▄
time:  0 1  2 3  3.5 4.5      ...then 1 µs/bit PPM data begins at 8 µs
       (μs)
```

### 7. Worked example

At a 2 Msps capture rate, 0.5 µs corresponds to exactly 1 sample — so the preamble's known pulse positions map to specific, fixed integer sample offsets from the start of a message. This is what makes a correlation-based detector so reliable here: the template isn't an approximation, it's an exact, small, fixed integer-sample pattern, known in advance for any capture at this standard rate.

### 8. Code

```python
import numpy as np

def load_iq(path, dtype='cu8'):
    raw = np.fromfile(path, dtype=np.uint8 if dtype=='cu8' else np.int16)
    if dtype == 'cu8':
        iq = (raw.astype(np.float32) - 127.5) / 127.5      # §3's conversion
    elif dtype == 'cs16':
        iq = raw.astype(np.float32) / 32768.0
    else:
        raise ValueError("unsupported dtype")
    return iq[0::2] + 1j*iq[1::2]                            # de-interleave I,Q

def sanity_check(x):
    return dict(mean_mag=np.mean(np.abs(x)), max_mag=np.max(np.abs(x)),
                looks_plausible=np.max(np.abs(x)) < 5)        # crude but effective, see §12

# --- ADS-B preamble detector, Lesson 6's matched filter, pointed at a new target ---
def adsb_preamble_template(fs):
    n_samples = int(8e-6 * fs)
    t = np.arange(n_samples) / fs
    template = np.zeros(n_samples)
    for pulse_start in [0.0, 1.0, 3.5, 4.5]:                  # µs, per §5
        mask = (t >= pulse_start*1e-6) & (t < (pulse_start+0.5)*1e-6)
        template[mask] = 1.0
    return template

def find_preambles(mag, template):
    corr = np.correlate(mag, template, mode='valid')          # exactly Lesson 6 §8
    threshold = 0.7 * np.max(corr)
    return np.where(corr > threshold)[0]
```

### 9. Experiment

Point `load_iq` at a real, publicly available ADS-B or general SDR sample capture (public datasets are indexed at places like IQEngine's dataset browser, and GNU Radio's own repository ships example files) captured around 1090 MHz at 2 Msps, run `sanity_check` first, then `find_preambles` on the signal's magnitude. You should see distinct correlation peaks at message boundaries throughout the file — real evidence of Lesson 6's optimality claim working on a signal you didn't generate yourself.

### 10. Expected output

`sanity_check` should show a bounded magnitude (roughly 0 to a few, not wildly large or NaN-laden) if the format guess was right. `find_preambles` should return a moderate number of detected positions, each corresponding to a real ADS-B transmission's start, spaced irregularly through the file exactly as real, asynchronous aircraft transmissions would be.

### 11. Debugging exercise

Symptom, no cause given: you load a raw IQ file someone else recorded, and even a basic FFT of it looks nonsensical — far too wide, or full of what looks like effectively random garbage, nothing resembling a believable RF spectrum.

What's the very first thing to suspect, before assuming your DSP code has a bug? (Hint: you now have three data-type options in §8's loader — what happens if you pick the wrong one?)

**Root cause (revealed):** almost always a **wrong sample-format assumption** — for instance, reading a file that's actually `cu8` (unsigned 8-bit) as if it were `cf32` (32-bit float). Reinterpreting raw bytes with the wrong width and encoding doesn't throw an error; it silently produces numerically "valid" but physically meaningless values, since four `cu8` bytes get reinterpreted as one nonsensical IEEE-754 float. This is the single most common first bug when working with an unfamiliar recorded file, and it's exactly why §16's sanity-check-across-formats habit exists — verify the format empirically before trusting anything downstream of it.

### 12. Common mistakes

- Forgetting the DC-centering conversion for unsigned formats (`-127.5`, §3) — skipping it introduces an artificial, format-induced DC offset that has nothing to do with real hardware LO leakage (Lesson 15), easily and wrongly attributed to the hardware when it's actually a loading bug.
- Assuming I,Q interleaving order without checking — some tools store Q,I,Q,I instead; getting this backward silently mirrors your spectrum left-to-right around DC, with no error thrown, only a confusingly "flipped" result.
- Trusting a file's claimed metadata (sample rate, center frequency) without a sanity check — if the claimed sample rate doesn't match file size versus expected duration, or the claimed center frequency doesn't line up with a recognizable known signal, the metadata (or your reading of it) is wrong, and it's worth catching before spending hours debugging code that was never the problem.

### 13. Real RF/cellular application

Correctly reading and interpreting captured IQ data is a daily, foundational skill for RF test and validation engineers, and it's exactly what protocol/PHY conformance tools from Keysight, Rohde & Schwarz, and Anritsu do internally when analyzing a captured waveform against a specification. The matched-filter preamble-detection technique in this lesson is literally the same technique behind LTE/NR PSS/SSS and PRACH detection (Lessons 6 and 13) — ADS-B is simply a smaller, faster, and more approachable protocol to practice the identical technique on, with an immediately checkable real-world result if you carry the decode further (a real aircraft's position and altitude).

### 14. Interview question

*"You're handed an unfamiliar raw IQ file with no documentation. What's your systematic approach to figuring out how to correctly interpret it?"* Walk through a real methodology: check file size against any claimed sample rate/duration for consistency; try the most common formats (`cu8`, `cs16`, `cf32`) in turn and sanity-check the resulting values and spectrum for physical plausibility before trusting further processing; treat any accompanying metadata as a hypothesis to verify, not a fact to assume.

### 15. Active recall

1. Why does an unsigned 8-bit IQ format need the `-127.5` conversion before use, while a signed format doesn't?
2. Why is ADS-B preamble detection not a new algorithm, but a direct reapplication of Lesson 6?
3. What's a fast, concrete way to sanity-check whether you've guessed a file's sample format correctly?

### 16. Mini assignment

Extend §8's `load_iq` into a small loader that tries `cu8`, `cs16`, and `cf32` in turn on the same raw file, runs `sanity_check` on each interpretation, and reports which one looks physically plausible (bounded, reasonable magnitude) versus which produce obvious garbage — turning §11's debugging exercise into an automated first-line check you can reuse on any unfamiliar file going forward.

### 17. Exit test

1. What three pieces of information does SigMF's metadata file solve the absence of?
2. True or false, with a one-line reason: "If a raw IQ file's metadata says the sample rate is 2 Msps, you can trust that without verification."
3. Your reasoning from §11 about the wrong-format symptom, if you haven't sent it.

---

**Next:** Lesson 18 shifts into **Stage 6 — Cellular Fundamentals**: how cellular systems are actually organized (cells, duplexing, the 3GPP standards process), setting up the direct jump into LTE's air interface starting in Lesson 19.

---

[← Lesson 16](lesson-16-gnu-radio-fm-demod.md) · [Table of Contents](../README.md) · [Lesson 18 →](lesson-18-cellular-fundamentals.md)