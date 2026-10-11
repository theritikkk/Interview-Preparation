# Compiler Design → Lexical Analysis

**GATE 2027 syllabus:** Compiler Design — lexical analysis.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** A lexical analyzer (scanner) reads the input left to right and applies the **longest match rule**: among all token patterns matching a prefix of the remaining input, it picks the longest match; ties between equally long matches are broken by the patterns' priority order (as listed). Whitespace is skipped and produces no token. An unrecognized character produces an ERROR token (of length 1) and scanning continues from the next character.

---

## Questions

### Q1 · MCQ · Fundamental
The "longest match rule" in lexical analysis means that, when multiple token patterns match a prefix of the remaining input, the scanner chooses

- (A) the first pattern listed, regardless of match length
- (B) the pattern producing the longest matched string
- (C) the pattern producing the shortest matched string
- (D) a pattern chosen at random

---

### Q2 · NAT · Fundamental
Using token patterns KEYWORD (if|else|while), ID ([a-zA-Z_][a-zA-Z0-9_]*), NUM (\d+), with KEYWORD given priority over ID on a tie, the input `while1` is scanned. The number of tokens produced is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about lexical analysis is/are TRUE?

- (A) Regular expressions are used to specify the patterns for each token class.
- (B) A lexical analyzer is typically implemented using a DFA obtained from the union of the NFAs for all token patterns.
- (C) The lexical analyzer is responsible for checking that the program is syntactically well-formed (e.g., matching parentheses).
- (D) A symbol table entry is typically created or looked up when an identifier token is recognized.

---

### Q4 · NAT · Standard
With token patterns KEYWORD (if|else|while), ID ([a-zA-Z_][a-zA-Z0-9_]*), NUM (\d+), OP ([+\-*/=<>]), LPAREN (`(`), RPAREN (`)`), SEMI (`;`) — KEYWORD has priority over ID on a tie — the input `if (x1 > 10) while (y=5) x1=x1+1;` is scanned (whitespace skipped, not counted as a token). The total number of tokens produced is ______.

---

### Q5 · MCQ · Standard
Which one of the following is generally **not** a task performed by the lexical analysis phase of a compiler?

- (A) Stripping out comments and whitespace
- (B) Grouping characters into tokens (e.g., recognizing `123` as a single NUM token)
- (C) Building the abstract syntax tree for an expression
- (D) Recognizing keywords and distinguishing them from identifiers

---

### Q6 · NAT · Moderate
A lexical analyzer uses the patterns `a?(b|c)*a` for token T1 and `b?(a|c)*b` for token T2, applying the longest-match rule with T1 given priority on ties. Scanning the single input string `bbaacabb` (8 characters) as one token attempt at the current position, the two patterns both match the entire 8-character string. Given this tie, the token produced is ______  (answer "T1" or "T2").

---

### Q7 · MSQ · Moderate
Which of the following statements is/are TRUE about **maximal munch** (another name for the longest-match rule) in lexical analysis?

- (A) It ensures that `<=` is tokenized as a single "less-than-or-equal" operator rather than as `<` followed by `=`, when both a single-character and multi-character pattern could apply.
- (B) It can cause minor ambiguity in languages with certain operator combinations, e.g., tokenizing `a+++b` in C-like languages as `a`, `++`, `+`, `b` rather than `a`, `+`, `++`, `b`.
- (C) It requires unbounded lookahead in the worst case for some token patterns, though most practical languages are designed to need only a small bounded lookahead.
- (D) It is irrelevant when all token patterns have a fixed, identical length.

---

### Q8 · NAT · Moderate
A lexical analyzer scans the input `aabbcabc` using the pattern `(ab|a|b|c)*` greedily matched as one single token from position 0 using longest match. Treating the sub-patterns `ab`, `a`, `b`, `c` as the "pieces" the longest overall match is built from (preferring `ab` over separate `a` then `b` whenever both the 2-character and 1-character alternatives are available at a position, since `ab` extends the match further), the minimum possible number of pieces used to tokenize the entire 8-character string this way is ______.

---

### Q9 · MSQ · Hard
Which of the following statements about the relationship between regular expressions, NFAs and DFAs in lexer construction is/are TRUE?

- (A) Thompson's construction converts a regular expression into an NFA with a number of states linear in the size of the regular expression.
- (B) Subset construction converts an NFA into an equivalent DFA, with the number of DFA states potentially exponential in the number of NFA states, in the worst case.
- (C) A lexer generator (like Lex/Flex) typically builds one combined NFA (or DFA) for all token patterns together, using a marker in accepting states to record which token pattern was matched.
- (D) DFA minimization is never applied in practical lexer generators, since it would merge accepting states for different token types.

---

### Q10 · NAT · Killer
A lexical analyzer processes the input string `ifelse123` using these patterns, in priority order: KEYWORD (`if` or `else`), ID (`[a-zA-Z_][a-zA-Z0-9_]*`), NUM (`\d+`). Applying the longest-match rule (with KEYWORD given priority only on an exact tie in match length), the number of tokens produced from this single input string (with no whitespace anywhere) is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Definition of the longest match (maximal munch) rule |
| 2 | 1 | `while1` as a whole matches ID (6 chars), longer than KEYWORD's `while` (5 chars); longest match wins regardless of priority (priority only breaks exact-length ties) |
| 3 | A, B, D | (C) is false — checking syntactic well-formedness (like matching parentheses) is the parser's job, not the lexer's |
| 4 | 18 | Verified by simulating the longest-match scanner on the full input |
| 5 | C | Building the AST is a parsing/syntax-analysis task, not lexical analysis |
| 6 | T1 | Both patterns match the full string with equal length, so the tie-break (T1 has priority) decides |
| 7 | A, B, C | (D) is false — even with fixed-length patterns, if multiple apply, a rule is still needed to pick one (though maximal munch becomes trivial/moot only when lengths truly can't differ, which isn't quite "irrelevant") |
| 8 | 6 | Greedy tokenization of `aabbcabc` (preferring `ab` whenever it applies): a, ab, b, c, ab, c — 6 pieces, lengths 1+2+1+1+2+1 = 8 |
| 9 | A, B, C | (D) is false — practical lexer generators do minimize the DFA, keeping per-state token-priority information intact (minimization respects the accepting-state partition by token type) |
| 10 | 1 | The entire 9-character string matches ID (`[a-zA-Z_][a-zA-Z0-9_]*` matches `ifelse123` fully, 9 chars), which is strictly longer than either KEYWORD alternative (2 or 4 chars); longest match gives a single ID token |
