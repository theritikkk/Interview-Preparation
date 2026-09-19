# 16 — Micro Templates

Minimal, reliable, copy-paste. Explanations live in the topic files.

---

## Boilerplate

```cpp
#include <bits/stdc++.h>
using namespace std;
using ll = long long;
using pii = pair<int,int>;
using pll = pair<ll,ll>;
#define all(x) (x).begin(), (x).end()
#define sz(x) (int)(x).size()
const int MOD = 1e9 + 7;
const int IINF = 0x3f3f3f3f;
const ll LINF = 0x3f3f3f3f3f3f3f3fLL;

void solve() {
    // ...
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    int T = 1;
    cin >> T;
    while (T--) solve();
    return 0;
}
```

---

## `__int128` printer

```cpp
using i128 = __int128;
ostream& operator<<(ostream& os, i128 x) {
    if (x == 0) return os << 0;
    if (x < 0) { os << '-'; x = -x; }
    string s;
    while (x) { s += char('0' + int(x % 10)); x /= 10; }
    reverse(s.begin(), s.end());
    return os << s;
}
i128 readI128() {
    string s; cin >> s;
    i128 r = 0; int i = 0; bool neg = (s[0] == '-'); if (neg) i = 1;
    for (; i < (int)s.size(); i++) r = r * 10 + (s[i] - '0');
    return neg ? -r : r;
}
```
Requires a 64-bit compiler.

---

## Binary exponentiation

```cpp
ll binpow(ll a, ll b, ll m) {
    a %= m; if (a < 0) a += m;
    ll r = 1 % m;
    while (b > 0) {
        if (b & 1) r = r * a % m;
        a = a * a % m;
        b >>= 1;
    }
    return r;
}
ll modinv(ll a, ll m) { return binpow(a, m - 2, m); }   // m PRIME
```

---

## Extended Euclid + general inverse

```cpp
ll ext_gcd(ll a, ll b, ll &x, ll &y) {
    if (!b) { x = 1; y = 0; return a; }
    ll x1, y1, g = ext_gcd(b, a % b, x1, y1);
    x = y1; y = x1 - (a / b) * y1;
    return g;
}
ll inv_general(ll a, ll m) {
    ll x, y;
    if (ext_gcd(a, m, x, y) != 1) return -1;
    return ((x % m) + m) % m;
}
```

---

## Sieve + SPF + factorization

```cpp
const int N = 1e6 + 5;
int spf[N];
vector<int> primes;
void sieve() {
    for (int i = 2; i < N; i++) {
        if (!spf[i]) { spf[i] = i; primes.push_back(i); }
        for (int p : primes) {
            if (p > spf[i] || (ll)p * i >= N) break;
            spf[p * i] = p;
        }
    }
}
vector<pii> factorize(int n) {
    vector<pii> f;
    while (n > 1) {
        int p = spf[n], c = 0;
        while (n % p == 0) n /= p, c++;
        f.push_back({p, c});
    }
    return f;
}
vector<ll> divisorsOf(ll n) {                 // O(sqrt n), no sieve needed
    vector<ll> d;
    for (ll i = 1; i * i <= n; i++)
        if (n % i == 0) { d.push_back(i); if (i != n / i) d.push_back(n / i); }
    sort(d.begin(), d.end());
    return d;
}
```

---

## nCr mod prime

```cpp
const int MAXF = 2e5 + 5;
ll fact[MAXF], invFact[MAXF];
void initC() {
    fact[0] = 1;
    for (int i = 1; i < MAXF; i++) fact[i] = fact[i-1] * i % MOD;
    invFact[MAXF-1] = binpow(fact[MAXF-1], MOD - 2, MOD);
    for (int i = MAXF - 1; i > 0; i--) invFact[i-1] = invFact[i] * i % MOD;
}
ll nCr(int n, int r) {
    if (r < 0 || r > n || n < 0) return 0;
    return fact[n] * invFact[r] % MOD * invFact[n-r] % MOD;
}
```

---

## Miller–Rabin

```cpp
using u64 = unsigned long long; using u128 = __uint128_t;
u64 mpow(u64 b, u64 e, u64 m){ u64 r=1; b%=m; while(e){ if(e&1) r=(u128)r*b%m; b=(u128)b*b%m; e>>=1;} return r; }
bool isPrime(u64 n) {
    if (n < 2) return false;
    for (u64 p : {2,3,5,7,11,13,17,19,23,29,31,37}) if (n % p == 0) return n == p;
    u64 d = n - 1; int s = 0;
    while (!(d & 1)) d >>= 1, s++;
    for (u64 a : {2,3,5,7,11,13,17,19,23,29,31,37}) {
        u64 x = mpow(a, d, n);
        if (x == 1 || x == n-1) continue;
        bool ok = false;
        for (int i = 1; i < s; i++) { x = (u128)x*x%n; if (x == n-1) { ok = true; break; } }
        if (!ok) return false;
    }
    return true;
}
```

---

## DSU

```cpp
struct DSU {
    vector<int> p, sz; int comps;
    DSU(int n) : p(n+1), sz(n+1,1), comps(n) { iota(p.begin(), p.end(), 0); }
    int find(int x) { return p[x]==x ? x : p[x]=find(p[x]); }
    bool unite(int a, int b) {
        a = find(a); b = find(b);
        if (a == b) return false;
        if (sz[a] < sz[b]) swap(a, b);
        p[b] = a; sz[a] += sz[b]; comps--;
        return true;
    }
    bool same(int a, int b) { return find(a) == find(b); }
};
```

---

## BFS / DFS / grid

```cpp
vector<int> bfs(int s, const vector<vector<int>>& adj) {
    vector<int> d(adj.size(), -1);
    queue<int> q; d[s] = 0; q.push(s);
    while (!q.empty()) {
        int u = q.front(); q.pop();
        for (int v : adj[u]) if (d[v] == -1) { d[v] = d[u] + 1; q.push(v); }
    }
    return d;
}

const int dx[4] = {0,0,1,-1}, dy[4] = {1,-1,0,0};
// inside the loop:
// int nx = x+dx[k], ny = y+dy[k];
// if (nx<0||nx>=R||ny<0||ny>=C||g[nx][ny]=='#'||vis[nx][ny]) continue;

void dfsIter(int s, const vector<vector<int>>& adj, vector<char>& vis) {
    vector<int> st{s};
    while (!st.empty()) {
        int u = st.back(); st.pop_back();
        if (vis[u]) continue;
        vis[u] = 1;
        for (int v : adj[u]) if (!vis[v]) st.push_back(v);
    }
}
```

---

## Topological sort

```cpp
vector<int> topo(int n, const vector<vector<int>>& adj) {
    vector<int> indeg(n+1, 0), order;
    for (int u = 1; u <= n; u++) for (int v : adj[u]) indeg[v]++;
    queue<int> q;
    for (int u = 1; u <= n; u++) if (!indeg[u]) q.push(u);
    while (!q.empty()) {
        int u = q.front(); q.pop(); order.push_back(u);
        for (int v : adj[u]) if (--indeg[v] == 0) q.push(v);
    }
    return order;                     // size < n  ⇒  cycle exists
}
```

---

## Dijkstra

```cpp
vector<ll> dijkstra(int s, const vector<vector<pii>>& adj) {
    const ll INF = 2e18;
    vector<ll> d(adj.size(), INF);
    priority_queue<pll, vector<pll>, greater<pll>> pq;
    d[s] = 0; pq.push({0, s});
    while (!pq.empty()) {
        auto [dd, u] = pq.top(); pq.pop();
        if (dd > d[u]) continue;
        for (auto [v, w] : adj[u])
            if (d[u] + w < d[v]) { d[v] = d[u] + w; pq.push({d[v], v}); }
    }
    return d;
}
```

---

## 0-1 BFS

```cpp
vector<int> bfs01(int s, const vector<vector<pii>>& adj) {
    vector<int> d(adj.size(), INT_MAX);
    deque<int> dq; d[s] = 0; dq.push_front(s);
    while (!dq.empty()) {
        int u = dq.front(); dq.pop_front();
        for (auto [v, w] : adj[u])
            if (d[u] + w < d[v]) {
                d[v] = d[u] + w;
                if (w) dq.push_back(v); else dq.push_front(v);
            }
    }
    return d;
}
```

---

## Kruskal

```cpp
ll kruskal(int n, vector<array<int,3>>& e) {     // {w, u, v}
    sort(e.begin(), e.end());
    DSU d(n); ll total = 0; int used = 0;
    for (auto [w, u, v] : e) if (d.unite(u, v)) { total += w; used++; }
    return used == n - 1 ? total : -1;
}
```

---

## Fenwick tree

```cpp
struct BIT {
    int n; vector<ll> t;
    BIT(int n) : n(n), t(n + 1, 0) {}
    void add(int i, ll v) { for (; i <= n; i += i & -i) t[i] += v; }   // 1-INDEXED
    ll sum(int i) { ll s = 0; for (; i > 0; i -= i & -i) s += t[i]; return s; }
    ll range(int l, int r) { return sum(r) - sum(l - 1); }
};
```

---

## Iterative segment tree (point update, range sum)

```cpp
struct Seg {
    int n; vector<ll> t;
    Seg(int n) : n(n), t(2 * n, 0) {}
    void build(const vector<ll>& a) {
        for (int i = 0; i < n; i++) t[n + i] = a[i];
        for (int i = n - 1; i > 0; i--) t[i] = t[2*i] + t[2*i+1];
    }
    void update(int i, ll v) { for (t[i += n] = v; i > 1; i >>= 1) t[i>>1] = t[i] + t[i^1]; }
    ll query(int l, int r) {                     // [l, r) HALF-OPEN
        ll res = 0;
        for (l += n, r += n; l < r; l >>= 1, r >>= 1) {
            if (l & 1) res += t[l++];
            if (r & 1) res += t[--r];
        }
        return res;
    }
};
```

---

## Lazy segment tree (range add, range sum)

```cpp
struct Lazy {
    int n; vector<ll> t, lz;
    Lazy(int n) : n(n), t(4*n, 0), lz(4*n, 0) {}
    void push(int v, int l, int r) {
        if (!lz[v]) return;
        t[v] += lz[v] * (r - l + 1);
        if (l != r) { lz[2*v] += lz[v]; lz[2*v+1] += lz[v]; }
        lz[v] = 0;
    }
    void upd(int v, int l, int r, int ql, int qr, ll val) {
        push(v, l, r);
        if (qr < l || r < ql) return;
        if (ql <= l && r <= qr) { lz[v] += val; push(v, l, r); return; }
        int m = (l + r) >> 1;
        upd(2*v, l, m, ql, qr, val); upd(2*v+1, m+1, r, ql, qr, val);
        t[v] = t[2*v] + t[2*v+1];
    }
    ll qry(int v, int l, int r, int ql, int qr) {
        push(v, l, r);
        if (qr < l || r < ql) return 0;
        if (ql <= l && r <= qr) return t[v];
        int m = (l + r) >> 1;
        return qry(2*v, l, m, ql, qr) + qry(2*v+1, m+1, r, ql, qr);
    }
};
// call as: seg.upd(1, 0, n-1, l, r, v);  seg.qry(1, 0, n-1, l, r);
```

---

## Sparse table (range min, O(1) query)

```cpp
struct Sparse {
    vector<vector<int>> sp;
    Sparse(const vector<int>& a) {
        int n = a.size(), LOG = __lg(n) + 1;
        sp.assign(LOG, vector<int>(n));
        sp[0] = a;
        for (int k = 1; k < LOG; k++)
            for (int i = 0; i + (1 << k) <= n; i++)
                sp[k][i] = min(sp[k-1][i], sp[k-1][i + (1 << (k-1))]);
    }
    int query(int l, int r) {              // [l, r] INCLUSIVE
        int k = __lg(r - l + 1);
        return min(sp[k][l], sp[k][r - (1 << k) + 1]);
    }
};
```

---

## Coordinate compression

```cpp
vector<int> vals = a;
sort(vals.begin(), vals.end());
vals.erase(unique(vals.begin(), vals.end()), vals.end());
auto rk = [&](int x) { return int(lower_bound(vals.begin(), vals.end(), x) - vals.begin()); };
for (int i = 0; i < n; i++) b[i] = rk(a[i]);
```

---

## Binary search

```cpp
// FIRST index in [lo, hi) where ok(x) is true; returns hi if none
int firstTrue(int lo, int hi, function<bool(int)> ok) {
    while (lo < hi) {
        int mid = lo + (hi - lo) / 2;
        if (ok(mid)) hi = mid; else lo = mid + 1;
    }
    return lo;
}

// LAST index in [lo, hi] where ok(x) is true; returns lo-1 if none
int lastTrue(int lo, int hi, function<bool(int)> ok) {
    int res = lo - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (ok(mid)) { res = mid; lo = mid + 1; } else hi = mid - 1;
    }
    return res;
}

// floating point
double fbs(double lo, double hi, function<bool(double)> ok) {
    for (int i = 0; i < 100; i++) {
        double mid = (lo + hi) / 2;
        if (ok(mid)) hi = mid; else lo = mid;
    }
    return lo;
}
```
Replace `function<...>` with a template or a lambda parameter if you need the speed (it's an indirect call).

---

## Prefix sums / difference arrays

```cpp
// 1D prefix
vector<ll> pre(n + 1, 0);
for (int i = 0; i < n; i++) pre[i+1] = pre[i] + a[i];
// sum of a[l..r] = pre[r+1] - pre[l]

// 2D prefix ((n+1) x (m+1))
for (int i = 1; i <= n; i++)
  for (int j = 1; j <= m; j++)
    P[i][j] = a[i-1][j-1] + P[i-1][j] + P[i][j-1] - P[i-1][j-1];
// rect [r1..r2][c1..c2] = P[r2+1][c2+1] - P[r1][c2+1] - P[r2+1][c1] + P[r1][c1]

// 1D difference
vector<ll> d(n + 1, 0);
// add v on [l, r]:  d[l] += v; d[r+1] -= v;
for (int i = 1; i < n; i++) d[i] += d[i-1];

// 2D difference
// D[r1][c1]+=v; D[r1][c2+1]-=v; D[r2+1][c1]-=v; D[r2+1][c2+1]+=v;  then prefix twice
```

---

## KMP prefix function

```cpp
vector<int> pfunc(const string& s) {
    int n = s.size(); vector<int> pi(n, 0);
    for (int i = 1; i < n; i++) {
        int j = pi[i-1];
        while (j > 0 && s[i] != s[j]) j = pi[j-1];
        if (s[i] == s[j]) j++;
        pi[i] = j;
    }
    return pi;
}
// search: pfunc(pat + '#' + txt), look for pi[i] == pat.size()
```

---

## Z-function

```cpp
vector<int> zfunc(const string& s) {
    int n = s.size(); vector<int> z(n, 0);
    for (int i = 1, l = 0, r = 0; i < n; i++) {
        if (i < r) z[i] = min(r - i, z[i - l]);
        while (i + z[i] < n && s[z[i]] == s[i + z[i]]) z[i]++;
        if (i + z[i] > r) l = i, r = i + z[i];
    }
    return z;
}
```

---

## Trie (array-backed)

```cpp
const int MAXNODES = 1e6;
int nxt[MAXNODES][26], cntN = 1;
bool term[MAXNODES];
void insert(const string& s) {
    int v = 0;
    for (char c : s) {
        int ch = c - 'a';
        if (!nxt[v][ch]) nxt[v][ch] = cntN++;
        v = nxt[v][ch];
    }
    term[v] = true;
}
bool query(const string& s) {
    int v = 0;
    for (char c : s) { v = nxt[v][c - 'a']; if (!v) return false; }
    return term[v];
}
```

---

## Rolling hash (safe)

```cpp
struct Hash {
    static const ll M = (1LL << 61) - 1;
    vector<ll> h, p; ll B;
    static ll mul(ll a, ll b) {
        __int128 c = (__int128)a * b;
        return ll((c >> 61) + (c & M)) % M;
    }
    Hash(const string& s) {
        mt19937_64 rng(chrono::steady_clock::now().time_since_epoch().count());
        B = uniform_int_distribution<ll>(256, M - 2)(rng);
        int n = s.size(); h.assign(n+1, 0); p.assign(n+1, 1);
        for (int i = 0; i < n; i++) {
            h[i+1] = (mul(h[i], B) + s[i]) % M;
            p[i+1] = mul(p[i], B);
        }
    }
    ll get(int l, int r) {                  // [l, r] inclusive
        return ((h[r+1] - mul(h[l], p[r-l+1])) % M + M) % M;
    }
};
```

---

## Matrix exponentiation

```cpp
const int K = 2;
using Mat = array<array<ll,K>,K>;
Mat mul(const Mat& A, const Mat& B) {
    Mat C{};
    for (int i = 0; i < K; i++)
      for (int k = 0; k < K; k++) if (A[i][k])
        for (int j = 0; j < K; j++)
            C[i][j] = (C[i][j] + A[i][k] * B[k][j]) % MOD;
    return C;
}
Mat mpow(Mat A, ll e) {
    Mat R{}; for (int i = 0; i < K; i++) R[i][i] = 1;
    while (e) { if (e & 1) R = mul(R, A); A = mul(A, A); e >>= 1; }
    return R;
}
```

---

## Meet in the middle (subset sum, n ≤ ~40)

```cpp
bool subsetSumExists(vector<ll>& a, ll target) {
    int n = a.size(), h = n / 2, rest = n - h;
    vector<ll> sumsA, sumsB;
    for (int mask = 0; mask < (1 << h); mask++) {
        ll s = 0;
        for (int i = 0; i < h; i++) if (mask >> i & 1) s += a[i];
        sumsA.push_back(s);
    }
    for (int mask = 0; mask < (1 << rest); mask++) {
        ll s = 0;
        for (int i = 0; i < rest; i++) if (mask >> i & 1) s += a[h + i];
        sumsB.push_back(s);
    }
    sort(sumsB.begin(), sumsB.end());
    for (ll s : sumsA)
        if (binary_search(sumsB.begin(), sumsB.end(), target - s)) return true;
    return false;
}
```
**Complexity:** O(2^(n/2) · n) — enumerate each half in O(2^(n/2)·n/2), sort one half, binary search from the other.
**Variant — count pairs with sum ≤ T instead of checking existence:** sort both halves and two-pointer across them instead of binary-searching per element.

---

## LCA (binary lifting)

```cpp
int LOG, up[20][MAXN], dep[MAXN];
void dfs(int u, int p) {
    up[0][u] = p;
    for (int k = 1; k < LOG; k++) up[k][u] = up[k-1][ up[k-1][u] ];
    for (int v : adj[u]) if (v != p) { dep[v] = dep[u] + 1; dfs(v, u); }
}
int lca(int a, int b) {
    if (dep[a] < dep[b]) swap(a, b);
    int diff = dep[a] - dep[b];
    for (int k = 0; k < LOG; k++) if (diff >> k & 1) a = up[k][a];
    if (a == b) return a;
    for (int k = LOG - 1; k >= 0; k--)
        if (up[k][a] != up[k][b]) { a = up[k][a]; b = up[k][b]; }
    return up[0][a];
}
// init: LOG = __lg(n) + 1; dep[root] = 0; dfs(root, root);
// dist(a,b) = dep[a] + dep[b] - 2*dep[lca(a,b)]
```

---

## Random

```cpp
mt19937 rng(chrono::steady_clock::now().time_since_epoch().count());
mt19937_64 rng64(chrono::steady_clock::now().time_since_epoch().count());
int r = uniform_int_distribution<int>(1, n)(rng);
shuffle(v.begin(), v.end(), rng);
```

---

## Custom hash for unordered_map

```cpp
struct custom_hash {
    static uint64_t splitmix64(uint64_t x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
    }
    size_t operator()(uint64_t x) const {
        static const uint64_t F = chrono::steady_clock::now().time_since_epoch().count();
        return splitmix64(x + F);
    }
};
unordered_map<ll, int, custom_hash> mp;
```

---

## Stress test generator (Python)

```python
import random, sys
random.seed(int(sys.argv[1]))
n = random.randint(1, 6)
print(n)
print(*[random.randint(-5, 5) for _ in range(n)])
```

```bash
for i in $(seq 1 1000); do
  python3 gen.py $i > in.txt
  ./sol < in.txt > o1.txt; ./brute < in.txt > o2.txt
  diff -q o1.txt o2.txt > /dev/null || { echo "seed $i"; cat in.txt; break; }
done
```