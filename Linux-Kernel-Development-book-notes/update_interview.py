import re

with open('interview-questions.md', 'r') as f:
    content = f.read()

ch6_content = """
---

## Chapter 6 — Kernel Data Structures

**Basic**
1. What is `struct list_head` and how is it different from a traditional linked list node?
2. What does `container_of()` do, and why is it so important in kernel code?
3. Name three kernel subsystems that use red-black trees.

**Intermediate**
4. Explain how `container_of()` works using `offsetof()` pointer arithmetic. Walk through a concrete example.
5. Why is `list_for_each_entry_safe()` needed? When would using the non-safe version cause a crash?
6. Why must kfifo sizes be a power of 2? What advantage does this provide over modulo arithmetic?
7. Compare the kernel's "intrusive" linked list design (embedding list_head in structs) with the traditional approach (list nodes wrapping data pointers). What are the advantages?

**Advanced / Kernel-Development**
8. How are red-black tree insertion and deletion different from a normal BST? Why does the rbtree API split insertion into rb_link_node() + rb_insert_color()?
9. Explain why the kernel's linked list operations provide NO locking. How does this interact with the concurrency requirements from Ch.2 and the synchronization primitives from Ch.9-10?
10. How does kfifo achieve lock-free operation for the single-producer/single-consumer case?

**Design Question**
11. You're designing a subsystem that needs to track 100,000 objects by both a unique integer ID and a sorted key. Which kernel data structures would you combine, and how would you handle the memory/performance tradeoffs?

*(Full detail in `06-kernel-data-structures.md`, Interview Questions section.)*
"""

marker = "## How to Use This File for Review"
idx = content.find(marker)
if idx != -1:
    new_content = content[:idx] + ch6_content + "\n" + content[idx:]
    with open('interview-questions.md', 'w') as f:
        f.write(new_content)
else:
    print("Marker not found!")

