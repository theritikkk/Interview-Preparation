import re

with open('debugging-playbook.md', 'r') as f:
    content = f.read()

ch6_content = """
## Scenario from Ch.6: List Corruption / General Protection Fault in list operations

| Step | Content |
|---|---|
| **Symptom** | Kernel oops/panic with a stack trace pointing to list_add(), list_del(), or list_for_each_entry(). Error message may include "list_add corruption" or "list_del corruption" |
| **Hypotheses** | (1) double list_del() on the same entry (poisoned pointers caught); (2) concurrent list manipulation without proper locking; (3) use-after-free of a struct containing a list_head; (4) memory corruption overwriting list_head pointers |
| **Tools** | CONFIG_DEBUG_LIST (enables runtime list integrity checking), KASAN (catches use-after-free), stack trace analysis, git blame the function in the stack trace |
| **Fix** | Add proper locking (spinlock or mutex) around list operations; ensure entries are removed from lists before being freed; use list_del_init() instead of list_del() if re-insertion is possible |

## Scenario from Ch.6: Kernel Module Crashes on Iteration After Deletion

| Step | Content |
|---|---|
| **Symptom** | A kernel module iterates a linked list and deletes entries during iteration. The module crashes with a null pointer dereference or accesses freed memory |
| **Hypotheses** | Using list_for_each_entry() instead of list_for_each_entry_safe() during deletion, causing the iterator to follow a poisoned next pointer |
| **Tools** | Code review, CONFIG_DEBUG_LIST, KASAN |
| **Fix** | Always use list_for_each_entry_safe() when removing elements during iteration. The _safe variant saves the next pointer before the loop body executes |
"""

marker = "*(This file will grow substantially starting with Chapter 7"
idx = content.find(marker)
if idx != -1:
    new_content = content[:idx] + ch6_content + "\n" + content[idx:]
    with open('debugging-playbook.md', 'w') as f:
        f.write(new_content)
else:
    print("Marker not found!")

