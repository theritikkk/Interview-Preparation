# Z — Prerequisites Cheat Sheet (P01–P12 on one page)

*Keep this open while working through the curriculum. Every entry is copied from a verified module; the module/section is shown so you can go back for the reasoning.*

---

## 1. Decibels and levels (P01)
| Item | Value |
|---|---|
| Power ratio ↔ dB | $10\log_{10}(P_2/P_1)$ · amplitude: $20\log_{10}(V_2/V_1)$ (§1.4) |
| **3-6-10 rule** | +3 dB ≈ ×2 · +6 dB ≈ ×4 · +10 dB = ×10 · −3 dB ≈ ×½ (§1.4) |
| Absolute levels | 0 dBm = 1 mW · 30 dBm = 1 W · **23 dBm ≈ 199.5 mW** · −90 dBm = 1 pW (§1.4) |
| Rules | dBm + dB → dBm · dBm − dBm → dB · **never add dBm; convert to mW** · average in linear, not dB (§1.4) |
| Noise floor | $N=-174+10\log_{10}B$ dBm (ideal, 290 K) · 20 MHz → −101 dBm · +NF (§1.5) |
| Wavelength | $\lambda=c/f$ · 900 MHz = 33.3 cm · 2.4 GHz = 12.5 cm · 3.5 GHz = 8.57 cm (§1.2) |

## 2. Sinusoids and sampling (P02)
* $x=A\cos(2\pi ft+\varphi)$, $\omega=2\pi f$, $T=1/f$; average power $A^{2}/2$ (§2.4).
* **Delay → phase:** $\Delta\varphi=-2\pi f\tau$ (500 Hz delayed 0.5 ms = 90°) (§2.4).
* **Mixing:** $\cos a\cos b=\tfrac12[\cos(a-b)+\cos(a+b)]$ → sum and difference frequencies (§2.5).
* $\cos$ is even ⇒ a real signal can't tell $+f$ from $-f$ (§2.6).
* **I/Q:** $A\cos(\omega t+\varphi)=I\cos\omega t-Q\sin\omega t$, $I=A\cos\varphi$, $Q=A\sin\varphi$; $A=\sqrt{I^2+Q^2}$, $\varphi=\operatorname{atan2}(Q,I)$ (§2.8).
* **Discrete frequency:** $\hat\omega=2\pi f/f_s$ rad/sample; 3 kHz at $f_s$ = 4 kHz aliases to **1 kHz** (§2.9).

## 3. Series and calculus (P03)
* Geometric: $\sum_{n=0}^{N-1}r^{n}=\dfrac{1-r^{N}}{1-r}$; $N$ equally spaced unit arrows sum to **0** (DFT orthogonality) (§3.3).
* **Instantaneous frequency** $=\frac1{2\pi}\,d\varphi/dt$ (chirp $2\pi(100t+50t^{2})\to100+100t$ Hz) (§3.5).
* Sinusoids of different integer frequencies are **orthogonal over whole periods**; leakage otherwise (§3.7).
* Taylor: $e^{x}=\sum x^{k}/k!$ · NumPy 2: use **`np.trapezoid`** (not `trapz`) (§3.9).

## 4. Linear algebra (P04)
* $\|\mathbf x\|^{2}=\sum|x_n|^{2}$ = energy · **complex inner product $\mathbf a^{H}\mathbf b$** → `np.vdot` (**`np.dot` does not conjugate!**) (§4.3).
* `*` is element-wise, **`@` is matrix product** · $(m\times n)(n\times p)=m\times p$ · MIMO: $\mathbf y=\mathbf H\mathbf x+\mathbf n$, $\mathbf H$ is $N_r\times N_t$ (§4.5).
* **Zero-forcing** $\hat{\mathbf x}=\mathbf H^{+}\mathbf y$; ill-conditioned $\mathbf H$ amplifies noise (cond 4002 → ~1415×) (§4.6).
* **Rank = number of independent streams** · cond $=\sigma_{\max}/\sigma_{\min}$ · SVD $\mathbf H=\mathbf U\boldsymbol\Sigma\mathbf V^{H}$ (§4.7, 4.9).
* Least squares $\hat{\mathbf x}=\mathbf A^{+}\mathbf b$; pilot channel estimate $\hat h=\langle\mathbf p,\mathbf y\rangle/\langle\mathbf p,\mathbf p\rangle$ (§4.10).

## 5. Probability and noise (P05)
* Independent variances add · Var$(aX)=a^{2}$Var$(X)$ · zero-mean variance = power (§5.4).
* $Q(x)=\tfrac12\operatorname{erfc}(x/\sqrt2)$: Q(1)=0.1587, Q(2)=0.02275, Q(3)=0.00135, Q(4)=3.17×10⁻⁵ (§5.6).
* **BPSK BER** $=Q\bigl(\sqrt{2E_b/N_0}\bigr)$: 0 dB → 0.0786 · 4 dB → 0.0125 · 10 dB → 3.9×10⁻⁶ (§5.6).
* **Complex noise: each real dimension has variance $N_0/2$**, total $N_0$ (missing "/2" = 3 dB error, 4.5× BER at 4 dB) (§5.7).
* Rayleigh: sum of many random phasors; power exponential; $P(\text{power}<0.1\,\text{mean})=0.0952$ (§5.8).
* **≥ 100 errors** before trusting a BER (relative uncertainty $\approx1/\sqrt{\text{errors}}$); SE $=\sqrt{\hat p(1-\hat p)/N}$, $z=(\hat p-p_0)/\mathrm{SE}$; 0 errors → rule of three $3/N$ (§5.9–5.10).

## 6. Signals and Fourier series (P06)
* LTI ⇒ sinusoid in → **same-frequency** sinusoid out; nonlinearity creates new frequencies ($\cos^{2}\to$ DC + $2f$) (§6.4).
* Series: $a_0=\frac1T\!\int x$, $a_k=\frac2T\!\int x\cos$, $b_k=\frac2T\!\int x\sin$. **Square wave:** $\frac4\pi\sum_{\text{odd }k}\sin(k\omega_0t)/k$ (§6.5).
* **Gibbs:** overshoot stays ≈ **1.179** (§6.5) · fundamental holds **81.06 %** of power; 99 % needs harmonic **41** (§6.6).
* **Pulse width $\tau$ → first null at $1/\tau$**, null-to-null $2/\tau$ (1 µs → 2 MHz) (§6.8).

## 7. Radio hardware (P07)
* 50 Ω: $V_{rms}=\sqrt{PR}$; **−90 dBm = 7.07 µV** (§7.2) · RC low-pass $f_c=1/(2\pi RC)$, −3 dB at $f_c$, −20 dB/decade (§7.3).
* **FSPL** $=20\log d_{km}+20\log f_{MHz}+32.44$ dB: 2 GHz 1 km = **98.46 dB**; ×2 distance or frequency = +6.02 dB (§7.6).
* **Sensitivity** $=-174+10\log B+\text{NF}+\text{SNR}_{\min}$ (20 MHz, NF 5, SNR 10 → **−86 dBm**) (§7.7) · Friis: $F=F_1+(F_2-1)/G_1+\cdots$ (§7.7).
* Zero-IF: $I=\text{LPF}\{2r\cos\omega_ct\}$, $Q=\text{LPF}\{-2r\sin\omega_ct\}$, $z=I+jQ=Ae^{j(2\pi\delta t+\varphi)}$ (§7.8).
* DC spike at 0 Hz = LO leakage · mirror ghost = IQ imbalance, IRR $=|\alpha|^{2}/|\beta|^{2}$ (ε=0.05, φ=2° → 30.5 dB) (§7.8).
* **ADC SNR $=6.02B+1.76$ dB** (16 bit ≈ 98 dB) · ppm: 1 ppm at 2 GHz = 2 kHz · sine PAPR = 3.01 dB, OFDM ≈ 8–10 dB (§7.8–7.10).

## 8. Digital foundations (P08)
* Two's complement range $[-2^{B-1},2^{B-1}-1]$; `11111111` = −1; **`int16` 32767+1 → −32768**; negate = invert + 1 (§8.2).
* **`cu8`:** $(\text{byte}-127.5)/127.5$ — cast *before* subtracting; interleaved I,Q,I,Q (§8.3). 30.72 Msps `cf32` = **245.76 MB/s** (§8.3).
* **Q15:** stored $=\text{value}\times2^{15}$; +1.0 not representable; product needs `>>15` (§8.4).
* float32 eps 1.19×10⁻⁷, float64 2.22×10⁻¹⁶ · **sequential float32 sums drift** (10 M × 0.1 → +8.8 %) (§8.5).
* **Endianness:** `0x1234` → `34 12` (little) / `12 34` (big, network order) (§8.6).
* XOR: $a\oplus a=0$; GF(2): + is XOR, × is AND; **parity check** $\mathbf H\mathbf r^{T}\bmod2$ = syndrome; Hamming (7,4) syndrome names the error position (§8.7).
* **LFSR** with primitive polynomial: period $2^{n}-1$, autocorrelation $R(0)=N$, $R(k\neq0)=-1$; never seed with zeros (§8.9).

## 9. Python / NumPy (P09)
* Slices/reshape are **views**; fancy indexing/`.copy()` copy (§9.2) · **`complex64 × np.float64` → `complex128`** (§9.3).
* FFT recipe: `fftshift(fft(x))` with `fftshift(fftfreq(N, 1/fs))`; Parseval $\sum|x|^2=\frac1N\sum|X|^2$; window with `np.hanning` (§9.6).
* Seed: `rng = np.random.default_rng(seed)`; unit complex noise $=(\text{randn}+j\,\text{randn})/\sqrt2$ (§9.7).
* Big files: `np.fromfile(..., count=, offset=)`, `np.memmap` (§9.8) · compare with `np.allclose`, never `==` (§9.10).

## 10. C++ (P10)
* `sizeof(std::complex<float>)` = 8, `<double>` = 16 · unsigned wraps, **signed overflow is undefined behaviour** (§10.3).
* `-1 < 1u` is **false** (`-Wall -Wextra` warns) · `std::norm` = $|z|^{2}$ (no sqrt) (§10.3, 10.6).
* `-fsanitize=address` (heap overflow, use-after-free) · `-fsanitize=undefined` · `-fsanitize=thread` · optimise with `-O2/-O3` (§10.2, 10.5).
* `push_back` can reallocate → pointers dangle; `reserve(n)` first (§10.6).
* **Accumulate in `double`** (float loop gave −2.8 % error) (§10.8).

## 11. Systems and protocols (P11)
* **Per-sample budget:** 1 ms / 30,720 samples = **32.6 ns**; subframe = 245,760 bytes (§11.1).
* Measured here: syscall ≈ **110 ns** · thread wake round trip ≈ **3–4 µs** · strided access **25–30×** slower per element · "1 ms" sleep sometimes ~1.4–2 ms (§11.2, 11.6, 11.7).
* **Data race:** plain shared counter lost up to half the updates; `volatile` ≠ atomic; fix with `std::atomic`/`std::mutex`; detect with TSan (§11.8).
* **Priority inversion** → priority inheritance or lock-free queue (§11.3).
* PDU = header + SDU; network byte order = big-endian (§11.10). Stop-and-wait utilisation $=T_{tx}/(T_{tx}+\text{RTT})$ (7.4 % at 10 ms RTT/10 Mbit/s); window ≈ $(T_{tx}+\text{RTT})/T_{tx}$ (§11.10).
* ARQ: mean transmissions $=1/(1-p)$; 10 % loss → median 5 ms but **p99 35 ms** (tail!) (§11.10).

## 12. College ↔ curriculum (P12)
* Lesson 18's SINR uses **one** nearest interferer; your notes use **six** ⇒ **7.78 dB** gap (N=7, n=4: 26.4 vs 18.7 dB) (§12.4).
* forward/reverse = **downlink/uplink** · MS = **UE** · BS = **gNB/eNodeB** · handoff = **handover** · SIR = **SINR** (§12.3).
* GPRS STANDBY ↔ **RRC_INACTIVE** · GSM FCCH/SCH/BCCH/RACH ↔ PSS/SSS/PBCH/PRACH (conceptual) (§12.5).
* The curriculum never mentions AMPS, EDGE, Mobile IP, HLR/VLR, paging, Walsh codes, RAKE (§12.1).

---

## 13. Lesson gates (suggested; from P13)
**Lesson 1:** P01–P03, P09 ≥ 3/5 + Labs 1, 3 · **2–5:** P06 · **7:** P05 + Lab 2 · **11–14, 46:** P04 · **15–17, 42:** P07, P08 + Lab 4 · **22:** P10 + Lab 5 · **24, 27:** P11 + Lab 6 · **20, 21, 33, 38, 39:** P11 §11.10 + Lab 7 · **41, 44, 45:** P08.

*Not covered by this sheet: derivations and worked examples — those are in the modules.*
