# Redis

⚠️ Skills-list item, no project bullet — prepare conceptually; caching questions are extremely common regardless.

---

## Cluster 1: Data Structures & Use Cases

### What is it?
Redis is an in-memory key-value store with rich built-in data structures beyond plain strings: lists, sets, sorted sets (score-ordered — the basis for leaderboards), hashes, and more — each with O(1) or O(log n) operations, all served from memory for very low latency.

### Interview Questions
- **L1:** "Redis is an in-memory store with rich data structures — strings, lists, sets, sorted sets, hashes — commonly used for caching, session storage, rate limiting, and leaderboards, trading durability guarantees for speed."
- **L2:** explain a sorted set's use for a leaderboard specifically (`ZADD`/`ZRANGE` give O(log n) insert and ranked-range queries) versus doing the equivalent ranking query repeatedly against a relational table, which would need a full sort or a maintained index for the same access pattern.

---

## Cluster 2: TTL, Eviction, Cache-Aside Pattern

### What is it?
Keys can have a TTL (time-to-live), after which Redis expires them automatically — the basis for caching and rate-limiting windows. When memory fills up, an eviction policy (LRU, LFU, random, or none — reject writes instead) decides what to drop. The **cache-aside** pattern: on read, check the cache first; on a miss, read from the source of truth and populate the cache; on write, update the source of truth and invalidate (or update) the cache entry.

### How does it work?
```
GET requires reading task list:
  1. cached = redis.get('tasks:tenant:t1')
  2. if cached: return cached
  3. else: data = db.query(...); redis.setex('tasks:tenant:t1', ttl=60, data); return data
```

### Interview Questions
- **L1:** "Cache-aside checks Redis first, falls back to the source of truth on a miss and populates the cache; TTL and eviction policies (like LRU) control what stays cached under memory pressure."
- **L2:** explain **cache stampede** (many concurrent requests all miss the same just-expired key simultaneously and all hit the database at once) and a standard mitigation (a short lock/single-flight around the cache-population step, or staggered TTLs to avoid many keys expiring at the exact same moment).
- **L3:** explain the cache-invalidation problem generally — famously one of the two hard things in computer science — specifically: an update to the source of truth that forgets to invalidate/update the corresponding cache entry leaves stale data being served until TTL expiry, a real and common production bug class.

### Comparison With Alternatives
Redis (in-memory, very fast, limited durability by default) vs. Memcached (simpler, pure key-value, no rich data structures) vs. a database's own query cache — Redis's data-structure richness is usually the deciding factor when the use case is more than plain key-value caching (e.g., rate limiting needs atomic increment-with-expiry, which Redis supports natively).

### Quick Revision
- Cache-aside: check cache → miss → read source → populate cache.
- TTL + eviction policy (LRU/LFU) control what Redis keeps under memory pressure.
- Cache stampede: many simultaneous misses on one expired key — mitigate with a lock or staggered TTLs.
- Cache invalidation on write is the hard, easy-to-get-wrong part — not populating the cache.
