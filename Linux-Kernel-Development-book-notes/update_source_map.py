import re

with open('source-code-map.md', 'r') as f:
    content = f.read()

ch6_content = """
### Chapter 6's Contribution to This Map
Key files:
- `include/linux/list.h` — THE most important header for reading kernel code. Defines list_head and all list manipulation/iteration macros
- `include/linux/container_of.h` — container_of() macro (also historically in `include/linux/kernel.h`)
- `include/linux/rbtree.h` — red-black tree API (rb_node, rb_root, rb_link_node, rb_insert_color, rb_erase)
- `lib/rbtree.c` — red-black tree implementation
- `include/linux/kfifo.h` — kfifo API
- `kernel/kfifo.c` — kfifo implementation
- `include/linux/idr.h` — IDR/IDA integer-to-pointer mapping
- `include/linux/xarray.h` — [MODERN] XArray (replaced many radix tree/IDR uses)
- `lib/idr.c` — IDR implementation
- `include/linux/types.h` — struct list_head definition

Key structures: list_head, rb_node, rb_root, kfifo
Key macros: container_of(), list_for_each_entry(), list_for_each_entry_safe(), list_add(), list_del(), LIST_HEAD(), rb_entry()
Key functions: rb_link_node(), rb_insert_color(), rb_erase(), rb_first(), rb_next(), kfifo_alloc(), kfifo_in(), kfifo_out(), idr_alloc(), idr_find(), idr_remove()

Navigation commands:
```bash
rg -n "struct list_head" include/linux/types.h
rg -n "container_of" include/linux/container_of.h
rg -n "list_for_each_entry" include/linux/list.h
rg -c "struct list_head" include/linux/ | sort -t: -k2 -rn | head  # most list-heavy headers
rg -n "struct rb_node" include/linux/rbtree.h
```
"""

marker = "*(Every subsequent chapter appends its own section here"
idx = content.find(marker)
if idx != -1:
    new_content = content[:idx] + ch6_content + "\n" + content[idx:]
    with open('source-code-map.md', 'w') as f:
        f.write(new_content)
else:
    print("Marker not found!")

