# P08 — Digital Foundations: Bits, Two's Complement, Floats, Endianness, XOR/GF(2), CRC and LFSRs

*Prerequisite module 8 · ⏱ about 5–6 hours · assumes basic programming; P01–P07 helpful but not required · every number and output below was computed and checked.*

> **Where this unlocks the curriculum.** A keyword scan of the lessons ties this module to: **Lesson 17** (recorded IQ files — `uint8`/`int16`/`float32`, interleaving, offset-binary `cu8`), **Lesson 25** (fixed-point Q-format, overflow/saturation, the 6.02B+1.76 dB rule), **Lesson 32** (M-sequences from LFSRs over GF(2)), **Lessons 41, 44, 45** (convolutional, polar and LDPC codes — all XOR / modulo-2 arithmetic) and the CRC mentions in Lessons 20 and 44. Radio code is full of "what exactly do these bytes mean?" — this module gives you the precise answers.

---

## 8.1 Bits, bytes, hex — and bit operations 🔥

* **Bit:** 0 or 1. **Byte:** 8 bits (values 0–255). **Word sizes:** 16, 32, 64 bits.
* **Binary place value:** $1011_2=1\cdot8+0\cdot4+1\cdot2+1\cdot1=11$.
* **Hexadecimal** (base 16, digits 0–9 A–F): one hex digit = 4 bits, so one byte = two hex digits. *Verified:* $255=\texttt{0xFF}=\texttt{0b11111111}$; $1000=\texttt{0x3E8}$; $\texttt{0xAB}$ → high nibble `0xA`, low nibble `0xB`.

**Bit operations (Python / C / C++ share them):**

| Op | Symbol | Meaning | Check (`a=1100`, `b=1010`) |
|---|---|---|---|
| AND | `&` | 1 only if both 1 | `1000` |
| OR | `\|` | 1 if either is 1 | `1110` |
| **XOR** | `^` | 1 if the bits **differ** | `0110` |
| shift left | `<<` | multiply by 2ⁿ | `1100<<1 = 11000` |
| shift right | `>>` | divide by 2ⁿ (floor) | `1100>>2 = 11` |

**Masking** extracts fields: `(0xAB >> 4) & 0x0F = 0xA`. ⚠️ In Python `^` is **XOR, not exponent** (`**` is power).

---

## 8.2 Integers in a fixed number of bits — two's complement 🔥

Hardware (and NumPy dtypes, C/C++ `int16_t`, DSP code) stores integers in a **fixed width**. Two questions: how are negatives stored, and what happens at the limits?

**Unsigned** $B$-bit integers cover $0\ldots2^{B}-1$ (8 bits: 0–255).

**Two's complement** (the universal signed format): the top bit has weight $-2^{B-1}$.

$$\text{value}=-b_{B-1}2^{B-1}+\sum_{i=0}^{B-2}b_i2^{i}\quad\Rightarrow\quad\text{range }[-2^{B-1},\,2^{B-1}-1]$$

*Verified (8-bit):* `11111111` = **−1**, `10000000` = **−128**, `01111111` = **+127**, `11111110` = **−2**. Ranges (from NumPy): `int8` −128…127, `uint8` 0…255, **`int16` −32768…32767**.

**Negate = invert all bits, then add 1.** *Verified:* 5 → `~5+1` masked to 8 bits = `11111011` = 251 unsigned = **−5** signed. **Sign extension** (widening a negative number) copies the top bit: 12-bit `0xFF8` → **−8**.

### Overflow: wrap-around vs saturation 🔥 (preview of Lesson 25)
Arithmetic that exceeds the range doesn't raise an error in fixed-width types:

* **Wrap-around:** *verified* `int16` 32767 + 1 → **−32768** (a full-scale positive sample becomes full-scale *negative*!); `uint8` 200 + 100 → **44**. Python's own `int` is unbounded (32767 + 1 = 32768) — which is why a Python prototype can hide a bug that appears in C++ or on an FPGA.
* **Saturation (clipping):** clamp to the limits instead — *verified* `clip(32768) → 32767`. Saturating costs a little distortion but avoids catastrophic sign flips; DSP code usually saturates deliberately.

---

## 8.3 Radio sample formats: offset binary, interleaving, data rates 🔥 (Lesson 17)

An IQ capture file is just bytes; **you must know the format.** Common names: **`cu8`** (complex unsigned 8-bit; RTL-SDR native), **`ci16`** (complex int16), **`cf32`** (complex float32 = NumPy `complex64`).

* **Interleaved layout:** I₀, Q₀, I₁, Q₁, … (not two separate blocks).
* **Offset binary (`cu8`):** unsigned bytes with the centre at 127.5, converted by (Lesson 17's formula)

$$x_{\text{float}}=\frac{x_{\text{uint8}}-127.5}{127.5}$$

*Verified:* byte 0 → **−1.0**, 255 → **+1.0**, 127 → −0.0039, 128 → +0.0039. Bytes `[0,255, 127,128, 255,0]` → complex samples **−1+1j, −0.0039+0.0039j, 1−1j**.

```python
raw = np.fromfile("capture.cu8", dtype=np.uint8)
f   = (raw.astype(np.float32) - 127.5) / 127.5          # NOTE: cast BEFORE subtracting!
z   = f[0::2] + 1j*f[1::2]                               # I,Q,I,Q,... -> complex
```
(⚠️ subtracting 127.5 from `uint8` without converting would wrap around — §8.2.)

**Data rates** (bytes per sample × sample rate): *verified*
| Capture | Rate |
|---|---|
| 2 Msps `cu8` (ADS-B, Lesson 17) | 4 MB/s → **240 MB per minute** |
| 20 Msps `ci16` | 80 MB/s |
| **30.72 Msps `cf32`** (LTE 20 MHz, Lesson 2) | **245.76 MB/s → 14.7 GB per minute** |

*Meaning:* wideband captures fill disks fast — choose the smallest adequate format and read in chunks.

---

## 8.4 Fixed-point fractions: the Q-format idea (preview of Lesson 25)

To store fractions in integers, pretend a binary point sits inside the word. In **Q15** (16-bit) the stored integer $I$ means $I/2^{15}$.

* Range $[-1,\,1-2^{-15}]=[-1,\ 0.99997]$; resolution $2^{-15}=3.05\times10^{-5}$.
* *Verified:* 0.5 → **16384**; −1.0 → **−32768**; the value **+1.0 cannot be represented** (max is 32767/32768 = 0.99997) and saturates to 32767.
* **Multiplying** two Q15 numbers gives a Q30 product that needs a **right shift by 15** to return to Q15: $0.5\times0.5$: raw product 268,435,456 → `>>15` = **8192** = 0.25 ✓.

Lesson 25 builds the whole story (bit growth, rounding, SQNR = 6.02B+1.76 dB, P07 §7.8) on this foundation.

---

## 8.5 Floating point (IEEE-754): reals with limited precision 🔥

A float stores **sign × significand × 2^exponent**. Two sizes matter:

| Type | Bytes | Significand | Machine epsilon | Typical decimal digits |
|---|---|---|---|---|
| `float32` (`complex64` = 2 × float32) | 4 (8) | 24 bits | **1.19×10⁻⁷** | ≈ 7 |
| `float64` (`complex128` = 2 × float64) | 8 (16) | 53 bits | **2.22×10⁻¹⁶** | ≈ 15–16 |

**Consequences (all verified):**
* Most decimals are **not exact**: `0.1 + 0.2 == 0.3` is **False** (the sum is 0.30000000000000004). Compare with a tolerance: `np.isclose`.
* **Large + small loses the small one:** in `float32`, 16,777,216 + 1 = 16,777,216 (2²⁴ is the largest integer where every integer is representable); +2 works.
* **Accumulation error grows.** Summing 0.1 ten million times: `float64` sequential gives 1,000,000.015; **`float32` sequential gives 1,087,937 — about 9 % too high**. (NumPy's `np.sum` uses *pairwise* summation and returned 1,000,000.1 — it hides the problem. A streaming accumulator in C++ or a long running average in `float32` does *not* get that help.) **Rule:** accumulate in higher precision, or use compensated (Kahan) / pairwise sums.
* Floats trade absolute accuracy for **dynamic range** — good for DSP prototyping; fixed point is cheaper/faster in hardware (Lesson 25).

---

## 8.6 Endianness and reading binary data 🔥

A multi-byte value can be stored with its **least significant byte first (little-endian — x86, ARM)** or **most significant first (big-endian — network byte order)**.

*Verified:* `0x1234` is stored `34 12` little-endian and `12 34` big-endian. Reading the bytes `34 12` as `<i2` (little-endian int16) gives **4660 (0x1234)**; as `>i2` (big-endian) gives **13330 (0x3412)** — a silent, completely wrong number if you pick the wrong order.

*Verified file round-trip:* three complex samples written as interleaved little-endian `int16` (`1000−2000j`, `−3000+4000j`, `32767+0j`) occupied exactly **12 bytes** and read back identically.

```python
iq = np.empty(2*len(z), dtype='<i2')              # '<' = little-endian int16
iq[0::2], iq[1::2] = z.real.astype('<i2'), z.imag.astype('<i2')
iq.tofile("out.ci16")
r  = np.fromfile("out.ci16", dtype='<i2'); z2 = r[0::2] + 1j*r[1::2]
```
Specify the byte order **explicitly** in file formats and network code (P11 returns to "network byte order").

---

## 8.7 XOR, modulo-2 arithmetic and GF(2) 🔥 (the language of error-correcting codes)

**XOR truth table:** 0⊕0 = 0, 0⊕1 = 1, 1⊕0 = 1, **1⊕1 = 0**.
**Laws:** $a\oplus a=0$; $a\oplus0=a$; commutative and associative; hence $(a\oplus b)\oplus b=a$ (*verified:* $9\oplus6\oplus6=9$) — **XOR twice cancels**, the basis of scrambling, parity and cancellation.

**GF(2)** (the "Galois field with two elements") is just {0,1} with **addition = XOR** and **multiplication = AND**. Linear algebra (P04) works over it: you add/multiply as usual and reduce **mod 2**. Lessons 41, 44 and 45 are all linear algebra over GF(2).

### Parity and the parity-check matrix (Lesson 45 preview)
* **Even parity** of `1011` (three 1s) = 1 — append a bit so the total number of 1s is even.
* A **linear code** is defined by a **parity-check matrix $\mathbf H$**: a word $\mathbf c$ is valid iff $\mathbf H\mathbf c^{T}=\mathbf0\pmod2$. The result $\mathbf s=\mathbf H\mathbf r^{T}\bmod2$ for a received word is its **syndrome**.
 *Verified:* $\mathbf H=\begin{bmatrix}1&1&0\\0&1&1\end{bmatrix}$: $\mathbf c=[1,1,1]$ gives syndrome $[0,0]$ (valid); $\mathbf r=[1,0,1]$ gives $[1,1]$ (**error detected**).
* **Hamming (7,4) code:** let column $i$ of $\mathbf H$ be the number $i$ in binary. *Verified:* it has exactly **16 codewords**, and flipping the bit at position 5 of a codeword gave syndrome $[1,0,1]=\mathbf5$ — **the syndrome names the error position**, so the single error can be corrected. (LDPC codes in Lesson 45 are the same idea with a huge, sparse $\mathbf H$.)

### Convolutional encoder as XOR logic (Lesson 41)
Lesson 41's simple rate-½, $K=3$ encoder keeps the two most recent input bits $(s_1,s_2)$ and outputs $o_1=x\oplus s_1\oplus s_2$, $o_2=x\oplus s_2$ (generators $[1,1,1]$ and $[1,0,1]$). *Verified for input $1,0,1,1$ (starting at state 00):* output pairs **11, 10, 00, 01**, ending in state (1,1). Two output bits per input bit ⇒ rate ½; 2 memory bits ⇒ $2^{K-1}=4$ states.

---

## 8.8 Polynomials over GF(2) and the CRC

A bit string is a polynomial with 0/1 coefficients: `1011` ↔ $x^{3}+x+1$. Addition is XOR; division is **long division using XOR**. A **CRC (cyclic redundancy check)** appends the remainder of dividing the data (shifted by the generator's degree) by a **generator polynomial**; the receiver divides the whole word and expects remainder 0.

*Verified:* generator $1011$ ($x^{3}+x+1$), data `1101`: remainder **001**, transmitted word `1101001`, receiver remainder **000** ✓; flipping one bit → remainder **110** (non-zero ⇒ error detected). `[FOUNDATION ADDED — the lessons mention CRC attachment but do not derive it]`

*Link to your college notes:* GSM adds **3 CRC bits** to the 50 most important speech bits before convolutional coding (Unit II, Chapter 9) — exactly this tool.

---

## 8.9 LFSRs and M-sequences 🔥 (Lesson 32)

A **linear-feedback shift register (LFSR)** is $n$ bits that shift every clock; the new bit entering is the **XOR of selected "tap" bits**. With well-chosen taps (a *primitive polynomial*) the output repeats only after $N=2^{n}-1$ steps — a **maximum-length sequence (M-sequence)**. Lesson 32's recipe: generate it, then map **0 → +1, 1 → −1** (BPSK).

*Verified by simulation (feedback taps 3,2 for $n=3$; 7,6 for $n=7$):*

| $n$ | Period $N=2^{n}-1$ | Ones / zeros | Periodic autocorrelation |
|---|---|---|---|
| 3 | 7 | 4 / 3 | $R(0)=7$, $R(k\ne0)=\mathbf{-1}$ |
| **7** | **127** | **64 / 63** | $R(0)=127$, $R(k\ne0)=\mathbf{-1}$ |

For $N=127$ the correlation-peak-to-sidelobe ratio is $20\log_{10}127=\mathbf{42.08}$ dB (Lesson 32 quotes ≈ 42.1 dB) — a sharp peak that makes sequences easy to find by correlation (P04 dot product), and a property of **balance** (one more 1 than 0).

* **All-zero state is a trap:** XOR of zeros stays zero, so the register never leaves it — always seed with a non-zero state.
* *Link to your college notes:* **PN codes** in IS-95 CDMA (Unit II, Chapter 12) — the long PN code ($2^{42}-1$) and short PN codes (period $2^{15}$) — are generated by LFSRs like this, and spreading/despreading (Unit I, Chapter 5) correlates against them.

---

## 8.10 Python workout (verified)

```python
import numpy as np, struct

# two's complement and overflow
print(np.array([32767], dtype=np.int16) + 1)          # [-32768]  wrap-around
print(np.clip(np.array([32767]) + 1, -32768, 32767))  # [32767]   saturation

# offset-binary cu8 -> float
raw = np.array([0, 255, 127, 128, 255, 0], dtype=np.uint8)
f = (raw.astype(np.float32) - 127.5) / 127.5
print(f[0::2] + 1j*f[1::2])

# endianness
print(struct.pack('<H', 0x1234).hex(), struct.pack('>H', 0x1234).hex())   # 3412 1234
print(np.frombuffer(bytes([0x34, 0x12]), dtype='<i2')[0])                  # 4660

# float32 accumulation error (sequential) vs float64
x = np.full(10**7, 0.1, dtype=np.float32)
print(np.cumsum(x)[-1], np.cumsum(x.astype(np.float64))[-1])   # ~1.0879e6  1.0000000149e6

# convolutional encoder, K=3, generators [1,1,1] and [1,0,1]
s1 = s2 = 0; out = []
for b in [1, 0, 1, 1]:
    out.append((b ^ s1 ^ s2, b ^ s2)); s2, s1 = s1, b
print(out)                                                       # [(1,1),(1,0),(0,0),(0,1)]

# LFSR m-sequence (taps 7,6) and autocorrelation
def lfsr(taps, n):
    st, out = [1]*n, []
    for _ in range(2**n - 1):
        out.append(st[-1]); fb = 0
        for t in taps: fb ^= st[t-1]
        st = [fb] + st[:-1]
    return np.array(out)
s = lfsr((7, 6), 7); a = 1 - 2*s                                  # 0->+1, 1->-1
print(len(s), s.sum(), {int(np.sum(a*np.roll(a, k))) for k in range(1, 127)})   # 127 64 {-1}
```

---

## 8.11 Common mistakes ⚠️
1. **Wrong signedness/width** (`uint8` vs `int8`, `int16` vs `uint16`) — the file has no header to tell you.
2. **Forgetting offset-binary:** reading `cu8` as plain unsigned gives a huge DC offset (everything looks ≈ +1).
3. **Subtracting before casting** a `uint8` array — wraps around (§8.2).
4. **Mixing endianness**, or assuming "the machine's native order" for a file written elsewhere.
5. **Comparing floats with `==`** (use tolerances) and **summing long streams in `float32`**.
6. **Silent integer overflow in NumPy arrays** (no exception) — check ranges or use wider/ saturating arithmetic.
7. **Arithmetic vs logical right shift** of negative numbers: `-8 >> 1` is −4 in Python/C++ (arithmetic) — don't expect zeros shifting in on the left.
8. **Using `+` where GF(2) needs `^`** (or forgetting `% 2` after a matrix product).
9. **Seeding an LFSR with all zeros.**
10. **Mapping bits to ±1 inconsistently** — Lesson 32 uses 0 → +1, 1 → −1; the opposite convention flips signs everywhere.
11. **Interpreting an interleaved file as two separate blocks** (I-block then Q-block) — or vice versa.

---

## 8.12 Practice problems (answers at the end)

1. Convert 0b10110110 to decimal (unsigned) and to hexadecimal. What is it as an 8-bit two's-complement number?
2. Write −20 as an 8-bit two's-complement pattern.
3. In `int16`, what is 30000 + 5000 (wrap-around)? What would saturation return?
4. RTL-SDR bytes `[64, 191]` are I and Q (`cu8`). Give the complex sample as floats.
5. How many MB does a 5-minute capture at 10 Msps `ci16` occupy?
6. What are the bytes of `0x0A0B0C0D` in little-endian order?
7. Give Q15 integers for 0.25, −0.5 and 0.999.
8. A 16-bit signed Q15 value is stored as `0xC000`. What real number is it?
9. Compute the K=3 encoder output (generators [1,1,1] and [1,0,1]) for input `1,1,0` starting from state 00.
10. Using $\mathbf H=\begin{bmatrix}1&1&0\\0&1&1\end{bmatrix}$, compute the syndrome of $\mathbf r=[0,1,1]$.
11. Divide `1011000` by `1011` (mod 2) and give the remainder — i.e. the CRC-3 of data `1011`.
12. A 4-bit LFSR with a primitive polynomial: what is its period? What happens if you start it at 0000?
13. *Think:* a colleague's float32 running-average code drifts after hours of streaming. Give two fixes.

## 8.13 Active recall
1. Explain two's complement and the range of $B$-bit signed integers. 2. What happens at overflow, and what is saturation? 3. Write Lesson 17's `cu8` → float formula and explain why a cast is needed first. 4. Explain Q15 and why +1.0 isn't representable. 5. What are float32 and float64 precision limits? 6. Why can float accumulation drift? 7. Define little- vs big-endian with an example. 8. State the XOR laws and why XOR twice cancels. 9. What is GF(2)? 10. What is a syndrome? 11. How does a CRC detect errors? 12. How is an M-sequence generated and what is special about its autocorrelation?

## 8.14 Self-check — ready for P09 if you can…
- [ ] convert between binary/hex/decimal and read two's-complement values;
- [ ] predict overflow and choose wrap vs saturation;
- [ ] decode `cu8`/`ci16`/`cf32` files and compute their data rates;
- [ ] explain float32 precision issues;
- [ ] do XOR/GF(2) arithmetic: parity, syndromes, a small convolutional encoder, a CRC, an LFSR.

---

## ✅ Answer key

1. 0b10110110 = 128+32+16+4+2 = **182 = 0xB6**; as two's complement: 182 − 256 = **−74**. 2. 20 = `00010100`; invert → `11101011`; +1 → **`11101100`**. 3. 35000 − 65536 = **−30536** (wrap-around); saturation returns **32767**. 4. $(64-127.5)/127.5=\mathbf{-0.498}$, $(191-127.5)/127.5=\mathbf{+0.498}$ ⇒ **−0.498 + 0.498j**. 5. 10×10⁶ samples/s × 4 bytes × 300 s = **12 GB** (12,000 MB). 6. **0D 0C 0B 0A**. 7. 0.25 → **8192**; −0.5 → **−16384**; 0.999 → 0.999 × 32768 = 32735.2 ⇒ **32735**. 8. `0xC000` as int16 = 49152 − 65536 = −16384 ⇒ −16384/32768 = **−0.5**. 9. Start (s₁,s₂) = (0,0). x=1: o₁=1⊕0⊕0=1, o₂=1⊕0=1 → (1,1); state (1,0). x=1: o₁=1⊕1⊕0=0, o₂=1⊕0=1 → (0,1); state (1,1). x=0: o₁=0⊕1⊕1=0, o₂=0⊕1=1 → (0,1). Output pairs **11, 01, 01**. 10. H·[0,1,1]ᵀ = [0⊕1⊕0, 1⊕1] = **[1, 0]** (non-zero ⇒ error detected). 11. 1011000 ⊕ 1011000 = 0000000 ⇒ remainder **000** (data `1011` is itself divisible by the generator). 12. Period **2⁴ − 1 = 15**; starting at 0000 it **stays at 0000 forever** (lock-up). 13. Accumulate in `float64` (or use pairwise/Kahan summation), and/or use an exponential moving average with a bounded window so the accumulator doesn't grow without limit.

---

*Next: **P09 — Python / NumPy / Matplotlib workflow** (arrays, dtypes, complex data, vectorising, `np.fft`, binary IQ files, plotting spectra and constellations, reproducible experiments) — the toolkit for every lesson's Code section.*
