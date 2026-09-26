[← Table of Contents](../README.md) · [← Lesson 42](lesson-42-pa-nonlinearity-papr-legal.md)

---

# Lesson 43 — Portfolio Packaging: GitHub Structure, READMEs, and Resume Bullets That Hold Up

*A practical lesson, not a technical one — but it turns out the exact same "measure, don't assert" ethos this curriculum has applied to your code since Lesson 9 applies with equal force to how you present that code to another person.*

### 1. Intuition

A brilliant project with no clear README is functionally invisible. A hiring manager or interviewer skimming a GitHub profile spends well under two minutes deciding whether to look closer. The engineering work — Projects 1 through 12, built across this curriculum — is necessary but not sufficient; it needs packaging that lets someone unfamiliar with RF/DSP understand, quickly, what was built, why it was hard, and what it demonstrates about you specifically. This is a genuinely distinct skill from the engineering itself, and it's one strong engineers neglect surprisingly often.

### 2. The engineering problem that forces this

A repository needs to work at two speeds simultaneously: a skimming reviewer should get the core value proposition in under a minute, and a genuinely engaged reviewer (in an actual technical interview, say) should be able to go deeper and find real evidence of *understanding* — not just working code that might have come from a tutorial. Critically, the project's own artifacts — plots, measured metrics, real numbers — should do the talking, rather than requiring the reviewer to simply trust unverified prose claims.

### 3. The core principle, stated directly

This lesson's actual content is one idea, applied consistently: **everything this curriculum has taught you about distrusting an unverified engineering claim — measure it, Lessons 9, 22, 25, 26, 34–37 — applies identically to how you present your own work.** A hiring engineer reading your README is doing exactly Lesson 34–38's kind of skeptical evaluation, now applied to *your* claims about *your* project. "Fast," "robust," "well-tested" are exactly as weak, unsupported, in a README as they'd be in a code comment with no benchmark behind them.

### 4. A structured README template, reasoned through

```
1. One-line summary: what this is, in plain language, no jargon a
   non-specialist reviewer would need to look up.
2. Why this is hard: the specific engineering problem, in 2-3 sentences —
   not "I built an OFDM system" but "I built and MEASURED a full OFDM
   TX/RX chain including cyclic-prefix-based multipath recovery."
3. Results, prominently, near the top — plots, measured numbers, a BER
   curve, a coding-gain figure — NOT buried at the bottom or in a
   separate notebook a skimming reviewer won't open.
4. Architecture diagram — a simple block diagram of the signal chain,
   even ASCII, so a reviewer can see the shape of the system at a glance.
5. What this demonstrates — explicit, not left for the reader to infer:
   name the specific skills (e.g. "matched-filter detection theory,
   Cauchy-Schwarz-optimal receiver design, translating a 3GPP-style
   sequence spec into working code").
6. How to run it — a reviewer who wants to verify your claims should
   be able to, in under 5 minutes, without guessing at missing steps.
7. Design decisions and trade-offs — the debugging stories and
   measured trade-offs that prove real understanding, not tutorial-following.
```

### 5. Why "what this demonstrates" must be explicit, not implicit

A reviewer outside your specific specialty — a recruiter, or even a hiring manager from an adjacent discipline within the same company — will very likely **not** automatically infer that "built a matched-filter-based Zadoff-Chu preamble detector" implies "understands optimal detection theory and can translate 3GPP spec sequences into working code." That inference has to be made explicit, in the reviewer's own language, or it simply doesn't happen — no matter how genuinely impressive the underlying work is. This is the single most common, highest-leverage gap between technically strong projects and portfolio projects that actually generate interviews.

### 6. Visualization

```
Ideal README reading order (top to bottom = what a 30-second skim sees first):

[ One-line summary ]
[ RESULTS: plot / measured number, right here, not buried ]
[ Architecture diagram ]
[ "What this demonstrates" — explicit skill list ]
[ How to run it ]
[ Design decisions & trade-offs (for a deeper read) ]
```

### 7. Worked example (structural outline, not invented content)

For Project 4 (OFDM TX/RX): one-line summary — "A complete OFDM transmitter/receiver in C++, including cyclic-prefix-based multipath recovery and per-subcarrier equalization, validated against a derived BER curve." Results section — the actual BER-vs-SNR plot from your own Lesson 7-style experiment, plus the specific measured coding gain if you layered in Lesson 41's Viterbi decoder. "What this demonstrates" — name Lessons 3, 4, 10, 11, 12 by concept (FFT/IFFT implementation, convolution/cyclic-prefix reasoning, channel estimation, MMSE equalization), not by lesson number.

### 8. Code — a small, reflexive tool: verify your own README, don't just assume it's complete

```python
import re

REQUIRED_SECTIONS = {
    "results": r"#.*(results|performance|measured)",
    "architecture": r"#.*(architecture|design|diagram)",
    "demonstrates": r"(demonstrates|skills|what this shows)",
    "how_to_run": r"#.*(usage|how to run|getting started)",
    "trade_offs": r"(trade-?off|design decision|debugging)",
}

def check_readme(path):
    with open(path) as f:
        text = f.read().lower()
    missing = [name for name, pattern in REQUIRED_SECTIONS.items()
               if not re.search(pattern, text)]
    has_numbers = bool(re.search(r"\d+(\.\d+)?\s*(db|%|ms|µs|mbps)", text))
    return {"missing_sections": missing, "has_concrete_metrics": has_numbers}

# check_readme("my_project/README.md")
```

### 9. Experiment

Run §8's checker against a deliberately incomplete example README (missing a results section, or containing only prose with no numeric metrics anywhere) and confirm it correctly flags what's missing. Then run it against your own draft READMEs for any completed curriculum projects.

### 10. Expected output

The checker flags missing sections explicitly and reports whether any concrete, numeric metric appears anywhere in the document — a simple, honest proxy for "does this README make verifiable claims or only unverified prose," directly applying this lesson's core principle back onto itself.

### 11. A realistic scenario worth reasoning through

You share your GitHub profile with a friend working in industry for quick feedback. After two minutes, they say: "I can tell you know how to code, but I can't tell if you understand DSP or if you copied the algorithms from a tutorial." Using this lesson: what's most likely missing?

**Reasoning through it:** almost certainly §5's gap — working code alone doesn't distinguish "I derived and understand cyclic-prefix's role from Lesson 12's proof" from "I copied a working example that happens to include a cyclic prefix." The differentiator isn't more code; it's showing *derived* understanding — a design decision explained in your own words, a real debugging story (exactly Lesson 34/38's methodology, written up), a measured trade-off you personally quantified. This is the whole curriculum's "reason from constraints, don't memorize" ethos, now applied to how the work is presented rather than how it was built.

### 12. Common mistakes

- Marketing language and buzzwords ("cutting-edge," "highly optimized") standing in for concrete, verifiable specifics — exactly the unverified-claim pattern this curriculum has taught you to distrust everywhere else.
- Burying actual results at the bottom of a README, or in a separate notebook a skimming reviewer won't open, rather than showing them prominently near the top.
- Resume bullets stating *what* was built without the *measured result*: "built an OFDM transceiver in C++" is far weaker than "built an OFDM transceiver in C++, achieving [specific measured BER/coding-gain figure] at [specific SNR]" — the second is dramatically stronger, and it's just this curriculum's own measured-results ethos applied to one sentence.

### 13. Real RF/cellular application

Hiring managers and technical interviewers at the companies in this curriculum's career map routinely skim GitHub portfolios as a first filter before a technical interview is even scheduled — a real, well-documented part of technical hiring. The "what would a Qualcomm/MediaTek/Samsung/Nokia/Ericsson engineer learn about me from this project" framing from this curriculum's original portfolio design is precisely what a well-structured README needs to answer explicitly for the reader, not leave for them to infer.

### 14. Practice: describe one of your own projects in 30 seconds

Pick one completed project from this curriculum and explain it out loud, in 30 seconds, to an imagined hiring manager — using §5's "what this demonstrates" framing explicitly, not just describing what the code does. This is Lesson 40's generation-not-recognition principle, applied to self-presentation specifically: if you can't produce this explanation unprompted and out loud, it isn't ready yet, no matter how good the underlying code is.

### 15. Active recall

1. Why must "what this demonstrates" be stated explicitly rather than left for a reviewer to infer?
2. Why do measured results belong near the top of a README rather than the bottom?
3. What's the concrete difference between a weak and a strong resume bullet for the same project, per §12?

### 16. Mini assignment

Run §8's checker against real draft READMEs for two of your own completed curriculum projects, revise based on what it flags, and write one resume bullet per project following §12's "state the measured result" principle.

### 17. Exit test

1. List the seven README sections from §4, from memory.
2. True or false, with a one-line reason: "A project with excellent working code but no results section or explicit skills framing will generally be evaluated as strongly as one that has both."
3. Your 30-second spoken description from §14, if you haven't done it yet — say it out loud, not just in your head.

---

**Next:** Lesson 44 returns to technical depth: **polar codes** — the channel code 3GPP actually chose for 5G NR's control channel, and the first explicit, constructive code ever proven to achieve Shannon capacity, closing a real gap left open since Lessons 19–20 and 29 discussed PBCH and PDCCH without ever specifying how those bits are error-protected.

---

[← Lesson 42](lesson-42-pa-nonlinearity-papr-legal.md) · [Table of Contents](../README.md) · [Lesson 44 →](lesson-44-polar-codes.md)