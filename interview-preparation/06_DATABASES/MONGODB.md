# MongoDB

⚠️ Skills-list item, no project bullet — your hands-on NoSQL experience is DynamoDB (Hermes); prepare this conceptually and be ready to say so honestly.

---

## Cluster 1: Document Model & Schema Flexibility

### What is it?
MongoDB stores JSON-like documents (BSON) in collections — no fixed schema is enforced by the database itself (though schema validation rules can be added). Related data is often **embedded** within one document (denormalized, like a blog post with its comments nested inside) rather than split across tables and joined, when that data is always accessed together.

### How does it work?
```js
// Embedding (denormalized) — good when comments are always read with the post
{ _id: 1, title: "...", comments: [{ author: "a", text: "..." }, { author: "b", text: "..." }] }

// Referencing (normalized-ish) — good when the referenced data is large, shared, or accessed independently
{ _id: 1, title: "...", authorId: ObjectId("...") }
```

### Interview Questions
- **L1:** "MongoDB stores flexible, schema-less JSON-like documents; related data can be embedded (denormalized, for data always read together) or referenced (normalized-ish, for shared/independently-accessed data) — the choice is a modeling decision, not automatic."
- **L2:** explain the real cost of "schema-less": flexibility at write time shifts validation responsibility to the application layer (or optional schema-validation rules) — a document missing an expected field doesn't error at the database level the way a relational `NOT NULL` constraint would, which is a genuine tradeoff, not a pure win.
- **L3:** contrast directly with your actual hands-on experience: DynamoDB (`06_DATABASES/DYNAMODB.md`) also lacks joins and pushes you toward access-pattern-first modeling, but its key-value/wide-column model (fixed partition/sort key structure) is meaningfully different from MongoDB's fully flexible document model with rich embedded/nested query capabilities (`$lookup` for joins, aggregation pipelines) — know this distinction precisely if asked to compare the two, rather than lumping them together as "both NoSQL."

### DO NOT SAY THIS
"MongoDB has no schema at all, ever." — Modern MongoDB supports optional JSON Schema validation rules per collection; "schema-less" describes the *default*, not an absolute limitation.

### Quick Revision
- Embed for data always accessed together; reference for shared/large/independently-accessed data.
- Schema flexibility shifts validation responsibility to the application layer by default.
- DynamoDB and MongoDB are both "NoSQL" but structurally different — don't conflate them if asked to compare.
