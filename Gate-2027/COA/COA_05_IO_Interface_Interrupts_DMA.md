# Computer Architecture → I/O Interface (Interrupts & DMA)

**GATE 2027 syllabus:** Computer Organization and Architecture — I/O interface (interrupt and DMA).
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** Polling: the CPU repeatedly checks a device's status register. Interrupt-driven I/O: the device signals the CPU only when ready, incurring a fixed overhead per interrupt (saving/restoring context plus ISR entry). DMA (Direct Memory Access): a DMA controller transfers data directly between device and memory without CPU involvement per byte/word, after an initial setup by the CPU; "cycle stealing" means the DMA controller takes individual memory-bus cycles away from the CPU during the transfer.

---

## Questions

### Q1 · MCQ · Fundamental
Compared to polling, the main advantage of interrupt-driven I/O is that the CPU

- (A) never needs to execute any I/O-related code
- (B) does not waste cycles repeatedly checking a device that isn't ready
- (C) can transfer data faster per byte
- (D) does not need an interrupt controller

---

### Q2 · NAT · Fundamental
An interrupt-driven I/O scheme handles 1000 interrupts, each costing 200 clock cycles of overhead (context save/restore plus ISR entry/exit), on a 1 GHz processor. The total overhead time, in microseconds, is ______.

---

### Q3 · MSQ · Standard
Which of the following statements about DMA is/are TRUE?

- (A) DMA is generally used for high-volume, block-oriented I/O transfers (e.g., disk-to-memory), where the per-transfer overhead of interrupts would be too costly.
- (B) During "cycle stealing" DMA, the DMA controller and CPU can both need the memory bus, and the DMA controller "steals" individual bus cycles the CPU would otherwise have used.
- (C) DMA transfers require the CPU to be involved in transferring every single byte or word.
- (D) A DMA controller typically has its own registers for the memory address, byte count and control status, set up by the CPU before the transfer begins.

---

### Q4 · NAT · Standard
A DMA controller transfers 2,000,000 bytes at an effective rate of 40,000,000 bytes/second. The transfer time, in milliseconds, is ______.

---

### Q5 · MCQ · Standard
"Cycle stealing" DMA, compared to "burst mode" DMA (which transfers an entire block before releasing the bus), generally

- (A) blocks the CPU from the bus for a longer continuous period per transfer
- (B) interleaves DMA and CPU bus accesses, giving the CPU more responsiveness at the cost of a somewhat longer total DMA transfer time
- (C) is always faster for the DMA transfer itself
- (D) requires no bus arbitration at all

---

### Q6 · NAT · Moderate
A polling loop costs 5 cycles to check the device status, and useful data-transfer work costs 1 cycle, per byte transferred (the polling check happens exactly once per byte, since the device is always found ready immediately in this idealized scenario). For transferring 1000 bytes this way, the total cycle count is ______.

---

### Q7 · MSQ · Moderate
Which of the following statements about interrupt handling is/are TRUE?

- (A) Multiple interrupt sources can be prioritized so that a higher-priority interrupt can preempt the servicing of a lower-priority one (nested interrupts).
- (B) A vectored interrupt scheme lets the interrupting device directly identify (via a supplied vector/address) which interrupt service routine should run, avoiding a software polling loop to find the source.
- (C) Daisy-chaining interrupt lines is one hardware technique to establish a fixed priority ordering among devices sharing an interrupt request line.
- (D) An interrupt can never occur while the CPU is already executing an interrupt service routine, under any circumstances.

---

### Q8 · NAT · Moderate
A DMA transfer moves 100,000 bytes, stealing exactly 1 CPU clock cycle per byte transferred, on a CPU running at 1 GHz that would otherwise need exactly 1,000,000 cycles to complete some unrelated computation concurrently with the transfer. The total time (in ms) for the CPU's computation to complete, accounting for the stolen cycles, is ______  (round to two decimal places).

---

### Q9 · MSQ · Hard
Which of the following statements about memory-mapped I/O vs. isolated (port-mapped) I/O is/are TRUE?

- (A) Memory-mapped I/O uses the same address space for both memory and I/O device registers, so ordinary load/store instructions can access devices.
- (B) Isolated I/O uses separate address spaces for memory and I/O, typically requiring dedicated IN/OUT-style instructions.
- (C) Memory-mapped I/O can reduce the number of distinct instruction types needed in the ISA, since no special I/O instructions are required.
- (D) Isolated I/O always provides strictly more total addressable I/O device registers than memory-mapped I/O could, for any given address-bus width.

---

### Q10 · NAT · Killer
Three methods are compared for transferring 1000 bytes, one event per byte: **polling** costs (5 + 1) cycles per byte; **interrupt-driven** costs (10 + 1) cycles per byte; **DMA** costs a fixed 50-cycle setup plus 0.1 cycles per byte. The **difference** between the total cycles for the most expensive method and the total cycles for the least expensive method, among these three, is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | B | Interrupts avoid wasted polling cycles |
| 2 | 200 | 1000 × 200 = 200,000 cycles; at 10⁹ cycles/s that's 2×10⁻⁴ s = 200 µs |
| 3 | A, B, D | (C) is false — that is precisely what DMA avoids |
| 4 | 50 | 2,000,000 / 40,000,000 s = 0.05 s |
| 5 | B | Cycle stealing trades some DMA speed for CPU responsiveness |
| 6 | 6000 | 1000 × (5 + 1) |
| 7 | A, B, C | (D) is false — nested interrupts allow a higher-priority interrupt to occur even during ISR execution |
| 8 | 1.10 | Total cycles = 1,000,000 + 100,000×1 = 1,100,000; time = 1,100,000 / 10⁹ s = 1.1 ms |
| 9 | A, B, C | (D) is false — there's no such universal guarantee; it depends on the specific address-space sizes chosen in each design |
| 10 | 4850 | Polling: 6000; Interrupt: 11000; DMA: 50 + 100 = 150; most expensive − least expensive = 11000 − 150 |
