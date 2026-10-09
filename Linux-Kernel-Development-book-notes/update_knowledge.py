import re

with open('final-knowledge-map.md', 'r') as f:
    content = f.read()

# 1. Update Mermaid graph
mermaid_additions = """    Build --> DataStructures["Kernel Data Structures\\n(list_head, rbtree, kfifo, idr)"]
    DataStructures --> ContainerOf["container_of()\\n(the key to reading kernel code)"]
    DataStructures --> TaskStruct
    DataStructures --> Sched"""

# Find the end of the mermaid graph
marker_mermaid_end = "```\n\n*(Future chapters attach here"
idx_mer = content.find(marker_mermaid_end)
if idx_mer != -1:
    content = content[:idx_mer] + mermaid_additions + "\n" + content[idx_mer:]
else:
    print("Mermaid end not found!")

# 2. Add Chapter 6 text tree
ch6_branch = """
---

## Chapter 6 Branch — Detail

```
Kernel Data Structures (Ch.6)
├── Linked Lists (list_head)
│   ├── Doubly linked, circular
│   ├── Embedded inside data structs (not wrapping them)
│   ├── container_of() — recover enclosing struct from member pointer
│   ├── list_for_each_entry() / _safe() iteration
│   └── No locking → caller must synchronize (Ch.9-10)
├── Queues (kfifo)
│   ├── FIFO ring buffer
│   ├── Lock-free for single-producer/single-consumer
│   └── Power-of-2 size for bitmask wrap
├── Maps (idr / XArray)
│   ├── Integer ID → pointer mapping
│   └── File descriptors, timer IDs, IPC IDs
├── Trees (rbtree)
│   ├── Self-balancing BST, O(log N)
│   ├── Embedded rb_node (same pattern as list_head)
│   └── Used by: CFS scheduler, VMA tree, I/O schedulers
└── Choosing the Right Structure
    ├── Ordered iteration → rbtree
    ├── Fast add/remove → linked list
    ├── FIFO → kfifo
    └── ID mapping → idr/xarray
```
"""

marker_deps = "## Cross-Chapter Dependency Notes"
idx_deps = content.find(marker_deps)
if idx_deps != -1:
    content = content[:idx_deps] + ch6_branch + "\n" + content[idx_deps:]
else:
    print("Deps marker not found!")

# 3. Add rows to table
table_rows = """| Kernel data structures | GNU C extensions (Ch.2, for container_of/typeof) | virtually every later chapter |
| list_head | nothing specific | Process management (Ch.3), scheduling (Ch.4), interrupts (Ch.7), synchronization (Ch.9-10), memory (Ch.12), VFS (Ch.13), modules (Ch.17) |
| rbtree | nothing specific | Scheduling (Ch.4), memory management (Ch.12, 15), block I/O (Ch.14) |
"""

marker_table_end = "*(This table is the machine-checkable version"
idx_tab = content.find(marker_table_end)
if idx_tab != -1:
    content = content[:idx_tab] + table_rows + "\n" + content[idx_tab:]
else:
    print("Table end marker not found!")

with open('final-knowledge-map.md', 'w') as f:
    f.write(content)

