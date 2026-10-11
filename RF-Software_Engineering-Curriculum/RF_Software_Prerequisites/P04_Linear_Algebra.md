# P04 — Linear Algebra for Radio: Vectors, Dot Products, Matrices, Rank, SVD, Least Squares

*Prerequisite module 4 · ⏱ about 6–7 hours · assumes P01–P03 · every number and output below was computed and checked.*

> **Where this unlocks the curriculum.** Lesson 14 (MIMO) is *the* linear-algebra lesson: a channel **matrix H**, its **rank** and **condition number**, the **Hermitian transpose**, **inverse/pseudo-inverse** and **zero-forcing**. Lessons 11 and 12 use zero-forcing/MMSE equalisation and orthogonality; Lesson 26 (SIMD) is built on the **dot product**; Lesson 45 (LDPC) on **parity-check matrices**; Lessons 46 and 48 on **precoding / beamforming weights** and **projections**. You do not need a full linear-algebra course — you need the ten ideas below and fluent NumPy.

---

## 4.1 Why radio is full of matrices

* A receiver with several antennas sees a **vector** of samples, one per antenna.
* Several transmit antennas sending different symbols are a vector $\mathbf x$; the channel mixes them: **$\mathbf y=\mathbf H\mathbf x+\mathbf n$** — one matrix equation (Lesson 14).
* "Undoing the channel" is solving a linear system.
* Correlation, matched filtering and the DFT are **dot products** with known vectors.

Linear algebra is simply the language for *"many numbers at once, combined linearly."*

---

## 4.2 Vectors, length and unit vectors

A **vector** is an ordered list of numbers, $\mathbf x=[x_1,x_2,\dots,x_N]$. Two pictures:
* **Data:** $N$ samples of a signal ("a vector of 1024 IQ samples").
* **Geometry:** an arrow from the origin to the point $(x_1,\dots,x_N)$.

**Operations.** Add element-wise; multiply by a scalar to stretch. $\mathbf a+\mathbf b$, $c\mathbf a$.

**Length (norm).** For real vectors $\|\mathbf x\|=\sqrt{x_1^2+\cdots+x_N^2}$. For **complex** vectors use *magnitudes squared*:

$$\|\mathbf x\|=\sqrt{\sum_{n}|x_n|^{2}}\qquad\Longrightarrow\qquad\|\mathbf x\|^{2}=\sum_n|x_n|^{2}=\textbf{energy}\ (\text{P03})$$

*Verified:* $\|[3,4]\|=\mathbf5$, unit vector $[0.6,0.8]$; $\|[1+j,\,1-j]\|^{2}=\mathbf4=\sum|x_n|^{2}$ ✓.

---

## 4.3 The dot (inner) product 🔥

For real vectors

$$\mathbf a\cdot\mathbf b=\sum_n a_nb_n=\|\mathbf a\|\,\|\mathbf b\|\cos\theta$$

where $\theta$ is the angle between them. Three readings of the same number:
1. **Similarity:** large and positive when the vectors point the same way; zero when perpendicular; negative when opposed.
2. **Correlation:** multiply two signals sample by sample and add up — exactly the matched-filter operation (Lesson 6).
3. **Projection:** how much of $\mathbf a$ lies along $\mathbf b$.

**Orthogonal ⇔ dot product 0.** *Verified:* $[3,4]\cdot[4,-3]=12-12=\mathbf0$. The angle between $[1,0]$ and $[1,1]$ is $\mathbf{45^\circ}$. P03's "sinusoids of different frequency are orthogonal" is this statement for long vectors.

### The complex dot product needs a conjugate 🔥
For complex vectors define

$$\langle\mathbf a,\mathbf b\rangle=\sum_n a_n^{*}\,b_n=\mathbf a^{H}\mathbf b$$

($^{*}$ = complex conjugate, $^{H}$ = conjugate transpose.) The conjugate guarantees $\langle\mathbf a,\mathbf a\rangle=\|\mathbf a\|^{2}$ is a real, non-negative energy.

**Verified with $\mathbf a=[1+j,\,2]$, $\mathbf b=[1,\,j]$:**
* `np.vdot(a, b)` $=\mathbf{1+j}$ (this is $\mathbf a^{H}\mathbf b$ — it conjugates the **first** argument).
* `np.dot(a, b)` $=\mathbf{1+3j}$ (**no conjugate** — not an inner product!).
* $\langle\mathbf a,\mathbf a\rangle$: `np.vdot(a,a)` $=\mathbf{6}$ (true energy: $|1+j|^2+|2|^2=2+4$) but `np.dot(a,a)` $=\mathbf{4+2j}$ (meaningless as energy).

> **This is the #1 linear-algebra bug in IQ code.** For complex data, use `np.vdot`, or write `a.conj() @ b`, or `np.sum(np.abs(a)**2)` for energy.

### Link back to your college notes (CDMA)
Despreading in CDMA *is* a dot product. With Walsh codes $W_1=[1,1,1,1]$, $W_2=[1,-1,1,-1]$, $W_3=[1,1,-1,-1]$ and the composite received chips $\mathbf s=[1,3,-1,1]$ (the 3-user example in Chapter 5 of the college notes), $\mathbf s\cdot W_k/4$ gives **[+1, −1, +1]** — each user's bit — because the Walsh codes are mutually orthogonal ($WW^{T}/4=I$, verified).

---

## 4.4 Projection and orthogonal expansion 🔥

The part of $\mathbf x$ along $\mathbf b$:

$$\operatorname{proj}_{\mathbf b}\mathbf x=\frac{\langle\mathbf b,\mathbf x\rangle}{\langle\mathbf b,\mathbf b\rangle}\;\mathbf b$$

*Verified:* project $[3,4]$ onto $[1,0]$: coefficient **3**, projection $[3,0]$.

If $\{\mathbf b_1,\dots,\mathbf b_N\}$ are **mutually orthogonal**, any vector can be rebuilt from its projections:

$$\mathbf x=\sum_k\frac{\langle\mathbf b_k,\mathbf x\rangle}{\langle\mathbf b_k,\mathbf b_k\rangle}\mathbf b_k$$

*Meaning:* measure how much of each "ingredient" $\mathbf b_k$ is present with one dot product each, and the mixture is fully described. **The DFT (Lesson 3), Fourier series (P06) and OFDM demodulation (Lesson 12) are exactly this with $\mathbf b_k$ = complex exponentials.**

---

## 4.5 Matrices and matrix–vector products 🔥

A **matrix** is a rectangular array, $m$ rows × $n$ columns. Multiplying a matrix by a vector gives a new vector:

$$\mathbf y=\mathbf A\mathbf x\qquad y_i=\sum_j A_{ij}x_j\quad(\text{row } i\text{ of }\mathbf A\ \text{dotted with }\mathbf x)$$

Two views: *each output is a dot product of a row with $\mathbf x$*, or *the output is a weighted sum of the columns of $\mathbf A$*. A matrix is a **linear transformation** (rotate, stretch, mix, project).

**Shape rule:** $(m\times n)\cdot(n\times p)\to(m\times p)$. *Verified:* $\begin{bmatrix}1&2\\3&4\end{bmatrix}\!\begin{bmatrix}1\\1\end{bmatrix}=\begin{bmatrix}3\\7\end{bmatrix}$.

**MIMO shapes (Lesson 14):** $\mathbf y$ is $N_r\times1$, $\mathbf H$ is $N_r\times N_t$, $\mathbf x$ is $N_t\times1$. A $4\times2$ $\mathbf H$ means 4 receive and 2 transmit antennas.

**Matrix–matrix product is *not* commutative.** *Verified:* with $\mathbf H=\begin{bmatrix}1&2\\3&4\end{bmatrix}$ and $\mathbf G=\begin{bmatrix}0&1\\1&0\end{bmatrix}$: $\mathbf{HG}=\begin{bmatrix}2&1\\4&3\end{bmatrix}\ne\mathbf{GH}=\begin{bmatrix}3&4\\1&2\end{bmatrix}$. (Order of channel, precoder and equaliser *matters*.)

**Special matrices.** Identity $\mathbf I$ ($\mathbf I\mathbf x=\mathbf x$); diagonal (scales each component — what the SVD reduces MIMO to); **transpose** $\mathbf A^{T}$ (swap rows/columns); **Hermitian (conjugate) transpose** $\mathbf A^{H}=(\mathbf A^{*})^{T}$ — *verified:* $\begin{bmatrix}1+j&2\\3&4-2j\end{bmatrix}^{H}=\begin{bmatrix}1-j&3\\2&4+2j\end{bmatrix}$.

---

## 4.6 Inverse, solving linear systems, and the pseudo-inverse 🔥

To "undo" $\mathbf y=\mathbf A\mathbf x$ find $\mathbf x=\mathbf A^{-1}\mathbf y$, where $\mathbf A^{-1}\mathbf A=\mathbf I$. It exists only if $\mathbf A$ is square and **non-singular** (determinant $\neq0$, i.e. columns independent).

*Verified:* $\mathbf A=\begin{bmatrix}2&1\\1&3\end{bmatrix}$, $\det=5$, $\mathbf A^{-1}=\begin{bmatrix}0.6&-0.2\\-0.2&0.4\end{bmatrix}$; solving $\mathbf A\mathbf x=[3,5]^T$ gives $\mathbf x=[0.8,\ 1.4]^T$ (check: $2(0.8)+1.4=3$ ✓).

**Pseudo-inverse $\mathbf A^{+}$** generalises the inverse to non-square or noisy cases; for a full-column-rank tall matrix $\mathbf A^{+}=(\mathbf A^{H}\mathbf A)^{-1}\mathbf A^{H}$.

### Zero-forcing in one line 🔥
With a known channel, **zero-forcing (ZF)** estimates the transmitted symbols as $\hat{\mathbf x}=\mathbf H^{+}\mathbf y$ — it *forces* the interference between streams to zero. For a single scalar channel $h$ (one subcarrier, Lesson 11) $\mathbf H^{+}$ is just $1/h$. *Verified experiment:* a random $2\times2$ complex channel with condition number 5.99 and noise variance 0.01 recovered QPSK symbols with maximum error **0.158**.

### Why a "nearly singular" channel is dangerous (Lesson 14's root cause) 🔥
Take $\mathbf H=\begin{bmatrix}1&1\\1&1.001\end{bmatrix}$ — the two antennas' channels are almost identical. Its **condition number is 4002**, and
$$\mathbf H^{+}=\begin{bmatrix}1001&-1000\\-1000&1000\end{bmatrix}$$
so each output component has a noise gain of **≈ 1415×** (the row norms), versus **1×** for $\mathbf H=\mathbf I$ (verified). Zero-forcing divides by something nearly zero, which **amplifies noise enormously** — roughly 63 dB in power (20 log₁₀ 1415). *Meaning:* a well-conditioned channel allows spatial multiplexing; an ill-conditioned one does not. MMSE (Lesson 11/14) damps this by not trusting tiny values.

---

## 4.7 Linear independence and rank 🔥

Vectors are **linearly independent** if none can be built from the others. The **rank** of a matrix is the number of independent columns (equivalently rows) — the number of independent "directions" it can carry.

* $\mathbf H=\begin{bmatrix}1&2\\2&4\end{bmatrix}$: second row = 2 × first, $\det=0$, **rank 1**, singular values $[5,\ \approx0]$ (the second prints as ~$10^{-16}$, i.e. numerically zero; verified).
* $\mathbf H=\mathbf I_2$: **rank 2**, singular values $[1,1]$.

*Meaning for MIMO:* the **number of independent data streams a channel can support is its rank** (Lesson 14). A rank-1 channel — e.g. a pure line-of-sight with closely spaced antennas — can carry only one stream, however many antennas you add.

---

## 4.8 Eigenvalues and eigenvectors (the 30-second version)

An **eigenvector** $\mathbf v$ of a square matrix is a direction the matrix only *stretches*: $\mathbf A\mathbf v=\lambda\mathbf v$; $\lambda$ is the **eigenvalue** (stretch factor).

*Verified:* $\begin{bmatrix}2&1\\1&2\end{bmatrix}$ has eigenvalues **1** and **3**, with eigenvectors $\tfrac1{\sqrt2}[1,-1]$ and $\tfrac1{\sqrt2}[1,1]$ — *perpendicular* to each other. $\operatorname{diag}(2,3)$ has eigenvalues 2 and 3.
**Rule:** symmetric (real) or Hermitian (complex) matrices have **real eigenvalues and orthogonal eigenvectors** — covariance matrices are of this type (P05).

---

## 4.9 SVD: the one decomposition worth knowing 🔥 (preview of MIMO diagonalisation)

*Any* matrix can be written

$$\mathbf H=\mathbf U\,\boldsymbol\Sigma\,\mathbf V^{H}$$

with $\mathbf U,\mathbf V$ having orthonormal columns (rotations) and $\boldsymbol\Sigma$ diagonal with non-negative **singular values** $\sigma_1\ge\sigma_2\ge\cdots$.

Meaning: *"rotate the input, stretch each axis independently by $\sigma_i$, rotate the output."* Consequences:
* **Rank = number of non-zero singular values.**
* **Condition number $\kappa=\sigma_{\max}/\sigma_{\min}$** — how unequal the stretches are (verified: 4002 above; a random $2\times2$ gave 2.12 with $\sigma=[1.318,\,0.621]$; reconstruction error $8\times10^{-17}$).
* **MIMO becomes parallel channels:** if the transmitter sends $\mathbf V\mathbf x'$ and the receiver applies $\mathbf U^{H}$, the system becomes $\mathbf y'=\boldsymbol\Sigma\mathbf x'+\mathbf n'$ — independent scalar channels with gains $\sigma_i$. *(Lesson 14 develops this; Lesson 46 mentions SVD in beamforming.)*

---

## 4.10 Least squares: the best answer when no exact one exists 🔥

If $\mathbf A\mathbf x=\mathbf b$ has no exact solution (more equations than unknowns, noisy data) choose $\mathbf x$ to **minimise the squared error** $\|\mathbf A\mathbf x-\mathbf b\|^{2}$:

$$\hat{\mathbf x}=(\mathbf A^{H}\mathbf A)^{-1}\mathbf A^{H}\mathbf b=\mathbf A^{+}\mathbf b$$

*Verified (line fit):* points $(0,1.1),(1,2.9),(2,5.2),(3,6.9)$ → slope **1.97**, intercept **1.07** — identical from `lstsq`, the normal equations and `pinv`.

**Channel-estimation toy (the idea of Lesson 11):** send a known pilot $\mathbf p=[1,\,j,\,-1,\,-j]$ through an unknown scalar channel, receive $\mathbf y=h\mathbf p+\mathbf n$. The least-squares estimate is
$$\hat h=\frac{\langle\mathbf p,\mathbf y\rangle}{\langle\mathbf p,\mathbf p\rangle}$$
*Verified:* true $h=0.8+0.6j$, estimate **0.820 + 0.644j** (error 0.049 with noise added). It is just a projection of $\mathbf y$ onto $\mathbf p$ (§4.4) — notice the conjugate in the inner product.

---

## 4.11 NumPy workout (verified)

```python
import numpy as np

a = np.array([1+1j, 2]); b = np.array([1, 1j])
print(np.vdot(a, b))        # (1+1j)  = a^H b   (conjugates first arg)
print(np.dot(a, b))         # (1+3j)  = NO conjugate!  -> not an inner product
print(np.vdot(a, a).real)   # 6.0     = energy

H = np.array([[2, 1], [1, 3]])
print(np.linalg.solve(H, [3, 5]))      # [0.8 1.4]  (prefer solve over inv)

M = np.array([[1, 2], [2, 4]])
print(np.linalg.matrix_rank(M), np.linalg.svd(M, compute_uv=False))  # 1  [5.0e+00 ~1e-16]  (second value is numerically zero)

Hbad = np.array([[1, 1], [1, 1.001]])
print(np.linalg.cond(Hbad))                       # ~4002
print(np.linalg.norm(np.linalg.pinv(Hbad), axis=1))  # ~[1415 1414] noise gain

U, S, Vh = np.linalg.svd(Hbad)                    # H = U @ diag(S) @ Vh
print(np.allclose(U @ np.diag(S) @ Vh, Hbad))     # True

# zero-forcing: x_hat = pinv(H) @ y
rng = np.random.default_rng(7)
H = (rng.standard_normal((2,2)) + 1j*rng.standard_normal((2,2))) / np.sqrt(2)
x = (np.array([1,-1,-1,1]) + 1j*np.array([1,1,-1,-1])).reshape(2,2) / np.sqrt(2)
y = H @ x + 0.07*(rng.standard_normal((2,2)) + 1j*rng.standard_normal((2,2)))
x_hat = np.linalg.pinv(H) @ y

# least squares
A = np.c_[np.arange(4.), np.ones(4)]; yv = np.array([1.1, 2.9, 5.2, 6.9])
print(np.linalg.lstsq(A, yv, rcond=None)[0])      # [1.97 1.07]
```

---

## 4.12 Common mistakes ⚠️
1. **`*` is element-wise; `@` is matrix multiplication.** `H*x` does not compute $\mathbf H\mathbf x$. (Verified: `H*H` = [[1,4],[9,16]] but `H@H` = [[7,10],[15,22]].)
2. **`np.dot` on complex vectors** does not conjugate. Use `np.vdot` or `a.conj() @ b`.
3. **`.T` vs `.conj().T`.** For complex matrices, $\mathbf A^{H}$ is `A.conj().T`. A plain transpose is wrong for most radio formulas.
4. **Shape traps.** A NumPy array of shape `(3,)` is neither a row nor a column. `v.reshape(-1,1) @ v.reshape(1,-1)` is $3\times3$ (outer product); `v @ v` is a scalar.
5. **Inverting explicitly.** Prefer `np.linalg.solve`/`lstsq`/`pinv` to `np.linalg.inv` — more accurate and faster.
6. **Ignoring the condition number.** A tiny determinant doesn't mean "tiny noise": always check `np.linalg.cond`.
7. **Assuming commutativity.** $\mathbf{AB}\ne\mathbf{BA}$.
8. **Norm of a complex vector with $x^2$ instead of $|x|^2$.**

---

## 4.13 Practice problems (answers at the end)

1. Find $\|[1,2,2]\|$ and the unit vector in its direction.
2. Compute $[2,-1,3]\cdot[1,4,0]$. Are they orthogonal?
3. For which $k$ is $[k,2]$ orthogonal to $[3,-6]$?
4. Compute $\langle\mathbf a,\mathbf b\rangle=\mathbf a^{H}\mathbf b$ for $\mathbf a=[1,\ j]$, $\mathbf b=[j,\ 1]$. What does the result mean?
5. Two CDMA users with $W_1=[1,1,1,1]$ and $W_2=[1,-1,1,-1]$ both send bit $+1$. The received chips are $[2,0,2,0]$. Despread with each code.
6. Compute $\mathbf A\mathbf x$ for $\mathbf A=\begin{bmatrix}1&2\\0&1\end{bmatrix}$, $\mathbf x=[3,1]^{T}$. With $\mathbf B=\begin{bmatrix}1&0\\1&1\end{bmatrix}$ compute $\mathbf{AB}$ and $\mathbf{BA}$.
7. Invert $\operatorname{diag}(4,\ 0.5)$.
8. Why is $\begin{bmatrix}1&2\\2&4\end{bmatrix}$ singular? State its rank.
9. A matrix has singular values 10 and 0.1. Condition number? Is zero-forcing a good idea?
10. Fit $y=ax+b$ to $(0,1),(1,3),(2,5)$ by least squares.
11. State the eigenvalues of $\operatorname{diag}(3,-1)$.
12. A channel has $\mathbf H$ of size $4\times2$. How many receive and transmit antennas? What is the maximum rank?
13. *Think:* a 2×2 line-of-sight channel with closely spaced antennas has rank 1. How many independent streams can it carry, and what does zero-forcing do to the noise?

## 4.14 Active recall
1. Why does the complex dot product need a conjugate? 2. State $\mathbf a\cdot\mathbf b=\|\mathbf a\|\|\mathbf b\|\cos\theta$ and three meanings. 3. What is projection? 4. How does the dot product explain CDMA despreading and DFT bins? 5. State the shape rule for matrix products. 6. What does the pseudo-inverse do and when is it needed? 7. What is rank, and why does it limit MIMO streams? 8. What does a large condition number mean physically? 9. Write the SVD and say what each factor does. 10. Why prefer `solve`/`lstsq` to `inv`? 11. Write the least-squares solution and the channel-estimate formula.

## 4.15 Self-check — ready for P05 if you can…
- [ ] compute norms and dot products (real *and* complex) and explain orthogonality;
- [ ] use `@`, `.conj().T`, `np.vdot` correctly;
- [ ] explain why an ill-conditioned channel amplifies noise under zero-forcing;
- [ ] state what rank, condition number and singular values tell you about a channel;
- [ ] derive a channel estimate as a projection.

---

## ✅ Answer key

1. $\sqrt{1+4+4}=\mathbf3$; unit vector $[\tfrac13,\tfrac23,\tfrac23]$. 2. $2-4+0=\mathbf{-2}$ — **not** orthogonal. 3. $3k-12=0\Rightarrow\mathbf{k=4}$. 4. $\bar1\cdot j+\bar j\cdot1=j-j=\mathbf0$ — the vectors are **orthogonal**. 5. $\mathbf r\cdot W_1/4=(2+0+2+0)/4=\mathbf{+1}$; $\mathbf r\cdot W_2/4=(2-0+2-0)/4=\mathbf{+1}$ — both bits recovered. 6. $\mathbf A\mathbf x=[5,1]^{T}$; $\mathbf{AB}=\begin{bmatrix}3&2\\1&1\end{bmatrix}$, $\mathbf{BA}=\begin{bmatrix}1&2\\1&3\end{bmatrix}$ (not equal). 7. $\operatorname{diag}(0.25,\ 2)$. 8. Row 2 = 2 × row 1 ⇒ $\det=0$ ⇒ columns dependent; **rank 1**. 9. $\kappa=10/0.1=\mathbf{100}$ — noise along the weak direction is amplified by up to 10× in amplitude (1/σ_min); prefer MMSE or fewer streams. 10. Points are exactly on $y=2x+1$: $\mathbf{a=2,\ b=1}$. 11. **3 and −1**. 12. **4 receive, 2 transmit**; max rank = min(4,2) = **2**. 13. **One stream**; zero-forcing would divide by (near-)zero singular values and **blow up the noise** — exactly the Lesson 14 failure mode.

---

*Next: **P05 — Probability and statistics for noise** (random variables, the Gaussian, the Q-function, Monte-Carlo, confidence intervals) — the maths behind BER curves.*
