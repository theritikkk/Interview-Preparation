# Elasticsearch

⚠️ Skills-list item, no project bullet — keep this brief and conceptual; it's the lowest-priority item in your database list for interview prep.

---

## Cluster 1: Inverted Index & When You Need It

### What is it?
Elasticsearch is built on an **inverted index**: instead of mapping documents → words (like a normal index), it maps words → the documents containing them, making full-text search (find all documents containing a term, ranked by relevance) fast — something a relational `LIKE '%term%'` query cannot do efficiently at scale (a `LIKE` with a leading wildcard can't use a standard B-tree index at all, forcing a full scan).

### Interview Questions
- **L1:** "Elasticsearch uses an inverted index (term → documents) to make full-text search and relevance ranking fast, which a standard relational index can't do efficiently for arbitrary substring/keyword search."
- **L2:** explain when you'd actually reach for it versus just adding a database index: structured, exact-match, or prefix queries are fine with a normal B-tree index (`03_CS_FUNDAMENTALS/DBMS.md`); free-text search, typo tolerance, relevance ranking, and faceted search are the signals that justify a dedicated search engine instead.

### Quick Revision
- Inverted index = word → documents, built for full-text search and ranking.
- Reach for Elasticsearch specifically for free-text/relevance-ranked search; a normal DB index handles structured/exact-match lookups fine.
