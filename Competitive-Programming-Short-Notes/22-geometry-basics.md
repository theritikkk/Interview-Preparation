# 22 — Geometry Basics

**Rule #1: stay in integers.** If the input coordinates are integers, almost every geometric predicate can be answered with integer arithmetic. Every `double` you introduce is a potential precision WA.

---

## Point type

```cpp
struct P {
    ll x, y;
    P operator-(const P& o) const { return {x - o.x, y - o.y}; }
    P operator+(const P& o) const { return {x + o.x, y + o.y}; }
    bool operator<(const P& o) const { return x != o.x ? x < o.x : y < o.y; }
    bool operator==(const P& o) const { return x == o.x && y == o.y; }
};
ll cross(const P& a, const P& b) { return a.x * b.y - a.y * b.x; }
ll dot  (const P& a, const P& b) { return a.x * b.x + a.y * b.y; }
ll cross(const P& o, const P& a, const P& b) { return cross(a - o, b - o); }
ll dist2(const P& a, const P& b) { ll dx = a.x-b.x, dy = a.y-b.y; return dx*dx + dy*dy; }
```

**Overflow check:** with coordinates up to 1e9, `cross` computes `1e9 × 1e9 = 1e18` and subtracts two of them → up to 2e18. That **fits** in `long long` (max 9.22e18), but only just. With coordinates up to 1e9 after a subtraction (so differences up to 2e9), the cross product reaches 8e18 — still fits, barely. Coordinates beyond that need `__int128`.

---

## Cross product — the workhorse

```
cross(a, b) = a.x·b.y − a.y·b.x
```

**Geometric meaning:** `cross(O, A, B)` is the signed area of the parallelogram spanned by `OA` and `OB` — **twice** the signed area of triangle OAB.

| Sign | Meaning |
|---|---|
| `> 0` | B is to the **left** of ray O→A... more precisely, O→A→B turns **counter-clockwise** |
| `< 0` | clockwise turn |
| `= 0` | O, A, B are **collinear** |

```cpp
int orient(const P& o, const P& a, const P& b) {
    ll c = cross(o, a, b);
    return (c > 0) - (c < 0);      // 1 = CCW, -1 = CW, 0 = collinear
}
```

**This one function answers:** collinearity, left/right, convexity, segment intersection, point-in-polygon, convex hull. Memorize it.

**Memory hook:** *cross = turn direction. Positive = left turn = counter-clockwise.*

---

## Dot product

```
dot(a, b) = a.x·b.x + a.y·b.y = |a||b|cos θ
```
| Sign | Angle |
|---|---|
| `> 0` | acute (< 90°) |
| `= 0` | perpendicular |
| `< 0` | obtuse (> 90°) |

---

## Distances — never take the square root

```cpp
ll d2 = dist2(a, b);           // squared distance, exact in integers
// compare d2 values directly instead of sqrt(d2)
```
`sqrt` introduces error and costs ~15 cycles. **Any comparison of distances can be done on squared distances.** Only compute the actual distance if you must *print* it.

**Manhattan distance:** `|x1−x2| + |y1−y2|`. Rotating by 45° (`u = x+y`, `v = x−y`) turns Manhattan distance into Chebyshev distance `max(|u1−u2|, |v1−v2|)` — a standard trick for "maximum Manhattan distance among n points" in O(n).

**Chebyshev distance:** `max(|dx|, |dy|)` — the number of king moves on a chessboard.

---

## Triangle area

**Shoelace for a triangle:**
```cpp
// TWICE the area, signed
ll area2 = cross(a, b, c);
// actual area = abs(area2) / 2.0
```
Keeping `2 × area` as an integer avoids all floating point. If the problem asks for the area, print `area2 / 2` with a `.5` if `area2` is odd, or print as a fraction.

**Triangle inequality:** three lengths form a (non-degenerate) triangle iff `a + b > c` for the largest side `c`. **Sort first, then check only the largest.** Watch for overflow: `a + b` with values up to 1e18.

---

## Polygon area (shoelace)

```cpp
ll area2(const vector<P>& p) {
    int n = p.size();
    ll s = 0;
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        s += p[i].x * p[j].y - p[j].x * p[i].y;
    }
    return s;                        // signed; abs(s) / 2 = area
}
```
**Sign tells orientation:** positive = counter-clockwise, negative = clockwise.
**Trap:** the `% n` wraparound to close the polygon. Forgetting the last edge is the standard bug.

---

## Pick's theorem

For a simple polygon with **integer** vertices:
```
A = I + B/2 − 1
```
where `A` = area, `I` = interior lattice points, `B` = boundary lattice points.

**Boundary points on a segment** from `(x1,y1)` to `(x2,y2)`: `gcd(|dx|, |dy|)`.

**Use when:** "count lattice points inside a polygon". Compute `A` by shoelace, `B` by summing gcds, solve for `I`.

---

## Segment intersection

```cpp
bool onSeg(const P& a, const P& b, const P& p) {      // p collinear with ab, is it between?
    return min(a.x,b.x) <= p.x && p.x <= max(a.x,b.x)
        && min(a.y,b.y) <= p.y && p.y <= max(a.y,b.y);
}
bool segInt(const P& a, const P& b, const P& c, const P& d) {
    int d1 = orient(a, b, c), d2 = orient(a, b, d);
    int d3 = orient(c, d, a), d4 = orient(c, d, b);
    if (d1 * d2 < 0 && d3 * d4 < 0) return true;      // proper crossing
    if (d1 == 0 && onSeg(a, b, c)) return true;       // collinear touching cases
    if (d2 == 0 && onSeg(a, b, d)) return true;
    if (d3 == 0 && onSeg(c, d, a)) return true;
    if (d4 == 0 && onSeg(c, d, b)) return true;
    return false;
}
```
**All integer arithmetic.** The collinear cases are where everyone loses points — handle them explicitly.

---

## Convex hull (Andrew's monotone chain)

```cpp
vector<P> hull(vector<P> p) {
    sort(p.begin(), p.end());
    p.erase(unique(p.begin(), p.end()), p.end());
    int n = p.size();
    if (n < 3) return p;
    vector<P> h(2 * n);
    int k = 0;
    for (int i = 0; i < n; i++) {                     // lower hull
        while (k >= 2 && cross(h[k-2], h[k-1], p[i]) <= 0) k--;
        h[k++] = p[i];
    }
    for (int i = n - 2, t = k + 1; i >= 0; i--) {     // upper hull
        while (k >= t && cross(h[k-2], h[k-1], p[i]) <= 0) k--;
        h[k++] = p[i];
    }
    h.resize(k - 1);
    return h;                                          // counter-clockwise
}
```
**O(n log n)**, dominated by the sort. Integer-exact.
**`<= 0` vs `< 0`:** `<= 0` **removes** collinear points from the hull; `< 0` **keeps** them. Read the problem to decide which you need — this is a frequent WA.
**Trap:** deduplicate first, and handle n ≤ 2 separately.

---

## Point in polygon

**Convex polygon:** binary search by angle from vertex 0 — O(log n) per query.
**Any simple polygon:** ray casting — shoot a ray to the right and count crossings; odd = inside.

```cpp
bool inPoly(const vector<P>& poly, const P& q) {      // ray casting, O(n)
    int n = poly.size(); bool in = false;
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if ((poly[i].y > q.y) != (poly[j].y > q.y)) {
            // careful: this comparison needs rational/exact handling for integer input
            ll dx = poly[j].x - poly[i].x, dy = poly[j].y - poly[i].y;
            ll lhs = (q.y - poly[i].y) * dx;
            ll rhs = (q.x - poly[i].x) * dy;
            if (dy > 0 ? lhs > rhs : lhs < rhs) in = !in;
        }
    }
    return in;
}
```
**Trap:** points exactly **on** the boundary. Decide what the problem wants and test those separately with `onSeg`.

---

## Angles and sorting by angle

```cpp
// Sort points around the origin by polar angle, WITHOUT atan2
int half(const P& p) { return (p.y < 0 || (p.y == 0 && p.x < 0)) ? 1 : 0; }
sort(pts.begin(), pts.end(), [](const P& a, const P& b) {
    int ha = half(a), hb = half(b);
    if (ha != hb) return ha < hb;
    ll c = cross(a, b);
    if (c != 0) return c > 0;
    return dist2({0,0}, a) < dist2({0,0}, b);
});
```
**Never sort by `atan2`** if you can avoid it — it's slow and imprecise. The half-plane + cross-product comparator is exact.

**Trap:** this comparator must be a strict weak ordering. The `half` split is what makes it one (a pure cross-product comparison is not transitive across the full circle).

---

## When you must use floating point

- Circle intersections, distances you have to print, angles in radians.
- Use `long double` on Codeforces (`%Lf` for printf).
- See `24_FLOATING_POINT_AND_RANDOM.md` for EPS selection.
- Prefer comparing squared quantities, scaling by a constant to stay integral, or reformulating with rationals.

---

## Traps

```
[ ] cross product overflow with coordinates near 1e9
[ ] used sqrt when comparing distances
[ ] convex hull: <= vs < for collinear points
[ ] polygon area: forgot the closing edge (% n)
[ ] segment intersection: collinear/touching cases ignored
[ ] point exactly on the polygon boundary
[ ] sorted by atan2 and got a precision WA
[ ] angle comparator not a strict weak ordering
[ ] degenerate input: all points equal, all collinear, n = 1 or 2
[ ] triangle inequality with overflow (a + b > c on 1e18 values)
[ ] assumed the polygon is convex when it isn't
[ ] assumed CCW orientation when the input is CW
```

---

## Memory hooks

- *Cross = turn direction. Positive = counter-clockwise.*
- *Compare squared distances; never `sqrt` to compare.*
- *Shoelace gives twice the signed area — keep it as an integer.*
- *Integer input → integer predicates. Every `double` is a bug waiting.*
- *Pick: `A = I + B/2 − 1`; boundary points on a segment = `gcd(|dx|, |dy|)`.*