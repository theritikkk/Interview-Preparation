# 08 — String Tricks

---

## Conversion

```cpp
int  x  = stoi(s);            // throws on overflow/bad input
ll   y  = stoll(s);
ll   z  = stoll(s, nullptr, 2);      // base 2
double d = stod(s);
string s = to_string(x);

// int -> string in another base: build manually or use bitset for binary
string bin = bitset<32>(x).to_string();     // zero-padded to 32
```
**Trap:** `stoi` on a value beyond `int` throws `out_of_range` → runtime error, not a wrong answer. Use `stoll` when the number can be large.

---

## Character arithmetic

```cpp
int d = c - '0';          // '7' -> 7
char c = '0' + d;         // 7 -> '7'
int i = c - 'a';          // 'c' -> 2
char c = 'a' + i;
int i = c - 'A';          // uppercase

tolower(c); toupper(c);   // <cctype>, return int
isdigit(c); isalpha(c); isalnum(c); isspace(c); isupper(c); islower(c);
```
**Trap:** `tolower` returns `int`. `string s; s += tolower(c);` works, but `char x = tolower(c)` needs the implicit narrowing (fine in practice).
**Trap (pedantic but real):** `tolower` takes an `int` that must be representable as `unsigned char`. Passing a negative `char` (possible with non-ASCII input) is UB. Contest input is ASCII, so this rarely bites.

**Case-flip trick:** `c ^ 32` toggles case for ASCII letters (`'a' = 97 = 0x61`, `'A' = 65 = 0x41`, differ in bit 5).

**ASCII values worth knowing:** `'0'` = 48, `'A'` = 65, `'a'` = 97, space = 32.

---

## Frequency arrays

```cpp
int freq[26] = {};                   // zero-initialized
for (char c : s) freq[c - 'a']++;

array<int, 26> f{};                  // same, but has .size(), copyable, comparable
array<int,26> fa{}, fb{};
if (fa == fb) { /* anagrams — array supports == */ }
```
**Use `array<int,26>` when:** you want to compare, copy, or store frequency vectors (e.g. as a `map` key for anagram grouping). Raw arrays can't be compared with `==` or returned by value.

**For arbitrary characters:** `int freq[128]` indexed by the char directly. Still beats a `map<char,int>` by a wide margin.

**Bitmask of present letters** (when only presence matters, not counts):
```cpp
int mask = 0;
for (char c : s) mask |= 1 << (c - 'a');
// two strings share a letter iff (m1 & m2) != 0
```
This turns "do these two strings share a character" into one AND. Very common in "max product of lengths of two words with no common letters".

---

## Reading input

```cpp
string s; cin >> s;                  // one token, stops at whitespace
getline(cin, line);                  // whole line including spaces

// THE TRAP:
int n; cin >> n;
getline(cin, line);                  // reads the leftover EMPTY rest of the line
// fix:
cin >> n; cin.ignore();              // discard one char (the newline)
// or:
cin >> n; getline(cin, line);        // dummy read, then the real getline
```
**`cin.ignore()`** discards one character. `cin.ignore(numeric_limits<streamsize>::max(), '\n')` discards up to the newline.

### stringstream

```cpp
stringstream ss(line);
string word;
while (ss >> word) { ... }           // tokenize by whitespace

// split by a delimiter
stringstream ss(s);
string tok;
while (getline(ss, tok, ',')) { ... }

// build a string
stringstream out;
out << a << " " << b;
string result = out.str();
```
`stringstream` is convenient but slow (allocations + stream machinery). For 1e6 tokens, parse manually.

---

## String operations

```cpp
s.substr(pos);              // from pos to the end
s.substr(pos, len);         // len characters — O(len), COPIES
s.find("abc");              // returns string::npos if not found
s.find(c, from);
s.rfind("abc");             // last occurrence
s.erase(pos, len);
s.insert(pos, "x");
s.replace(pos, len, "xyz");
s.push_back(c); s += c;     // O(1) amortized
reverse(s.begin(), s.end());
s.compare(t);               // <0, 0, >0
s1 < s2;                    // lexicographic, works directly
```

**Trap:** `if (s.find("x") != string::npos)`. Writing `if (s.find("x") >= 0)` is always true — `npos` is `(size_t)-1`, a huge unsigned value.

**Trap:** `substr` is O(len) and allocates. Inside a loop over all substrings that's O(n³). Use indices instead.

**Trap:** `s = s + c` in a loop is O(n²) — it builds a new string each time. `s += c` is amortized O(1).

---

## Small String Optimization (SSO)

`std::string` in libstdc++ stores strings up to **15 characters inline** in the 32-byte object — no heap allocation. Beyond that it allocates.

**Consequences:**
- Copying short strings is cheap (no allocation) → `vector<string>` of short words is fine.
- `sizeof(string)` is 32 regardless of content.
- A `map<string,int>` with short keys is much less bad than you'd fear.
- Moving a short string is **not** free (it copies the inline buffer), unlike a long one.

This is also a classic interview question — see `13_QUANT_CPP_INTERVIEW.md`.

---

## Z-function

`z[i]` = length of the longest common prefix of `s` and `s[i..]`. `z[0]` is conventionally 0 or n.

```cpp
vector<int> zfunc(const string& s) {
    int n = s.size();
    vector<int> z(n, 0);
    for (int i = 1, l = 0, r = 0; i < n; i++) {
        if (i < r) z[i] = min(r - i, z[i - l]);
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) z[i]++;
        if (i + z[i] > r) l = i, r = i + z[i];
    }
    return z;
}
```
**O(n).**

**Pattern matching:** build `z` on `pattern + '#' + text` (`'#'` must not appear in either). Any `i` with `z[i] == pattern.size()` is a match at `i - pattern.size() - 1` in the text.

**Also gives:** number of distinct prefixes that are also suffixes, periodicity.

---

## KMP prefix function

`pi[i]` = length of the longest **proper** prefix of `s[0..i]` that is also a suffix of `s[0..i]`.

```cpp
vector<int> prefix_function(const string& s) {
    int n = s.size();
    vector<int> pi(n, 0);
    for (int i = 1; i < n; i++) {
        int j = pi[i - 1];
        while (j > 0 && s[i] != s[j]) j = pi[j - 1];
        if (s[i] == s[j]) j++;
        pi[i] = j;
    }
    return pi;
}
```
**O(n).**

**Pattern matching:** run it on `pattern + '#' + text`, look for `pi[i] == pattern.size()`.

**The period trick:** the smallest period of `s` is `n - pi[n-1]`. The string is a repetition of that period iff `n % (n - pi[n-1]) == 0`. This solves a surprising number of problems ("minimum characters to append to make it a repetition", "is this string periodic").

**Borders:** all borders of `s` are `pi[n-1]`, `pi[pi[n-1]-1]`, … following the chain down to 0.

---

## String hashing (do it safely)

```cpp
struct Hash {
    static const ll M = (1LL << 61) - 1;     // Mersenne prime
    vector<ll> h, p;
    ll B;
    static ll mul(ll a, ll b) {
        __int128 c = (__int128)a * b;
        return ll((c >> 61) + (c & M)) % M;   // fast mod for 2^61-1
    }
    Hash(const string& s) {
        mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
        B = uniform_int_distribution<ll>(256, M - 2)(rng);
        int n = s.size();
        h.assign(n + 1, 0); p.assign(n + 1, 1);
        for (int i = 0; i < n; i++) {
            h[i+1] = (mul(h[i], B) + s[i]) % M;
            p[i+1] = mul(p[i], B);
        }
    }
    ll get(int l, int r) {                    // [l, r] inclusive, 0-indexed
        return ((h[r+1] - mul(h[l], p[r-l+1])) % M + M) % M;
    }
};
```

**Why the randomness:** with a fixed base and `MOD = 1e9+7`, anti-hash tests exist on Codeforces and people **will** hack you. Random base, and ideally a 2^61−1 modulus (collision probability ~n²/2^61, negligible) or **double hashing** with two different moduli.

**Never:** single hash with MOD = 2^64 (unsigned overflow) — Thue-Morse strings break it deterministically.

**Use when:** comparing substrings in O(1), finding duplicates, palindrome checks, matching multiple patterns. When an exact deterministic answer is required and n is small, prefer KMP/Z.

---

## Trie (array-backed)

```cpp
const int MAXNODES = 1e6, ALPHA = 26;
int nxt[MAXNODES][ALPHA], cntNode = 1;      // node 0 = root
bool term[MAXNODES];

void insert(const string& s) {
    int v = 0;
    for (char c : s) {
        int ch = c - 'a';
        if (!nxt[v][ch]) nxt[v][ch] = cntNode++;
        v = nxt[v][ch];
    }
    term[v] = true;
}
```
**Use an array, not pointers.** `new TrieNode` per node is slow and fragments memory; the array version is contiguous and cache-friendly.
**Memory:** `nxt[1e6][26]` of int = **104 MB**. Size `MAXNODES` to the *total length of all strings*, not a guess.
**Binary trie** (ALPHA = 2, depth 30) for XOR-max queries — same structure, `nxt[MAXNODES][2]`.

---

## Palindromes

```cpp
bool isPal(const string& s) {
    for (int i = 0, j = s.size() - 1; i < j; i++, j--)
        if (s[i] != s[j]) return false;
    return true;
}
```
**Expand around center** — all palindromic substrings in O(n²):
```cpp
for (int c = 0; c < n; c++) {
    for (int d = 0; c-d >= 0 && c+d < n && s[c-d] == s[c+d]; d++) { /* odd length */ }
    for (int d = 0; c-d >= 0 && c+d+1 < n && s[c-d] == s[c+d+1]; d++) { /* even length */ }
}
```
**Trap:** you need **both** the odd-length and even-length passes. Forgetting the even case is the standard bug.

**Manacher** gives all palindromic radii in O(n) — keep it as a template, don't memorize it. Full code in `27_ADVANCED_STRINGS_AND_STRUCTURES.md`.

**Aho-Corasick** generalizes both the Trie above and KMP to matching many patterns against one text in a single O(n) pass — also in `27_ADVANCED_STRINGS_AND_STRUCTURES.md`.

---

## Common traps

```
[ ] s.find(...) compared with >= 0 instead of != string::npos
[ ] s.size() is unsigned → s.size() - 1 on an empty string
[ ] getline after cin >> without cin.ignore()
[ ] substr in a loop → O(n^2) or worse
[ ] s = s + c instead of s += c
[ ] freq array not reset between test cases
[ ] assumed lowercase only when input has uppercase/digits
[ ] empty string edge case
[ ] palindrome: forgot even-length centers
[ ] hashing with a fixed base on Codeforces → hacked
[ ] Trie MAXNODES too small
[ ] comparing strings with strcmp semantics when < already works
```

---

## Memory hooks

- *`c - '0'` for digits, `c - 'a'` for letters, `'a' + i` to go back*
- *`cin.ignore()` before every `getline` that follows a `>>`*
- *`npos`, never `>= 0`*
- *`+=` not `= s +`*
- *KMP period: `n - pi[n-1]`*
- *Z on `pat + '#' + text`*
- *random the hash base, or get hacked*