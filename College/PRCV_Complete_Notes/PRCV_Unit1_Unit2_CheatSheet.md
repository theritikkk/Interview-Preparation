# PRCV (ML-411T) — Units 1 & 2: Last-Minute Cheat Sheet + Rapid-Fire Q&A

> Read this the night before / morning of the exam. Full explanations are in the notes; this file is the **compressed version**.
> Part A = one-glance summary · Part B = 48 rapid-fire questions (cover the answers and test yourself) · Part C = traps that lose marks.

---

# PART A — ONE-GLANCE SUMMARY

## UNIT 1 — Induction algorithms

| Topic | Remember |
|---|---|
| **Induction** | General rule from limited data; a *leap beyond the data*, never proven. Needs representative sampling + uniformity of nature. |
| **Rule induction** | IF–THEN rules; from trees (root→leaf path), sequential covering, Apriori. Metrics: support $=P(A\cap B)$, confidence $=P(B\mid A)$, lift $=\dfrac{P(B\mid A)}{P(B)}$. Readable but weak on non-linear/high-dim data. |
| **Decision tree** | Recursive partitioning. $H=-\sum p_i\log_2p_i$ (pure = 0, 50/50 = 1). Gain $=H(S)-\sum\frac{\lvert S_v\rvert}{\lvert S\rvert}H(S_v)$. **ID3**: gain · **C4.5**: gain ratio · **CART**: Gini $1-\sum p_i^2$ (regression: variance). Overfits → prune. |
| **Naïve Bayes** | $\hat y=\arg\max_y P(y)\prod_iP(x_i\mid y)$. Assumes features **conditionally independent given class**. Learning = counting. Numeric attr → Gaussian per class ($\mu_c,\sigma_c^2$). |
| **Zero probability** | Fix with **Laplace**: $\dfrac{\text{count}+\alpha}{N_C+\alpha K}$ (α = 1 default). Large α → uniform. α trades bias vs variance. |
| **Underflow** | Use logs: $\log P(y)+\sum\log P(x_i\mid y)$. |
| **Bayes decision** | Decide $\omega_1$ if $p(x\mid\omega_1)P(\omega_1)>p(x\mid\omega_2)P(\omega_2)$. Dichotomizer $g(x)=\ln\frac{p(x\mid\omega_1)}{p(x\mid\omega_2)}+\ln\frac{P(\omega_1)}{P(\omega_2)}$; decide $\omega_1$ if $g>0$. |
| **Neural net** | $z=\sum w_ix_i+b$, $a=f(z)$. Forward pass → loss (MSE/CE) → backprop (chain rule) → $w\leftarrow w-\eta\,\partial L/\partial w$. Non-linear activation (sigmoid/tanh/ReLU) is essential. Paradigms: supervised / unsupervised / reinforcement. |
| **Genetic algorithm** | Population → fitness → selection → crossover → mutation → survivor selection → repeat. Terms: chromosome (solution), gene (position), allele (value), genotype (encoded) vs phenotype (real), fitness function. Beats gradient methods on multi-peaked (local-optima) landscapes; stochastic, no optimality guarantee. |
| **Instance-based** | Lazy/memory-based; store data, compute at query time, $O(n)$ per query. **k-NN**: vote (discrete) or mean (real). Fooled by irrelevant attributes. **LWR**: local weighted fit, kernel $K(d)$. **RBF net**: $\hat f=w_0+\sum w_uK_u$, $K_u=e^{-d^2/2\sigma^2}$. **CBR**: symbolic cases, new distance metrics. |
| **SVM** | Maximise margin $=2/\lVert w\rVert$; hyperplane $w^Tx+b=0$; support vectors = closest points. Hard: $\min\frac12\lVert w\rVert^2$ s.t. $t_i(w^Tx_i+b)\ge1$. Soft: $+C\sum\zeta_i$ (large C → narrow margin). Kernels: linear, polynomial, **RBF** $e^{-\gamma\lVert x-z\rVert^2}$, sigmoid. Dual: αᵢ > 0 ⇒ support vector. |

## UNIT 2 — Statistical pattern recognition

| Topic | Remember |
|---|---|
| **SPR pipeline** | data → pre-process → feature extraction/selection → model → evaluate. |
| **Classification vs regression** | categorical label (accuracy, precision, recall, F1) vs continuous value (MSE, RMSE, $R^2$). Precision $=\frac{TP}{TP+FP}$, recall $=\frac{TP}{TP+FN}$, $F_1=\frac{2PR}{P+R}$. |
| **Feature / feature vector / classifier** | one measurable property / the array describing one sample / function mapping a vector to a class. |
| **Pre-processing** | cleaning (impute, dedupe), min–max $\frac{x-x_{\min}}{x_{\max}-x_{\min}}$, z-score $\frac{x-\mu}{\sigma}$, one-hot, PCA. |
| **Curse of dimensionality** (Bellman 1961) | volume grows exponentially → **sparsity**, high compute, **distances concentrate**, overfitting, hard to visualise. Cures: PCA/LDA/t-SNE, feature selection, L1/L2, ensembles, more data. |
| **Polynomial fitting** | $y=\sum_{j=0}^na_jx^j$; minimise $E=\sum(y_i-\hat y_i)^2$; normal eq. $\mathbf a=(\Phi^T\Phi)^{-1}\Phi^T\mathbf y$. Low degree underfits, high degree overfits (degree $N-1$ fits exactly). |
| **Bias–variance** | Error $=\text{Bias}^2+\text{Variance}+\text{noise}$. Simple = high bias; complex = high variance. |
| **Regularisation** | Ridge L2: $+\frac\lambda2\sum w^2$ (shrinks). Lasso L1: $+\lambda\sum\lvert w\rvert$ (sparse, exact zeros). Elastic Net = both. |
| **Cross-validation** | k-fold (train on $k-1$, validate on 1, average); LOOCV $k=N$. Used to pick degree/λ. |
| **Multivariate non-linear** | polynomial with interactions, neural nets, kernel SVM. Issues: overfitting, cost, interpretability. |
| **Bayes' theorem** | Posterior $=\frac{\text{likelihood}\times\text{prior}}{\text{evidence}}$. Base rate matters (0.1 % disease, 99 %/99 % test → $P(D\mid+)\approx9\%$). |
| **Decision boundary** | hypersurface where discriminants tie. Linear (logistic, linear SVM, LDA) vs non-linear (trees, NN, kernel SVM). Equal-covariance Gaussians → linear; unequal → quadratic. |
| **Parametric methods / MLE** | assume a distribution with fixed parameters. MLE: maximise likelihood; Gaussian $\hat\mu=\bar x$, $\hat\sigma^2=\frac1N\sum(x-\hat\mu)^2$ (÷$N-1$ unbiased). Bayesian: $p(\theta\mid D)\propto p(D\mid\theta)p(\theta)$. |
| **Sequential estimation** | $\mu_N=\mu_{N-1}+\frac1N(x_N-\mu_{N-1})$. Methods: RLS, **Kalman** (predict → update), SGD ($\theta\leftarrow\theta-\eta\nabla L$). |
| **Kalman** | Predict: $\hat x^-=A\hat x+Bu$, $P^-=APA^T+Q$. Update: $K=P^-H^T(HP^-H^T+R)^{-1}$, $\hat x=\hat x^-+K(z-H\hat x^-)$, $P=(I-KH)P^-$. |
| **Linear discriminant** | $g(\mathbf x)=\mathbf w^T\mathbf x+w_0$; $g>0\Rightarrow\omega_1$; $\mathbf w\perp$ boundary; distance $=g(\mathbf x)/\lVert\mathbf w\rVert$. |
| **Fisher's LD** | $J(\mathbf w)=\dfrac{\mathbf w^TS_B\mathbf w}{\mathbf w^TS_W\mathbf w}$; two-class: $\mathbf w\propto S_W^{-1}(\mathbf m_2-\mathbf m_1)$; $S_W=\sum\sum(\mathbf x-\boldsymbol\mu_k)(\mathbf x-\boldsymbol\mu_k)^T$; $S_B=\sum N_k(\boldsymbol\mu_k-\boldsymbol\mu)(\cdot)^T$. At most $C-1$ dimensions. **Supervised** (PCA is not). |
| **Feed-forward net** | layers input → hidden → output, no cycles. $z_j=\sum w_{ij}x_i+b_j$, $a_j=\sigma(z_j)$, $y=f(x;\theta)$. Backprop + SGD. Universal approximator; overfits → regularise/dropout/early stop. |

---

# PART B — RAPID-FIRE Q&A (cover the answers!)

### Unit 1

1. **What is induction, and why can it never be proved?** Inferring a general rule from limited observations; it always goes beyond the data, so a new observation can still contradict it.
2. **Name the two assumptions of scientific induction.** Representative sampling; uniformity of nature.
3. **Give the three rule-quality metrics.** Support, confidence, lift.
4. **How do you get rules from a decision tree?** Trace each root-to-leaf path; the conditions form the IF-part, the leaf is the THEN-part.
5. **Entropy of a 50/50 binary node? Of a pure node?** 1; 0.
6. **Define information gain.** Entropy of the parent minus the weighted entropy of the children (entropy reduction).
7. **ID3 vs C4.5 vs CART?** Information gain (categorical) · gain ratio (continuous, missing values, pruning) · Gini for classification / variance reduction for regression (binary trees).
8. **Why does C4.5 use gain ratio?** Information gain is biased toward attributes with many values.
9. **Why prune a tree?** To reduce overfitting.
10. **What is the "naïve" assumption?** Features are conditionally independent given the class.
11. **Why is Naïve Bayes fast?** Training is just counting; prediction multiplies a few probabilities.
12. **How are numeric attributes handled?** Fit a Gaussian (mean, variance) per class; use its density as $P(x\mid c)$.
13. **What is the zero-probability problem and its fix?** An unseen attribute value gives probability 0 and kills the product; use Laplace smoothing $(\text{count}+\alpha)/(N+\alpha K)$.
14. **Effect of a very large α?** Estimates drift to the uniform value $1/K$; data matter less (high bias).
15. **Why add logs in Naïve Bayes?** The product of many small probabilities underflows; sums of logs do not.
16. **State the Bayes minimum-error rule.** Choose the class with the largest posterior, equivalently the largest $p(x\mid\omega_i)P(\omega_i)$.
17. **What is a dichotomizer?** A two-class classifier using one function $g(x)=g_1-g_2$; class decided by the sign of $g$.
18. **Why are non-linear activations needed in a neural network?** Without them the whole network collapses into a single linear map.
19. **What does backpropagation do?** Uses the chain rule to compute the gradient of the loss w.r.t. every weight, then updates $w\leftarrow w-\eta\,\partial L/\partial w$.
20. **GA: chromosome / gene / allele?** One candidate solution / one position in it / the value at that position.
21. **GA: genotype vs phenotype?** Encoded representation (e.g., bit string) vs the real-world solution it decodes to.
22. **Why use a GA instead of gradient descent?** Gradient methods get stuck in local optima on multi-peaked landscapes and need derivatives; GAs search globally without derivatives.
23. **Why is k-NN called "lazy"?** It does no training; all computation is deferred to query time (cost $O(n)$ per query).
24. **Weakness of k-NN?** Slow queries, large memory, fooled by irrelevant attributes / high dimensions.
25. **Locally weighted regression in one line?** Fit a model only near the query, weighting training points by a distance kernel.
26. **What is an RBF network's hidden unit?** A Gaussian centred on a point $x_u$, active only for inputs near $x_u$; the output is a linear combination.
27. **SVM objective and margin?** Maximise margin $2/\lVert w\rVert$, i.e. minimise $\frac12\lVert w\rVert^2$ subject to $t_i(w^Tx_i+b)\ge1$.
28. **What are support vectors?** The training points closest to the hyperplane (αᵢ > 0); they alone define it.
29. **Role of C?** Weights the slack penalty: large C → narrow margin, few violations; small C → wide margin, more violations.
30. **What is the kernel trick?** Compute inner products in a high-dimensional feature space via $K(x,z)$ without mapping explicitly, making non-linear problems linearly separable.

### Unit 2

31. **Classification vs regression?** Categorical output vs continuous output.
32. **Write precision, recall, F1.** $\frac{TP}{TP+FP}$, $\frac{TP}{TP+FN}$, $\frac{2PR}{P+R}$.
33. **Feature vs feature vector?** One measurable property vs the array of all features of one sample.
34. **Min–max normalisation vs standardisation?** Rescale to $[0,1]$ vs rescale to mean 0, std 1.
35. **Name four effects of the curse of dimensionality.** Sparsity, computational cost, distance concentration, overfitting (also: hard visualisation).
36. **Name four remedies.** PCA/LDA, feature selection, regularisation, more data/ensembles.
37. **Why can a polynomial of degree $N-1$ fit $N$ points perfectly yet be useless?** It fits the noise (overfitting); it oscillates between points and generalises poorly.
38. **Write the least-squares normal equations.** $\Phi^T\Phi\,\mathbf a=\Phi^T\mathbf y$.
39. **Bias–variance in one sentence.** Simple models have high bias and low variance; complex models have low bias and high variance; the best model balances them.
40. **L1 vs L2 regularisation?** L1 (lasso) gives sparse weights with exact zeros; L2 (ridge) shrinks weights smoothly.
41. **Explain k-fold CV.** Split into $k$ folds; train on $k-1$, validate on the remaining one; repeat $k$ times and average.
42. **Base-rate example?** With 0.1 % prevalence and a 99 %/99 % test, a positive result means only ≈ 9 % chance of disease.
43. **Linear vs non-linear decision boundary?** Hyperplane (logistic, linear SVM, LDA) vs curved (trees, NN, kernel SVM).
44. **Parametric vs non-parametric?** Fixed number of parameters for an assumed distribution vs complexity that grows with the data (k-NN, Parzen).
45. **Fisher's criterion and solution?** Maximise between-class over within-class scatter; two-class $\mathbf w\propto S_W^{-1}(\mathbf m_2-\mathbf m_1)$; at most $C-1$ useful directions.

### Bonus (often asked)

46. **Sequential mean update?** $\mu_N=\mu_{N-1}+\frac1N(x_N-\mu_{N-1})$.
47. **Kalman filter in two steps?** Predict state and covariance from the model; update using the Kalman gain and the new measurement.
48. **Why can a feed-forward network overfit, and cures?** Many parameters; use regularisation, dropout, early stopping, more data.

---

# PART C — TRAPS THAT LOSE MARKS

1. **Naïve Bayes numericals:** forgetting to check for a zero count; forgetting to **normalise** when a probability is asked (the unnormalised scores are not probabilities); counting from the wrong table.
2. **Entropy:** using natural log instead of $\log_2$; forgetting the weights $\lvert S_v\rvert/\lvert S\rvert$.
3. **SVM:** margin is $2/\lVert w\rVert$ (not $1/\lVert w\rVert$); large C means *narrow* margin.
4. **Laplace smoothing:** the denominator needs $+\alpha K$ (not just $+\alpha$).
5. **Fisher:** it is **supervised**; PCA is not. $S_W$ uses *class* means, $S_B$ uses the overall mean. Only the direction of $\mathbf w$ matters.
6. **Overfitting:** zero training error is a warning sign, not success — always mention test/validation error.
7. **Lasso vs ridge:** exact zeros come from L1, not L2.
8. **Genetic algorithms:** state the terminology and the loop; mention "no guarantee of optimality".
9. **Bayes' theorem:** don't confuse $P(D\mid+)$ with $P(+\mid D)$ (sensitivity).
10. **k-NN:** scale the features first; $k$ must be chosen (cross-validation); ties for even $k$.
11. **Always finish** a numerical with a one-line conclusion (class / decision / direction).

*Good luck — you've got this.*
