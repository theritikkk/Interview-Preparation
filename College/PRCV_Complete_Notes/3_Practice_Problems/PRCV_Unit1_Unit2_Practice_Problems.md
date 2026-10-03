# PRCV (ML-411T) — Units 1 & 2: Solved Numericals + Practice Problems

> Companion to your **Unit 1 & 2 revision notes**. Every answer below was **recomputed with code**, so the numbers are safe to rely on.
> **How to use it:** cover the solution, solve the problem on paper, then compare. Problems marked ★ are the most likely exam shapes.
> Formulas are LaTeX (`$…$`); open in Typora / Obsidian / VS Code Preview / GitHub / Jupyter.

**Contents:** (U1) 1 Entropy & information gain ★ · 2 Naïve Bayes + Laplace ★ · 3 Bayes' theorem & Bayes decision rule ★ · 4 k-NN · 5 SVM · 6 Association-rule metrics · 7 Genetic algorithm · 8 Neuron / backprop
(U2) 9 Polynomial least squares & overfitting ★ · 10 Fisher's linear discriminant ★ · 11 MLE & sequential estimation · 12 Kalman filter · 13 Metrics, scaling, curse of dimensionality · **Answer-writing tips**

---

# UNIT 1

## Problem 1 ★ — Entropy, information gain, Gini, gain ratio (decision tree root)

The 14-row weather data (Outlook, Temperature, Humidity, Windy → Play golf):

| # | Outlook | Temp | Humidity | Windy | Play |
|---|---|---|---|---|---|
| 0 | Rainy | Hot | High | F | No |
| 1 | Rainy | Hot | High | T | No |
| 2 | Overcast | Hot | High | F | Yes |
| 3 | Sunny | Mild | High | F | Yes |
| 4 | Sunny | Cool | Normal | F | Yes |
| 5 | Sunny | Cool | Normal | T | No |
| 6 | Overcast | Cool | Normal | T | Yes |
| 7 | Rainy | Mild | High | F | No |
| 8 | Rainy | Cool | Normal | F | Yes |
| 9 | Sunny | Mild | Normal | F | Yes |
| 10 | Rainy | Mild | Normal | T | Yes |
| 11 | Overcast | Mild | High | T | Yes |
| 12 | Overcast | Hot | Normal | F | Yes |
| 13 | Sunny | Mild | High | T | No |

**(a)** Compute $H(\text{Play})$. **(b)** Information gain of each attribute; which is the root? **(c)** Gini gain and gain ratio of Outlook. **(d)** Write the rule for the pure branch.

**Solution.**

**(a)** 9 Yes, 5 No:
$$H(S)=-\tfrac9{14}\log_2\tfrac9{14}-\tfrac5{14}\log_2\tfrac5{14}=0.4098+0.5305=\mathbf{0.940}$$

**(b)** $\text{Gain}(S,A)=H(S)-\sum_v\frac{|S_v|}{|S|}H(S_v)$

| Attribute | Branches (Yes, No) → entropy | Remainder | **Gain** |
|---|---|---|---|
| **Outlook** | Overcast (4,0)→0 ; Rainy (2,3)→0.971 ; Sunny (3,2)→0.971 | $\frac4{14}\!\cdot\!0+\frac5{14}\!\cdot\!0.971+\frac5{14}\!\cdot\!0.971=0.694$ | **0.247** |
| Humidity | High (3,4)→0.985 ; Normal (6,1)→0.592 | $\frac7{14}(0.985)+\frac7{14}(0.592)=0.789$ | 0.152 |
| Windy | False (6,2)→0.811 ; True (3,3)→1 | $\frac8{14}(0.811)+\frac6{14}(1)=0.892$ | 0.048 |
| Temperature | Cool (3,1)→0.811 ; Hot (2,2)→1 ; Mild (4,2)→0.918 | $0.911$ | 0.029 |

→ **Outlook** has the largest gain ⇒ it is the **root**.

**(c)** Gini of the root: $1-(\tfrac9{14})^2-(\tfrac5{14})^2=0.459$. After splitting on Outlook: $\tfrac5{14}(0.48)+\tfrac5{14}(0.48)+\tfrac4{14}(0)=0.343$. **Gini gain $=0.459-0.343=0.116$.**
Split information: $-\big(2\cdot\tfrac5{14}\log_2\tfrac5{14}+\tfrac4{14}\log_2\tfrac4{14}\big)=1.577$, so **gain ratio $=0.247/1.577=0.156$.**

**(d)** Overcast branch is pure (4 Yes, 0 No): `IF Outlook = Overcast THEN Play = Yes`. (Sunny and Rainy still need a further split.)

---

## Problem 2 ★ — Naïve Bayes, then Laplace smoothing

Use the table in Problem 1. Priors: $P(\text{Yes})=\tfrac9{14}$, $P(\text{No})=\tfrac5{14}$.

**Counts you need** (Yes has 9 rows, No has 5):

| Value | Yes | No | | Value | Yes | No |
|---|---|---|---|---|---|---|
| Sunny | 3 | 2 | | Overcast | 4 | 0 |
| Rainy | 2 | 3 | | Hot | 2 | 2 |
| Mild | 4 | 2 | | Cool | 3 | 1 |
| High | 3 | 4 | | Normal | 6 | 1 |
| Windy = T | 3 | 3 | | Windy = F | 6 | 2 |

**(a)** Classify $X_1=(\text{Sunny},\text{Hot},\text{Normal},\text{F})$.
**(b)** Classify $X_2=(\text{Overcast},\text{Cool},\text{High},\text{T})$ without smoothing. What goes wrong?
**(c)** Fix (b) with Laplace smoothing $\alpha=1$ (each attribute's $K$: Outlook 3, Temp 3, Humidity 2, Windy 2).
**(d)** Why compute in log space?

**Solution.**

**(a)**
$$\text{Yes: }\tfrac39\cdot\tfrac29\cdot\tfrac69\cdot\tfrac69\cdot\tfrac9{14}=0.02116\qquad\text{No: }\tfrac25\cdot\tfrac25\cdot\tfrac15\cdot\tfrac25\cdot\tfrac5{14}=0.00457$$
Normalise: $P(\text{Yes})=\dfrac{0.02116}{0.02116+0.00457}=\mathbf{0.822}$, $P(\text{No})=0.178$ → **Yes**.

> If your class uses the *standard textbook* version of this table (Sunny = 2 Yes / 3 No), the numbers become $0.0141$ vs $0.0069$ → $0.67$ / $0.33$ (still **Yes**). Count from whichever table the question gives you.

**(b)**
$$\text{No: }P(\text{Overcast}\mid\text{No})=\tfrac05=0\ \Rightarrow\ \text{score}(\text{No})=\mathbf{0}\ \text{(zero-probability problem)}$$
$$\text{Yes: }\tfrac49\cdot\tfrac39\cdot\tfrac39\cdot\tfrac39\cdot\tfrac9{14}=0.01058$$
The zero wipes out the "No" class *regardless of the other three attributes* — the classifier is overconfident (it says Yes with probability 1).

**(c)** $P(v\mid C)=\dfrac{\text{count}+1}{N_C+K}$:
$$\text{Yes: }\tfrac{5}{12}\cdot\tfrac{4}{12}\cdot\tfrac{4}{11}\cdot\tfrac{4}{11}\cdot\tfrac9{14}=0.01181\qquad
\text{No: }\tfrac{1}{8}\cdot\tfrac{2}{8}\cdot\tfrac{5}{7}\cdot\tfrac{4}{7}\cdot\tfrac5{14}=0.00456$$
$P(\text{Yes})=\dfrac{0.01181}{0.01181+0.00456}=\mathbf{0.722}$ → **Yes**, now with a sensible confidence. (Check of part (a) with $\alpha=1$: Yes 0.02169, No 0.00615 → 0.779.)

**(d)** Products of many numbers $<1$ underflow. Use $\ln$: for (a), $\ln0.02116=-3.855$ vs $\ln0.00457=-5.388$ → the larger log-score (Yes) wins; the argmax is unchanged because $\ln$ is monotonic.

---

## Problem 3 ★ — Bayes' theorem and the Bayes decision rule

**(a) Spam filter.** 20 % of mail is spam. The word "free" appears in 60 % of spam and 5 % of non-spam. An email contains "free". $P(\text{spam}\mid\text{free})=?$

$$P(\text{free})=0.6(0.2)+0.05(0.8)=0.16,\qquad P(\text{spam}\mid\text{free})=\frac{0.6\times0.2}{0.16}=\mathbf{0.75}$$

**(b) Medical test (from your notes, as a check).** Prevalence 0.1 %, sensitivity 99 %, specificity 99 %:
$$P(D\mid+)=\frac{0.99(0.001)}{0.99(0.001)+0.01(0.999)}=\frac{0.00099}{0.01098}\approx\mathbf{0.090}$$

**(c) Bayes decision between two Gaussians.** $\omega_1:\mathcal N(2,1)$ with $P(\omega_1)=0.7$; $\omega_2:\mathcal N(5,1)$ with $P(\omega_2)=0.3$. Find the decision boundary and classify $x=3.5$ and $x=4$.

For equal variances $\sigma^2$, setting $p(x\mid\omega_1)P(\omega_1)=p(x\mid\omega_2)P(\omega_2)$ gives a single threshold

$$x^{*}=\frac{\mu_1+\mu_2}{2}+\frac{\sigma^{2}}{\mu_2-\mu_1}\ln\frac{P(\omega_1)}{P(\omega_2)}=3.5+\frac13\ln\frac{0.7}{0.3}=3.5+0.282=\mathbf{3.78}$$

Decide $\omega_1$ if $x<3.78$, else $\omega_2$.
- $x=3.5$ (midpoint, equal likelihoods): $P(\omega_1\mid x)=\mathbf{0.70}$ (equals the prior) → $\omega_1$.
- $x=4>3.78$ → $\omega_2$.

The larger prior of $\omega_1$ **shifts the boundary toward the less probable class** ($3.5\to3.78$).

---

## Problem 4 — k-Nearest Neighbour (and why $k$ matters)

Training: class A: $(1,1),(2,1),(2,3)$; class B: $(5,5),(6,4),(7,6)$. Query $x_q=(3,4)$.

| Point | Class | Distance to $x_q$ |
|---|---|---|
| (2,3) | A | $\sqrt2=1.414$ |
| (5,5) | B | $\sqrt5=2.236$ |
| (6,4) | B | $3.000$ |
| (2,1) | A | $\sqrt{10}=3.162$ |
| (1,1) | A | $\sqrt{13}=3.606$ |
| (7,6) | B | $\sqrt{20}=4.472$ |

- $k=1$ → nearest is A → **A**.
- $k=3$ → A, B, B → **B** (2 vs 1).
- $k=5$ → A, B, B, A, A → **A** (3 vs 2).
- **Distance-weighted** vote, $w=1/d^2$, $k=3$: A $=1/2=0.500$; B $=1/5+1/9=0.311$ → **A**.

**Lesson:** the answer depends on $k$; a small $k$ is noise-sensitive, a large $k$ blurs the boundary. Use cross-validation to choose $k$, and **scale features first** (distance-based).

---

## Problem 5 — SVM: find $\mathbf w$, $b$, margin; classify

Only two support vectors: $x_1=(1,1)$ with $t=-1$ and $x_2=(3,3)$ with $t=+1$.

The hyperplane is perpendicular to $x_2-x_1=(2,2)$, so $\mathbf w=c\,(2,2)$. Margin constraints $\mathbf w^T x_2+b=+1$ and $\mathbf w^T x_1+b=-1$ subtract to $\mathbf w^T(x_2-x_1)=2$:
$$c(4+4)=2\ \Rightarrow\ c=\tfrac14\ \Rightarrow\ \mathbf w=(0.5,\,0.5)$$
$$b=1-\mathbf w^T x_2=1-3=-2$$
$$\text{margin}=\frac{2}{\lVert\mathbf w\rVert}=\frac{2}{0.7071}=\mathbf{2.83}\ (=\text{distance between the two points})$$

Decision function $f(x)=0.5x_1+0.5x_2-2$.
- $(4,1)$: $f=0.5(5)-2=+0.5$ → **class +1**.
- $(1,2)$: $f=-0.5$ → **class −1**.
- Hinge loss of $(2,2.5)$ with $t=+1$: $f=0.25$, so $\max(0,\,1-0.25)=\mathbf{0.75}$ (inside the margin → penalised).

---

## Problem 6 — Association-rule metrics (rule induction)

Six transactions: {bread, milk}, {bread, butter}, {milk, butter}, {bread, milk, butter}, {bread}, {milk}. Evaluate the rule **bread ⇒ milk**.

- $\text{support}(\text{bread,milk})=2/6=0.333$
- $P(\text{bread})=4/6$, $P(\text{milk})=4/6$
- $\text{confidence}=P(\text{milk}\mid\text{bread})=\dfrac{2/6}{4/6}=\mathbf{0.5}$
- $\text{lift}=\dfrac{0.5}{4/6}=\mathbf{0.75}<1$ → bread and milk are **negatively associated**: a weak/misleading rule despite 50 % confidence.

---

## Problem 7 — Genetic algorithm: one generation

Maximise $f(x)=x^2$ for $x\in[0,31]$ with 5-bit chromosomes. Population: `01101`, `11000`, `01000`, `10011`.

| Chromosome | $x$ | $f=x^2$ | Selection prob. $f/\sum f$ | Expected copies (pop = 4) |
|---|---|---|---|---|
| 01101 | 13 | 169 | 0.144 | 0.58 |
| 11000 | 24 | 576 | 0.492 | 1.97 |
| 01000 | 8 | 64 | 0.055 | 0.22 |
| 10011 | 19 | 361 | 0.309 | 1.23 |
| **Sum / Avg / Max** | | **1170 / 292.5 / 576** | 1.000 | 4.0 |

*(Roulette-wheel selection: the fittest, `11000`, is picked most often; `01000` is likely to die out.)*

**Crossover** (one-point after bit 4) of `01101` and `11000`:
`0110|1` × `1100|0` → offspring **`01100`** ($x=12$, $f=144$) and **`11001`** ($x=25$, $f=625$).
**Mutation** (flip bit 3 of `11001`) → `11101` ($x=29$, $f=841$).
Fitness rises toward the optimum `11111` ($x=31$, $f=961$) over generations. (Terminate on a fixed number of generations, a fitness threshold, or no improvement.)

---

## Problem 8 — A single neuron: forward pass and one backprop step

Inputs $\mathbf x=(1,2)$, weights $\mathbf w=(0.5,-0.3)$, bias $b=0.1$, sigmoid activation, target $t=1$, loss $L=\tfrac12(t-a)^2$, learning rate $\eta=0.5$.

**Forward:** $z=0.5(1)-0.3(2)+0.1=0$ → $a=\sigma(0)=0.5$ → $L=\tfrac12(0.5)^2=0.125$.

**Backward** (chain rule): $\dfrac{\partial L}{\partial z}=-(t-a)\,a(1-a)=-(0.5)(0.25)=-0.125$
$$\frac{\partial L}{\partial \mathbf w}=-0.125\,\mathbf x=(-0.125,\,-0.25),\qquad \frac{\partial L}{\partial b}=-0.125$$

**Update:** $\mathbf w\leftarrow\mathbf w-\eta\nabla=(0.5625,\,-0.175)$, $b\leftarrow0.1625$.
**Check:** new $a=0.593$, new $L=0.083<0.125$ ✓ (the loss decreased).

---

# UNIT 2

## Problem 9 ★ — Polynomial least squares and overfitting

Data: $(0,1),\ (1,3),\ (2,4),\ (3,8)$.

**(a) Fit a straight line $y=a_0+a_1x$** with the normal equations $\Phi^T\Phi\,\mathbf a=\Phi^T\mathbf y$:
$$\Phi^T\Phi=\begin{bmatrix}4&6\\6&14\end{bmatrix},\qquad \Phi^T\mathbf y=\begin{bmatrix}16\\35\end{bmatrix}$$
$$\det=56-36=20,\quad a_0=\frac{14(16)-6(35)}{20}=0.7,\quad a_1=\frac{4(35)-6(16)}{20}=2.2$$
$$\boxed{\hat y=0.7+2.2x},\qquad E=\sum(y_i-\hat y_i)^2=0.09+0.01+1.21+0.49=\mathbf{1.8}$$

**(b) Degree 2:** $\hat y=1.2+0.7x+0.5x^2$, $E=\mathbf{0.8}$.
**(c) Degree 3** (4 points, 4 coefficients): passes through *every* point, $E=\mathbf0$: $\hat y=1+3.83x-2.5x^2+0.667x^3$ — **perfect training fit, but it oscillates** (overfitting; training error is a poor guide).
**(d) Ridge** on the line (penalise only the slope, $\lambda=1$): solve $\begin{bmatrix}4&6\\6&15\end{bmatrix}\mathbf a=\begin{bmatrix}16\\35\end{bmatrix}$ → $a_0=1.25$, $a_1=\mathbf{1.83}<2.2$ (**slope shrunk**).
**(e) Why not a high degree in many variables?** The number of monomials of degree $\le M$ in $D$ variables is $\binom{D+M}{M}$. For $M=3$: $D=2\to10$, $D=10\to286$, $D=100\to176{,}851$ coefficients — the curse of dimensionality.

---

## Problem 10 ★ — Fisher's linear discriminant

Class 1: $(1,2),(2,3),(3,3),(4,5),(5,5)$. Class 2: $(4,2),(5,0),(5,2),(3,2),(5,3),(6,3)$.

**Step 1 — means:** $\mathbf m_1=(3,\ 3.6)$, $\mathbf m_2=(4.667,\ 2)$.

**Step 2 — scatter matrices:**
$$S_1=\begin{bmatrix}10&8\\8&7.2\end{bmatrix},\quad S_2=\begin{bmatrix}5.333&1\\1&6\end{bmatrix},\quad S_W=S_1+S_2=\begin{bmatrix}15.333&9\\9&13.2\end{bmatrix}$$

**Step 3 — inverse:** $\det S_W=15.333(13.2)-81=121.4$,
$$S_W^{-1}=\frac1{121.4}\begin{bmatrix}13.2&-9\\-9&15.333\end{bmatrix}=\begin{bmatrix}0.1087&-0.0741\\-0.0741&0.1263\end{bmatrix}$$

**Step 4 — direction:**
$$\mathbf w\propto S_W^{-1}(\mathbf m_1-\mathbf m_2)=S_W^{-1}\begin{bmatrix}-1.667\\1.6\end{bmatrix}=\begin{bmatrix}-0.300\\0.326\end{bmatrix}\ \ (\text{normalised: }(-0.677,\,0.736))$$

**Step 5 — project and classify.** Projected class means: $\mathbf w^T\mathbf m_1=+0.273$, $\mathbf w^T\mathbf m_2=-0.748$; threshold (midpoint) $=-0.238$.
- Class 1 projections: $0.35,\,0.38,\,0.08,\,0.43,\,0.13$ — **all above** the threshold.
- Class 2 projections: $-0.55,\,-1.50,\,-0.85,\,-0.25,\,-0.52,\,-0.82$ — **all below** it.
→ perfectly separated. Fisher criterion value $J=\dfrac{(\mathbf w^T(\mathbf m_1-\mathbf m_2))^2}{\mathbf w^TS_W\mathbf w}=1.02$.

*(Using $\mathbf m_2-\mathbf m_1$ flips the sign of $\mathbf w$ — same line, same separation.)*

---

## Problem 11 — Maximum likelihood and sequential estimation

**(a)** Data: 2, 4, 4, 4, 5, 5, 7, 9. Gaussian MLE:
$$\hat\mu=\frac{40}{8}=\mathbf5,\qquad \hat\sigma^2_{ML}=\frac{9+1+1+1+0+0+4+16}{8}=\frac{32}{8}=\mathbf4$$
(The unbiased estimate divides by $N-1$: $32/7=4.571$; the ML estimate is biased low.)

**(b) Sequential mean** for the stream 4, 6, 8, 3 using $\mu_N=\mu_{N-1}+\tfrac1N(x_N-\mu_{N-1})$:

| $N$ | $x_N$ | $\mu_N$ |
|---|---|---|
| 1 | 4 | 4 |
| 2 | 6 | $4+\frac12(2)=5$ |
| 3 | 8 | $5+\frac13(3)=6$ |
| 4 | 3 | $6+\frac14(-3)=\mathbf{5.25}$ |

(Equals the batch mean $21/4=5.25$, but without storing old data.)

---

## Problem 12 — Kalman filter (scalar)

State model $x_k=x_{k-1}+w$ ($A=1$), measurement $z_k=x_k+v$ ($H=1$), $Q=0.1$, $R=0.5$; start $\hat x_0=0$, $P_0=1$. Measurements $z_1=1.2$, $z_2=0.9$.

**Step 1.** Predict: $\hat x^-=0$, $P^-=P_0+Q=1.1$.
Gain: $K_1=\dfrac{P^-}{P^-+R}=\dfrac{1.1}{1.6}=0.6875$.
Update: $\hat x_1=0+0.6875(1.2-0)=\mathbf{0.825}$; $P_1=(1-K_1)P^-=\mathbf{0.344}$.

**Step 2.** Predict: $P^-=0.344+0.1=0.444$. $K_2=\dfrac{0.444}{0.944}=0.470$.
Update: $\hat x_2=0.825+0.470(0.9-0.825)=\mathbf{0.860}$; $P_2=\mathbf{0.235}$.

**Reading it:** the uncertainty $P$ shrinks as measurements arrive, and the gain $K$ drops (we trust the model more and each new measurement less). If a measurement is missing (occlusion), skip the update step: $P$ keeps growing.

---

## Problem 13 — Metrics, scaling, and counting

**(a) Confusion matrix:** TP = 40, FP = 10, FN = 20, TN = 30.
$$\text{Precision}=\tfrac{40}{50}=0.80,\quad \text{Recall}=\tfrac{40}{60}=0.667,\quad F_1=\tfrac{2(0.8)(0.667)}{0.8+0.667}=0.727,\quad \text{Accuracy}=\tfrac{70}{100}=0.70$$

**(b) Scaling** the feature values $10,20,30,40,100$:
- Min–max: $\frac{x-10}{90}$ → $0,\ 0.111,\ 0.222,\ 0.333,\ \mathbf1$.
- z-score: $\mu=40$, $\sigma=31.62$ (population) → $-0.949,\ -0.632,\ -0.316,\ 0,\ \mathbf{1.897}$.
The outlier (100) squashes the others under min–max scaling but not as badly under z-scoring.

**(c) Entropy checks:** 3 equally likely classes → $\log_2 3=1.585$ bits; a 50/50 split → 1 bit; a 3:1 split → 0.811 bit.

---

# Answer-writing tips for numericals

1. **Write the formula first**, then substitute numbers, then state the result with units/interpretation (class, decision, direction).
2. **Count from the table you are given**; if a textbook version differs, say so.
3. **Show intermediate tables** (counts, class means, scatter matrices): partial credit is awarded for method.
4. For Naïve Bayes always **check for zeros** and apply Laplace if any count is 0; state $\alpha$ and $K$.
5. For information gain, use $\log_2$ and keep 3 decimals; a pure branch has entropy 0.
6. For Fisher/LDA, **only the direction of $\mathbf w$ matters** — don't worry about its length or sign.
7. For least squares, write $\Phi^T\Phi\,\mathbf a=\Phi^T\mathbf y$ explicitly, solve the small system, and compute $E$ to compare model orders.
8. End with a **one-line conclusion** ("therefore predict Yes", "therefore degree 3 overfits").

*Good luck with the exam!*
