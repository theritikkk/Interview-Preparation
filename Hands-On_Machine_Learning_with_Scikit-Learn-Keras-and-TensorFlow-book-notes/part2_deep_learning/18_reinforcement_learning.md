# Chapter 18 — Reinforcement Learning

## 🎯 Why this chapter matters
Every previous chapter learned from a fixed dataset. **Reinforcement Learning (RL)** is different: an **agent** learns by *acting* in an **environment**, receiving **rewards** (or penalties), and gradually discovering which behaviors pay off — through trial and error, not labeled examples. This is how AlphaGo learned to beat world champions and how game-playing/robotics AI works.

---

## 🧠 Learning to Optimize Rewards

**Core loop:** the agent observes the environment → takes an action → receives a reward → repeat. Its goal: learn a **policy** (its strategy for choosing actions) that **maximizes cumulative reward over time**. Examples: a robot getting reward for approaching a target; a thermostat rewarded for saving energy while staying near the target temperature; a trading bot rewarded for profit.

### Policy Search — how do you find a good policy?
- **Brute force / genetic algorithms**: try many candidate policies (or "evolve" a population of them, keeping the best performers and mutating them), and keep whatever works best. Simple, but the space of possible policies is usually far too vast to search this way efficiently.
- **Policy Gradients (PG)**: treat this as an optimization problem — evaluate the *gradient* of expected rewards with respect to the policy's parameters, and nudge the parameters in the direction that increases reward, just like Gradient Descent. This is the practical, scalable approach — covered in depth below.

A policy can be **stochastic** (involves randomness — e.g., a robot vacuum that moves forward with probability $p$) rather than a fixed deterministic rule; randomness itself can be *useful* for exploring the environment.

---

## 🎮 OpenAI Gym — a standard playground for RL experiments

`gym` provides many ready-made environments (CartPole, Atari games, robotics simulations) with a standard interface: `env.reset()`, `env.step(action)` → `(observation, reward, done, info)`, `env.render()`. This lets researchers/practitioners benchmark algorithms consistently without building a simulator from scratch each time.

---

## 🕹️ Policy Gradients — REINFORCE algorithm

A simple, foundational PG algorithm (Williams, 1992):

1. Let the current neural-network policy **play several episodes**. At each step, compute (but don't yet apply) the gradient that would make the action *just taken* more likely.
2. After the episodes finish, compute each action's **advantage** — a measure, in hindsight, of how much better or worse than average that action's outcome was. This uses **discounted rewards**: future rewards count less the further away they are, controlled by a **discount factor** $\gamma$ (e.g., 0.95) — then these discounted returns are **normalized** (subtract the mean, divide by the standard deviation) across all sampled actions.
3. **Scale each saved gradient by its action's advantage**: good actions (positive advantage) get their gradient applied as-is (reinforcing them); bad actions (negative advantage) get the *opposite* gradient applied (discouraging them).
4. Average all the scaled gradients and take one Gradient Descent step.

```python
def play_one_step(env, obs, model, loss_fn):
    with tf.GradientTape() as tape:
        left_proba = model(obs[np.newaxis])
        action = (tf.random.uniform([1, 1]) > left_proba)
        y_target = tf.constant([[1.]]) - tf.cast(action, tf.float32)
        loss = tf.reduce_mean(loss_fn(y_target, left_proba))
    grads = tape.gradient(loss, model.trainable_variables)   # saved, not yet applied
    obs, reward, done, info = env.step(int(action[0, 0].numpy()))
    return obs, reward, done, grads

def discount_rewards(rewards, discount_factor):
    discounted = np.array(rewards, dtype=float)
    for step in range(len(rewards) - 2, -1, -1):
        discounted[step] += discounted[step + 1] * discount_factor
    return discounted
```
After several episodes, gradients get scaled by each action's normalized advantage and averaged before an `optimizer.apply_gradients()` call — over ~150 iterations, this successfully teaches a policy to balance CartPole's pole.

⚠️ **PG is sample-inefficient** — it needs to play many full episodes just to estimate each action's advantage, making it slow on complex tasks. It's foundational, but more efficient algorithms (below) build on top of it.

---

## 🧩 Markov Decision Processes (MDPs) — the mathematical foundation

An MDP extends a **Markov chain** (a memoryless random process — the next state depends only on the current state, not the history) by adding: at each state, the agent **chooses an action**, and different actions lead to different transition probabilities *and* different rewards.

**Bellman Optimality Equation**: the optimal value $V^*(s)$ of a state (the expected sum of all future discounted rewards, if the agent acts optimally from here on) satisfies:

$$
V^*(s) = \max_a \sum_{s'} T(s,a,s')\left[R(s,a,s') + \gamma \cdot V^*(s')\right]
$$

This leads directly to the **Value Iteration** algorithm: initialize all state values to 0, then repeatedly apply this equation to every state — guaranteed to converge to the true optimal values, given enough iterations (an example of **Dynamic Programming**).

**Q-Values (state-action values)**: a very similar iterative algorithm estimates $Q^*(s,a)$ — the expected discounted future reward of taking action $a$ in state $s$, then acting optimally afterward. Once you have optimal Q-Values, the optimal policy is trivial: **always pick the action with the highest Q-Value** in the current state.

---

## 🎯 Q-Learning — when you don't know the environment's dynamics in advance

Value/Q-Value Iteration require knowing the transition probabilities $T$ and rewards $R$ upfront. **Q-Learning** instead *learns* Q-Values purely by **watching an agent act** (even acting randomly!) and updating estimates from observed experience:

$$
Q(s,a) \xleftarrow{\alpha} r + \gamma \cdot \max_{a'} Q(s', a')
$$

("$\xleftarrow{\alpha}$" means a running average update with learning rate $\alpha$.) Remarkably, this converges to the optimal Q-Values even from a **completely random** exploration policy — though it takes far more iterations than Value Iteration (which "cheats" by already knowing the true dynamics).

⚠️ **Q-Learning is off-policy**: the policy being *trained* (always pick the highest Q-Value) differs from the policy being *executed* to explore (which might be random). Compare to Policy Gradients, which is **on-policy** — it explores using the very policy it's improving.

### Exploration policies — random exploration is wasteful
A purely random exploration policy eventually visits everything, but very inefficiently. Better: the **ε-greedy policy** — act randomly with probability $\varepsilon$, greedily (highest current Q-Value) otherwise. Start with $\varepsilon$ high (e.g., 1.0, mostly exploring) and decay it over training (e.g., down to 0.05, mostly exploiting what's been learned).

---

## 🧠 Deep Q-Learning (DQN) — Q-Learning doesn't scale to huge state spaces

For a game like Ms. Pac-Man, the number of possible states vastly exceeds what you could ever store in a table. The solution: approximate $Q(s,a)$ with a **neural network** — a **Deep Q-Network (DQN)** — instead of a lookup table. (DeepMind's 2013 breakthrough showing this works well, without hand-engineered features, kicked off the modern deep RL era.)

**Training target**, straight from the Bellman equation:
$$
Q_{\text{target}}(s,a) = r + \gamma \cdot \max_{a'} Q_\theta(s', a')
$$
Minimize the (squared or Huber) error between the DQN's predicted Q-value and this target.

```python
model = keras.models.Sequential([
    keras.layers.Dense(32, activation="elu", input_shape=[4]),   # e.g., CartPole's 4 observations
    keras.layers.Dense(32, activation="elu"),
    keras.layers.Dense(2),    # one Q-value output per possible action
])

def epsilon_greedy_policy(state, epsilon=0):
    if np.random.rand() < epsilon:
        return np.random.randint(2)
    Q_values = model.predict(state[np.newaxis])
    return np.argmax(Q_values[0])
```

### Replay Buffer — a crucial stabilization trick
Instead of training only on the most recent experience, store **every** (state, action, reward, next_state, done) tuple in a **replay buffer**, and train on **randomly sampled batches** from it. This breaks the strong correlation between consecutive experiences (which would otherwise badly destabilize Gradient Descent) and lets old experiences be reused.

```python
from collections import deque
replay_buffer = deque(maxlen=2000)

def training_step(batch_size):
    experiences = sample_experiences(batch_size)   # random sample from replay_buffer
    states, actions, rewards, next_states, dones = experiences
    next_Q_values = model.predict(next_states)
    max_next_Q_values = np.max(next_Q_values, axis=1)
    target_Q_values = rewards + (1 - dones) * discount_factor * max_next_Q_values
    mask = tf.one_hot(actions, n_outputs)
    with tf.GradientTape() as tape:
        all_Q_values = model(states)
        Q_values = tf.reduce_sum(all_Q_values * mask, axis=1, keepdims=True)
        loss = tf.reduce_mean(loss_fn(target_Q_values, Q_values))
    grads = tape.gradient(loss, model.trainable_variables)
    optimizer.apply_gradients(zip(grads, model.trainable_variables))
```

⚠️ **Catastrophic forgetting**: a hallmark RL frustration — a DQN can appear to be learning beautifully, reach great performance, and then suddenly **collapse** a few episodes later, having "forgotten" what worked. This happens because the learning environment/targets keep shifting under Gradient Descent's feet, and experiences are correlated. Bigger replay buffers and lower learning rates help but don't fully solve it.

> 🔑 **Loss is a poor progress indicator in RL** — loss can go down while the agent performs *worse* (e.g., overfitting to one region of the environment), or go up while the agent performs *better* (e.g., correctly raising previously-underestimated Q-values). **Track actual episode reward, not loss**, to judge real progress.

### Deep Q-Learning variants (stabilize/improve the basic algorithm)
- **Fixed Q-Value Targets**: use a **separate, slowly-updated copy** of the network to compute targets, instead of the same network being trained — avoids a "dog chasing its own tail" instability.
- **Double DQN**: reduces a systematic *overestimation* bias in vanilla DQN's max-based targets by decoupling action *selection* from action *evaluation* using two networks.
- **Prioritized Experience Replay**: sample *more important* experiences (those with the largest prediction error) from the replay buffer more often, instead of uniformly — speeds up learning.
- **Dueling DQN**: architecturally splits the network into a state-value estimate and an action-advantage estimate, which are recombined — often improves learning efficiency and stability.

---

## 🧰 TF-Agents — a production-grade RL library

Hand-rolling every RL algorithm (as above) is instructive but not how you'd build something serious. **TF-Agents** provides ready-made, well-tested environments, agents, replay buffers, and training infrastructure for scaling RL up (including Atari preprocessing wrappers — grayscale conversion, frame skipping, downsampling — and full training-loop scaffolding). Worth reaching for once you've understood the fundamentals above.

---

## 🏆 Overview of Popular RL Algorithms (know the names and core ideas)

- **Actor-Critic**: combines Policy Gradients (the "actor," choosing actions) with a DQN-style value estimator (the "critic," judging how good those actions were) — the actor learns faster/more efficiently by leaning on the critic's estimates instead of needing to finish whole episodes to compute advantages, much like an athlete (actor) improving faster with a coach's (critic's) feedback.
- **A3C (Asynchronous Advantage Actor-Critic)**: many agents train in parallel on separate environment copies, periodically syncing weight updates to/from a shared master network — pools experience across agents.
- **A2C**: a synchronous version of A3C — simpler, and better utilizes GPU batch parallelism.
- **SAC (Soft Actor-Critic)**: adds an entropy-maximization objective — the agent tries to be as **unpredictable** as possible while still collecting reward, which naturally encourages more thorough exploration and much better sample efficiency than earlier methods.
- **PPO (Proximal Policy Optimization)**: clips the policy update to prevent excessively large, destabilizing weight changes — a simpler, very popular, empirically effective descendant of TRPO; the algorithm behind OpenAI Five's Dota 2 superhuman play.
- **Curiosity-based exploration**: in environments with sparse/rare rewards, give the agent an *intrinsic* reward for encountering surprising, hard-to-predict outcomes — makes the agent naturally explore even before any external reward signal shows up.

---

## ⚠️ Common Pitfalls

- Judging RL training progress by loss curves alone — track actual reward/return per episode instead.
- Expecting RL training to be as stable/reliable as supervised learning — RL is notoriously sensitive to hyperparameters, random seeds, and network architecture choices; "must be forced to work," as researcher Andrej Karpathy put it.
- Skipping the replay buffer (or making it too small) — leads to unstable training from highly-correlated consecutive experiences.
- Forgetting to decay $\varepsilon$ in an ε-greedy policy — too much random exploration forever prevents the agent from ever exploiting what it has learned.
- Ignoring reward sparsity — if rewards are extremely rare, plain RL algorithms may barely learn at all; consider reward shaping (injecting helpful intermediate rewards) or curiosity-based methods.
- Using on-policy intuitions (like Policy Gradients) to reason about off-policy algorithms (like Q-Learning), or vice versa — they behave differently with respect to what "the policy being explored" means.

---

## 🔑 Cheat Sheet

- RL loop: agent observes → acts → gets reward → repeats; goal = maximize cumulative (discounted) reward via a learned **policy**.
- **Policy Gradients (REINFORCE)**: play episodes, compute discounted+normalized **advantages**, scale saved gradients by advantage, apply — on-policy, simple, but sample-inefficient.
- **MDP**: states + actions + transition probabilities + rewards; **Bellman equation** → Value/Q-Value Iteration (needs known dynamics).
- **Q-Learning**: learns Q-values from experience alone (no known dynamics needed) — off-policy; use **ε-greedy** exploration, decaying ε over time.
- **Deep Q-Learning (DQN)**: neural net approximates Q-values for huge state spaces; **replay buffer** stabilizes training by decorrelating experiences; watch **actual reward**, not loss, to judge progress; beware **catastrophic forgetting**.
- DQN variants: Fixed Targets, Double DQN, Prioritized Experience Replay, Dueling DQN — all aimed at stabilizing/speeding up basic DQN.
- **Actor-Critic family** (A3C, A2C, SAC, PPO): combine policy learning with a learned value/advantage estimator for much better sample efficiency than plain PG; **PPO** and **SAC** are strong modern defaults.
- **TF-Agents**: production-grade library for scaling RL beyond hand-rolled toy implementations.

---

## ✍️ Practice

1. **Hands-on:** Implement the REINFORCE algorithm above on Gym's `CartPole-v1`. Plot mean episode reward over training iterations — how many iterations until it consistently reaches ~200?
2. **Conceptual:** Explain the difference between on-policy and off-policy learning, using Q-Learning (off) and Policy Gradients (on) as your examples.
3. **Hands-on:** Implement basic Deep Q-Learning (with a replay buffer) on `CartPole-v1`. Do you observe any catastrophic forgetting during training? Try increasing the replay buffer size — does it help?
4. **Debug-the-scenario:** Your DQN's loss steadily decreases over training, but the agent's actual game score gets *worse*. What does this tell you, and what should you monitor instead?
   <details><summary>Answer</summary>Loss is a poor proxy for RL performance — it doesn't necessarily track actual policy quality (e.g., the agent could be overfitting to a narrow region of the state space it keeps revisiting). Monitor the agent's actual cumulative reward per episode instead.</details>
5. **Conceptual:** Why does adding an entropy-maximization term (as in SAC) tend to improve exploration and sample efficiency compared to plain ε-greedy exploration?

Next: `19_training_deploying_at_scale.md` — the final chapter: getting a trained model into production, across devices, and at scale.