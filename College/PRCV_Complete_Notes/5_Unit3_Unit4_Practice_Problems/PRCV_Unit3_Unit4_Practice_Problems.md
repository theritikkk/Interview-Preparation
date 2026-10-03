# PRCV (ML-411T) — Units 3 & 4: Solved Numericals + Practice Problems

> Companion to the **Unit 3 & 4 revision notes**. Every answer was **recomputed with code**, so the numbers can be trusted.
> **How to use it:** cover the solution, solve on paper, then compare. ★ = most likely exam shape.
> Conventions: images are indexed (row, column) = $(y,x)$; $(x,y)$ is used when a point is written as coordinates. Formulas are LaTeX — open in Typora / Obsidian / VS Code Preview / GitHub / Jupyter.

**Contents:** (U3) 1 Histogram equalisation ★ · 2 Spatial filters ★ · 3 Otsu + iterative threshold ★ · 4 Sobel / Prewitt ★ · 5 Harris corner response · 6 Morphology ★ · 7 GLCM ★ · 8 LBP
(U4) 9 Labelling, size filter, Euler ★ · 10 Chamfer distance transform ★ · 11 Chain code & shape number ★ · 12 Zhang–Suen thinning · 13 Moments & orientation ★ · 14 Fourier descriptors · 15 Region & boundary descriptors · 16 Snake energy · 17 Occlusion prediction · **Answer-writing tips**

---

# UNIT 3

## Problem 1 ★ — Histogram equalisation

A 3-bit image ($L=8$, 16 pixels):

$$\begin{bmatrix}0&1&1&2\\1&2&2&3\\2&3&3&3\\3&4&5&7\end{bmatrix}$$

Equalise it: $s_k=\operatorname{round}\big((L-1)\sum_{j\le k}p(r_j)\big)$.

**Solution.**

| $r$ | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| count $n$ | 1 | 3 | 4 | 5 | 1 | 1 | 0 | 1 |
| $p=n/16$ | 0.0625 | 0.1875 | 0.25 | 0.3125 | 0.0625 | 0.0625 | 0 | 0.0625 |
| CDF | 0.0625 | 0.25 | 0.5 | 0.8125 | 0.875 | 0.9375 | 0.9375 | 1 |
| $7\times$CDF | 0.44 | 1.75 | 3.5 | 5.69 | 6.13 | 6.56 | 6.56 | 7 |
| **$s$** | **0** | **2** | **4** | **6** | **6** | **7** | **7** | **7** |

(3.5 rounds up to 4.) Replace each pixel by its $s$:

$$\begin{bmatrix}0&2&2&4\\2&4&4&6\\4&6&6&6\\6&6&7&7\end{bmatrix}$$

The gray levels now spread over 0–7 (before: mostly 1–3). With only 8 levels the histogram cannot become exactly flat; levels 3 and 4 merged into 6, so equalisation can **merge** levels.

---

## Problem 2 ★ — Spatial filters on a noisy patch

$$P=\begin{bmatrix}12&11&13\\13&\mathbf{200}&12\\11&12&14\end{bmatrix}\quad(\text{centre is an impulse})$$

**(a)** Output at the centre of a $3\times3$ **mean**, **median** and **Gaussian** ($\tfrac1{16}\begin{bmatrix}1&2&1\\2&4&2\\1&2&1\end{bmatrix}$) filter.
**(b)** Apply the Laplacian sharpening $g=f-\nabla^2f$ (mask $\begin{bmatrix}0&1&0\\1&-4&1\\0&1&0\end{bmatrix}$) at the centre of $Q=\begin{bmatrix}10&10&50\\10&\mathbf{10}&50\\10&10&50\end{bmatrix}$.

**Solution.**

**(a)** Mean $=\dfrac{12+11+13+13+200+12+11+12+14}{9}=\dfrac{298}{9}=\mathbf{33.1}$.
Median: sorted $11,11,12,12,12,13,13,14,200$ → **12**.
Gaussian: $\dfrac{(12+2\cdot11+13)+(2\cdot13+4\cdot200+2\cdot12)+(11+2\cdot12+14)}{16}=\dfrac{946}{16}=\mathbf{59.1}$.
→ The median ignores the impulse (best for salt-and-pepper noise); mean and Gaussian smear it.

**(b)** $\nabla^2f=(10+10+10+50)-4(10)=+40$, so $g=10-40=-30\to$ **clipped to 0**.
The dark pixel next to the edge becomes darker (undershoot) — this overshoot/undershoot is what makes the edge look sharper.

---

## Problem 3 ★ — Otsu's method and the iterative threshold

An 8-level image ($0$–$7$) has histogram counts $[4,\ 9,\ 13,\ 8,\ 2,\ 6,\ 12,\ 6]$ ($N=60$, mean gray level $3.52$).

**(a)** Find Otsu's threshold. **(b)** Find the threshold from the iterative (mean-of-means) algorithm starting from the global mean.

**Solution.**

**(a)** $\sigma_B^2(k)=P_1P_2\,(m_1-m_2)^2$ for each candidate $k$ (pixels $\le k$ = class 1):

| $k$ | $P_1$ | $m_1$ | $m_2$ | $\sigma_B^2$ |
|---|---|---|---|---|
| 0 | 0.067 | 0.00 | 3.77 | 0.883 |
| 1 | 0.217 | 0.69 | 4.30 | 2.206 |
| 2 | 0.433 | 1.35 | 5.18 | 3.603 |
| **3** | **0.567** | **1.74** | **5.85** | **4.150** ← max |
| 4 | 0.600 | 1.86 | 6.00 | 4.111 |
| 5 | 0.700 | 2.31 | 6.33 | 3.400 |
| 6 | 0.900 | 3.13 | 7.00 | 1.348 |

→ **$T^*=3$**: pixels $\le3$ are background, $\ge4$ object (the valley at level 4).

**(b)** $T_0=3.52$. Group above: levels 4–7 → $m_1=5.85$; group below: levels 0–3 → $m_2=1.74$. $T_1=\tfrac12(5.85+1.74)=3.79$. Re-splitting at 3.79 gives the same groups, so it **converges to $T\approx3.79$**, i.e. the same cut between levels 3 and 4 as Otsu.

---

## Problem 4 ★ — Sobel and Prewitt gradient

Patches: $\;A=\begin{bmatrix}20&20&80\\20&20&80\\20&20&80\end{bmatrix}$ and $B=\begin{bmatrix}60&30&10\\30&10&10\\10&10&10\end{bmatrix}$. Find $G_x$, $G_y$, magnitude and direction at the centre.

Masks: Sobel $G_x=\begin{bmatrix}-1&0&1\\-2&0&2\\-1&0&1\end{bmatrix}$, $G_y=\begin{bmatrix}-1&-2&-1\\0&0&0\\1&2&1\end{bmatrix}$; Prewitt uses weights $1,1,1$ instead of $1,2,1$.

**Solution.**

**Patch A (vertical edge):** Sobel $G_x=(60)+2(60)+(60)=240$, $G_y=0$ → $M=\mathbf{240}$, $\alpha=\mathbf{0^\circ}$ (gradient points right; the edge is vertical).
Prewitt $G_x=3(60)=180$, $G_y=0$ → $M=180$.

**Patch B:** Sobel $G_x=(-60+10)+(-60+20)+(-10+10)=-90$; $G_y=(-60-60-10)+(10+20+10)=-90$.
$$M=\sqrt{90^2+90^2}=\mathbf{127.3},\qquad \alpha=\tan^{-1}\frac{-90}{-90}=-135^\circ\ (\text{i.e. }45^\circ\text{ diagonal gradient})$$
Prewitt: $G_x=G_y=-70$ → $M=99.0$.

Sobel gives the **larger** response because the centre row/column has weight 2 (extra smoothing). The edge direction is **perpendicular** to the gradient.

---

## Problem 5 — Harris corner response

For a window, the summed gradient products are $\sum I_x^2,\ \sum I_y^2,\ \sum I_xI_y$. With $k=0.04$ classify:

| Case | $\sum I_x^2$ | $\sum I_y^2$ | $\sum I_xI_y$ |
|---|---|---|---|
| 1 | 100 | 80 | 10 |
| 2 | 100 | 2 | 0 |
| 3 | 1 | 1 | 0 |

**Solution.** $M=\begin{bmatrix}\sum I_x^2&\sum I_xI_y\\\sum I_xI_y&\sum I_y^2\end{bmatrix}$, $R=\det M-k(\operatorname{tr}M)^2$.

| Case | eigenvalues | $\det$ | $\operatorname{tr}$ | $R$ | Class |
|---|---|---|---|---|---|
| 1 | 75.9, 104.1 | 7900 | 180 | $7900-0.04(180)^2=\mathbf{+6604}$ | **corner** (both large, $R\gg0$) |
| 2 | 2, 100 | 200 | 102 | $200-0.04(102)^2=\mathbf{-216}$ | **edge** ($R<0$; one large, one small) |
| 3 | 1, 1 | 1 | 2 | $1-0.04(4)=\mathbf{+0.84}$ | **flat** ($R\approx0$) |

---

## Problem 6 ★ — Binary morphology

Binary image $A$ ($6\times9$); structuring element: $3\times3$ square (all ones); pixels outside the image count as 0.

```
A:                      erosion A⊖B:            dilation A⊕B:
0 0 0 0 0 0 0 0 0       0 0 0 0 0 0 0 0 0       1 1 1 1 1 0 1 1 1
0 1 1 1 0 0 0 1 0       0 0 0 0 0 0 0 0 0       1 1 1 1 1 1 1 1 1
0 1 1 1 1 1 0 0 0       0 0 1 0 0 0 0 0 0       1 1 1 1 1 1 1 1 1
0 1 1 1 1 1 0 0 0       0 0 0 0 1 0 0 0 0       1 1 1 1 1 1 1 0 0
0 0 0 1 1 1 0 0 0       0 0 0 0 0 0 0 0 0       1 1 1 1 1 1 1 0 0
0 0 0 0 0 0 0 0 0       0 0 0 0 0 0 0 0 0       0 0 1 1 1 1 1 0 0
```

**(a)** Opening and closing. **(b)** Boundary extraction using a $+$-shaped (cross) SE.

**Solution.**

**(a)** *Opening* $A\circ B=(A\ominus B)\oplus B$ — the isolated pixel at row 1, col 7 is erased; the main body is restored exactly:
```
0 0 0 0 0 0 0 0 0
0 1 1 1 0 0 0 0 0
0 1 1 1 1 1 0 0 0
0 1 1 1 1 1 0 0 0
0 0 0 1 1 1 0 0 0
0 0 0 0 0 0 0 0 0
```
*Closing* $A\bullet B=(A\oplus B)\ominus B$ returns $A$ unchanged — there are no holes/gaps narrower than the SE to fill.

**(b)** Erosion with the cross SE keeps only the pixels whose 4-neighbours are all object pixels (centre $(2,2),(2,3),(3,3),(3,4)$), so $\beta(A)=A-(A\ominus B_{+})$ is:
```
0 0 0 0 0 0 0 0 0
0 1 1 1 0 0 0 1 0
0 1 0 0 1 1 0 0 0
0 1 1 0 0 1 0 0 0
0 0 0 1 1 1 0 0 0
0 0 0 0 0 0 0 0 0
```
Note: opening is **anti-extensive** ($A\circ B\subseteq A$), removes small objects; closing is **extensive**, fills small holes.

---

## Problem 7 ★ — GLCM and Haralick features

3-level image:

$$\begin{bmatrix}0&0&1&2\\0&1&1&2\\1&1&2&2\\0&1&2&2\end{bmatrix}$$

Build the GLCM for $d=1$ with $\theta=0^\circ$ (neighbour to the right) and $\theta=90^\circ$ (neighbour below). Compute contrast, energy and homogeneity.

**Solution.** Row $i$ = gray level of the reference pixel, column $j$ = gray level of the neighbour (12 pairs each).

$$P_{0^\circ}=\begin{bmatrix}1&3&0\\0&2&4\\0&0&2\end{bmatrix}\qquad P_{90^\circ}=\begin{bmatrix}1&2&0\\1&3&1\\0&0&4\end{bmatrix}$$

With $p=P/12$:

| Feature | formula | $0^\circ$ | $90^\circ$ |
|---|---|---|---|
| Contrast | $\sum(i-j)^2p$ | $\frac{3+4}{12}=\mathbf{0.583}$ | $\frac{2+1+1}{12}=\mathbf{0.333}$ |
| Energy | $\sum p^2$ | $\frac{1+9+4+16+4}{144}=\mathbf{0.236}$ | $\frac{1+4+1+9+1+16}{144}=\mathbf{0.222}$ |
| Homogeneity | $\sum\frac{p}{1+\lvert i-j\rvert}$ | $\mathbf{0.708}$ | $\mathbf{0.833}$ |

The texture changes more along the rows ($0^\circ$) than down the columns ($90^\circ$): vertical neighbours are more alike → this texture is **directional**. *(Symmetric GLCM $P+P^T$ gives the same contrast and homogeneity but a lower energy, 0.149.)*

---

## Problem 8 — Local Binary Pattern

$$\begin{bmatrix}5&9&1\\4&\mathbf4&6\\7&2&3\end{bmatrix}$$

Threshold the 8 neighbours against the centre ($\ge$ → 1), read **clockwise from the top-left** with weights $1,2,4,8,16,32,64,128$.

**Solution.** Neighbours clockwise (TL, T, TR, R, BR, B, BL, L) $=5,9,1,6,3,2,7,4$ → bits $1,1,0,1,0,0,1,1$.
$$\text{LBP}=1\cdot1+1\cdot2+0+1\cdot8+0+0+1\cdot64+1\cdot128=\mathbf{203}$$
Brightness changes that preserve order (adding a constant, scaling) leave the code unchanged — LBP is robust to monotonic illumination change.

---

# UNIT 4

## Problem 9 ★ — Connectivity, labelling, size filtering and Euler number

```
1 1 0 0 0 1 0 0
1 0 0 0 1 1 0 1
0 0 1 0 0 0 0 1
0 1 1 0 0 1 0 1
0 0 0 0 1 0 0 0
1 0 0 0 0 0 1 1
```

**(a)** Label with 4- and 8-connectivity (count objects and areas). **(b)** Size-filter (keep area $\ge2$). **(c)** Euler number.

**Solution.**

**(a)** Components (row, col):
- $\{(0,0),(0,1),(1,0)\}$ — area 3
- $\{(0,5),(1,4),(1,5)\}$ — area 3
- $\{(1,7),(2,7),(3,7)\}$ — area 3
- $\{(2,2),(3,1),(3,2)\}$ — area 3
- $(3,5)$, $(4,4)$ — touch **only diagonally**
- $(5,0)$ — area 1; $\{(5,6),(5,7)\}$ — area 2

| | 4-connectivity | 8-connectivity |
|---|---|---|
| Number of objects | **8** | **7** |
| Areas | 3, 3, 3, 3, 1, 1, 1, 2 | 3, 3, 3, 3, **2** (the diagonal pair merges), 1, 2 |

**(b)** Keep area $\ge2$: 4-connectivity → **5** objects (the three single pixels $(3,5),(4,4),(5,0)$ are removed); 8-connectivity → **6** objects (only $(5,0)$ is removed). Keep area $\ge3$: **4** objects in both cases.

**(c)** No background region is enclosed, so $H=0$ and $E=C-H$ = **7** (8-connected) or **8** (4-connected).

---

## Problem 10 ★ — Chamfer 3-4 distance transform

Object pixels = 1 (distance to the nearest background pixel; the border outside is background):

```
0 0 0 0 0 0 0
0 1 1 1 1 1 0
0 1 1 1 1 1 0
0 1 1 1 1 0 0
0 0 1 1 0 0 0
```

Initialise background $=0$, object $=\infty$. **Forward pass** (top-left → bottom-right) uses neighbours $W{+}3,\ N{+}3,\ NW{+}4,\ NE{+}4$; **backward pass** uses $E{+}3,\ S{+}3,\ SE{+}4,\ SW{+}4$.

**Solution.**

After the forward pass:
```
0 0 0 0 0 0 0
0 3 3 3 3 3 0
0 3 6 6 6 4 0
0 3 6 9 8 0 0
0 0 3 6 0 0 0
```
After the backward pass (final chamfer distances):
```
0 0 0 0 0 0 0
0 3 3 3 3 3 0
0 3 6 6 4 3 0
0 3 4 4 3 0 0
0 0 3 3 0 0 0
```
Divide by 3 to get approximate Euclidean distances: $1,\ 2,\ 1.33$. Compare with the exact Euclidean values (1, 2, **1.41**) and the other metrics at the same pixels:

| pixel type | chamfer/3 | Euclidean | city-block | chessboard |
|---|---|---|---|---|
| diagonal-corner pixels, e.g. $(3,2)$ | 1.33 | 1.41 | 2 | 1 |
| deep interior, e.g. $(2,2)$ | 2 | 2 | 2 | 2 |

Chamfer 3-4 is a cheap two-pass approximation to Euclidean distance (error ≲ 8 %); $D_4$ over-estimates diagonal distances, $D_8$ under-estimates them.

---

## Problem 11 ★ — Chain code, first difference, shape number, length

A closed 8-connected boundary visits $(x,y)$ (with $y$ pointing up):
$(1,1)\to(2,1)\to(3,1)\to(4,2)\to(4,3)\to(3,4)\to(2,4)\to(1,3)\to(1,2)\to(1,1)$.
8-direction code: $0=E,1=NE,2=N,3=NW,4=W,5=SW,6=S,7=SE$.

**(a)** Chain code. **(b)** First difference and shape number. **(c)** Boundary length. **(d)** Effect of rotating the shape by $90^\circ$.

**Solution.**

**(a)** Moves: $(1,0)\!\to\!0$, $(1,0)\!\to\!0$, $(1,1)\!\to\!1$, $(0,1)\!\to\!2$, $(-1,1)\!\to\!3$, $(-1,0)\!\to\!4$, $(-1,-1)\!\to\!5$, $(0,-1)\!\to\!6$, $(0,-1)\!\to\!6$:
$$\text{code}=0\,0\,1\,2\,3\,4\,5\,6\,6$$
This is already the minimum-magnitude rotation (start-point normalised).

**(b)** $d_i=(c_{i+1}-c_i)\bmod 8$ (circular): $0\to0{:}0,\ 0\to1{:}1,\ 1\to2{:}1,\ \dots,\ 5\to6{:}1,\ 6\to6{:}0,\ 6\to0{:}(0-6)\bmod8=2$:
$$\text{first difference}=0\,1\,1\,1\,1\,1\,1\,0\,2$$
Its minimum-magnitude circular rotation is itself ($0\,1\ldots$ is smaller than $0\,2\ldots$) ⇒ **shape number $=011111102$**.

**(c)** Even (axis) steps $N_e=6$ (codes 0,0,2,4,6,6), odd (diagonal) steps $N_o=3$ (codes 1,3,5):
$$P=N_e+\sqrt2\,N_o=6+3\sqrt2=\mathbf{10.24}$$

**(d)** Rotating by $90^\circ$ adds 2 (mod 8) to every digit: $2\,2\,3\,4\,5\,6\,7\,0\,0$. The raw code changes, but the **first difference is identical** ($0\,1\,1\,1\,1\,1\,1\,0\,2$), so the shape number is rotation-invariant (for multiples of $45^\circ$).

---

## Problem 12 — Zhang–Suen thinning decisions

For the neighbourhoods below ($P_1$ is the centre, $P_2$ above, then clockwise to $P_9$), compute $B(P_1)$ (number of object neighbours) and $A(P_1)$ (number of $0\to1$ transitions in $P_2,P_3,\dots,P_9,P_2$) and decide whether $P_1$ is deleted.

$$(a)\ \begin{bmatrix}0&1&0\\0&1&1\\0&0&0\end{bmatrix}\qquad (b)\ \begin{bmatrix}1&1&0\\1&1&0\\1&0&0\end{bmatrix}\qquad (c)\ \begin{bmatrix}0&0&0\\1&1&1\\1&1&1\end{bmatrix}$$

Rules: delete if $2\le B\le6$, $A=1$, and — sub-iteration 1: $P_2P_4P_6=0$ and $P_4P_6P_8=0$; sub-iteration 2: $P_2P_4P_8=0$ and $P_2P_6P_8=0$.

**Solution.**

| | $(P_2..P_9)$ | $B$ | $A$ | Sub-iter. 1 | Sub-iter. 2 | Result |
|---|---|---|---|---|---|---|
| (a) | 1,0,1,0,0,0,0,0 | 2 | **2** | ✗ ($A\neq1$) | ✗ | **kept** — deleting it would disconnect $P_2$ from $P_4$ |
| (b) | 1,0,0,0,0,1,1,1 | 4 | 1 | ✓ ($P_2P_4P_6=0$, $P_4P_6P_8=0$) | ✓ | **deleted** (a redundant corner pixel) |
| (c) | 0,0,1,1,1,1,1,0 | 5 | 1 | ✗ ($P_4P_6P_8=1$) | ✓ ($P_2=0$) | **kept in pass 1, deleted in pass 2** (a pixel on the top boundary, removed by the second sub-iteration) |

---

## Problem 13 ★ — Moments, orientation, eccentricity, Hu invariants

Binary shape (6 object pixels), coordinates $(x,y)$ = (column, row):

```
0 0 0 0 0
0 1 1 1 0
0 1 1 0 0
0 1 0 0 0
0 0 0 0 0
```

**(a)** $m_{00},m_{10},m_{01}$ and the centroid. **(b)** $\mu_{20},\mu_{02},\mu_{11}$ and the orientation. **(c)** Eccentricity. **(d)** $\eta_{20},\eta_{02},\eta_{11},\phi_1,\phi_2$.

**Solution.**

**(a)** Pixels $(1,1),(2,1),(3,1),(1,2),(2,2),(1,3)$: $m_{00}=6$, $m_{10}=\sum x=10$, $m_{01}=\sum y=10$:
$$\bar x=\bar y=\frac{10}{6}=\mathbf{1.667}$$

**(b)** Deviations $(x-\bar x)$: $-0.667,0.333,1.333,-0.667,0.333,-0.667$; $(y-\bar y)$: $-0.667,-0.667,-0.667,0.333,0.333,1.333$.
$$\mu_{20}=3.333,\quad \mu_{02}=3.333,\quad \mu_{11}=-1.667$$
$$\theta=\tfrac12\tan^{-1}\!\frac{2\mu_{11}}{\mu_{20}-\mu_{02}}=\tfrac12\tan^{-1}\!\frac{-3.333}{0}=\tfrac12(-90^\circ)=\mathbf{-45^\circ}$$
(major axis along the anti-diagonal, from $(3,1)$ to $(1,3)$ — it matches the picture).

**(c)** $\lambda_{1,2}=\dfrac{\mu_{20}+\mu_{02}\pm\sqrt{(\mu_{20}-\mu_{02})^2+4\mu_{11}^2}}{2\,m_{00}}=\dfrac{6.667\pm3.333}{12}=0.833,\ 0.278$:
$$e=\sqrt{1-\lambda_2/\lambda_1}=\sqrt{1-\tfrac13}=\mathbf{0.816}$$

**(d)** $\eta_{pq}=\mu_{pq}/m_{00}^{(p+q)/2+1}=\mu_{pq}/36$ for $p+q=2$:
$$\eta_{20}=\eta_{02}=0.0926,\quad \eta_{11}=-0.0463$$
$$\phi_1=\eta_{20}+\eta_{02}=\mathbf{0.1852},\qquad \phi_2=(\eta_{20}-\eta_{02})^2+4\eta_{11}^2=\mathbf{0.00857}$$
**Check of invariance:** rotating the shape by $90^\circ$ gives $\phi_1=0.1852$ again.

---

## Problem 14 — Fourier descriptors and their invariance

A rectangle sampled at its four corners: $s(k)=x+jy$ for $k=0..3$: $s=[\,0,\ 2,\ 2+j,\ j\,]$.

**(a)** Compute $a(u)=\sum_k s(k)e^{-j2\pi uk/4}$. **(b)** Normalised descriptors $d(u)=\lvert a(u)\rvert/\lvert a(1)\rvert$. **(c)** Show they survive scaling by 2, rotation by $90^\circ$ (multiply by $j$) and translation by $5-2j$.

**Solution.**

**(a)** $a(0)=4+2j$ (= $K\times$ centroid), $a(1)=-3-3j$, $a(2)=0$, $a(3)=-1+j$.
Magnitudes: $\lvert a\rvert=[4.47,\ 4.24,\ 0,\ 1.41]$.

**(b)** Ignore $a(0)$ (translation) and divide by $\lvert a(1)\rvert=4.24$:
$$d(u)=\big[\,1,\ 0,\ 0.333\,\big]\quad(u=1,2,3)$$

**(c)** New boundary $s'=2j\,s+(5-2j)$: $a'(0)=16$ (translation changed it), $a'(1)=6-6j$, $a'(2)=0$, $a'(3)=-2-2j$ ⇒ $\lvert a'\rvert=[16,\ 8.49,\ 0,\ 2.83]$ and
$$d'(u)=\big[\,1,\ 0,\ 0.333\,\big]\ \ \text{— identical.}$$
Also shifting the start point by one sample only changes phases, so the magnitudes are unchanged.
**Reconstruction:** keeping only $a(0),a(1)$ gives a smoothed approximation $(0.25-0.25j,\ 1.75-0.25j,\ 1.75+1.25j,\ 0.25+1.25j)$ (a square-ish blob); adding $a(3)$ restores the rectangle exactly. Fewer coefficients ⇒ smoother shape.

---

## Problem 15 — Region and boundary descriptors

A region has area $A=200$ px, boundary with $N_e=40$ axis steps and $N_o=14$ diagonal steps, convex-hull area $250$, bounding box $25\times16$.

**Solution.**
- Perimeter (chain-code length): $P=40+14\sqrt2=\mathbf{59.8}$; with Kulpa's correction $0.948(40)+1.343(14)=\mathbf{56.7}$ (closer to the real boundary length because the staircase overestimates).
- Circularity $=\dfrac{4\pi A}{P^2}=\dfrac{4\pi(200)}{59.8^2}\approx\mathbf{0.70}$ (1 for a circle); the inverse, compactness $\dfrac{P^2}{4\pi A}\approx\mathbf{1.42}$ ($\ge1$).
- Solidity $=\dfrac{A}{A_{hull}}=\dfrac{200}{250}=\mathbf{0.80}$ (concave).
- Extent $=\dfrac{A}{w\,h}=\dfrac{200}{25\cdot16}=\mathbf{0.50}$; aspect ratio $=25/16=\mathbf{1.56}$.
- Equivalent diameter $=\sqrt{4A/\pi}=\mathbf{15.96}$.
Interpretation: moderately round (0.70), with a notable concavity (solidity 0.8).

---

## Problem 16 — One snake energy evaluation

A contour point $v_i$ has fixed neighbours $v_{i-1}=(0,0)$ and $v_{i+1}=(4,0)$. Candidate positions for $v_i$ and the external energy $E_{ext}=-\lvert\nabla I\rvert^2$ there:

| Candidate | $v_i$ | $E_{ext}$ |
|---|---|---|
| A | $(2,0)$ | $-5$ |
| B | $(2,1)$ | $-20$ |
| C | $(2,2)$ | $-12$ |

Use $E_{int}=\alpha\,\lvert v_i-v_{i-1}\rvert^2+\beta\,\lvert v_{i-1}-2v_i+v_{i+1}\rvert^2$ with $\alpha=1$. Which position does a greedy snake choose for $\beta=2$ and for $\beta=5$?

**Solution.**

| | stretch $\lvert v_i-v_{i-1}\rvert^2$ | bend $\lvert v_{i-1}-2v_i+v_{i+1}\rvert^2$ |
|---|---|---|
| A | $4$ | $\lvert(0,0)-(4,0)+(4,0)\rvert^2=0$ |
| B | $5$ | $\lvert(0,-2)\rvert^2=4$ |
| C | $8$ | $\lvert(0,-4)\rvert^2=16$ |

| $\beta$ | A | B | C | Chosen |
|---|---|---|---|---|
| 2 | $4+0-5=\mathbf{-1}$ | $5+8-20=\mathbf{-7}$ | $8+32-12=28$ | **B** (strongest edge wins) |
| 5 | $4+0-5=\mathbf{-1}$ | $5+20-20=5$ | $8+80-12=76$ | **A** (stiffness wins) |

**Lesson:** a larger bending weight $\beta$ makes the snake **stiffer/smoother** and can make it ignore a real edge; a smaller $\beta$ lets it follow the edge into bends.

---

## Problem 17 — Predicting through occlusion

A tracked object moves with constant velocity $v=3$ px/frame; at frame 10 it is at $x=40$ with position variance $\sigma_x^2=1$ and velocity variance $\sigma_v^2=0.5$ (assume uncorrelated). It is **occluded in frames 11–14** and reappears at frame 15 measured at $x=57$.

**Solution.** Predicted position $\hat x_n=40+3n$ and (simplified) variance $\sigma^2_n=\sigma_x^2+n^2\sigma_v^2$:

| frame | $n$ | predicted $x$ | variance | $\sigma$ |
|---|---|---|---|---|
| 11 | 1 | 43 | 1.5 | 1.22 |
| 12 | 2 | 46 | 3.0 | 1.73 |
| 13 | 3 | 49 | 5.5 | 2.35 |
| 14 | 4 | 52 | 9.0 | 3.00 |
| 15 | 5 | 55 | 13.5 | 3.67 |

At frame 15: innovation $=57-55=2$ px, only $0.54\sigma$ → well inside a $3\sigma$ gate ⇒ **accept** the measurement as the same object and run the Kalman update (the large predicted uncertainty gives a large gain, so the estimate jumps toward 57).
The key idea: with no measurement we only **predict**, so **uncertainty grows**; when the object reappears we update and it shrinks again.

---

# Answer-writing tips for Unit 3 & 4 numericals

1. **State the formula and mask first** (Sobel masks, SE shape, GLCM direction, connectivity type) — wrong convention = lost marks even with correct arithmetic.
2. **Say how you treat borders** (zero padding / replicate) in filtering and morphology problems.
3. **Show the intermediate table** (Otsu per-threshold table, CDF table, two-pass labels, forward/backward distance passes).
4. **Label which connectivity** you use for objects and for background; the object count depends on it.
5. For chain codes always give: start point, direction convention, code, **first difference** (for rotation invariance) and shape number.
6. For moments give $m_{00}$, centroid, then central moments — never use raw moments for shape comparison (they depend on position).
7. Finish each problem with a **one-line conclusion** ("threshold T = 3", "7 objects under 8-connectivity", "snake chooses B").

*Good luck with the exam!*
