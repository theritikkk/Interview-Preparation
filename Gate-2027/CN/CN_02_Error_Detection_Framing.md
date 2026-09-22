# Computer Networks → Error Detection & Framing (CRC, Checksum, Bit Stuffing)

**GATE 2027 syllabus:** Computer Networks — data link layer: error detection.
**Set:** 10 questions (MCQ / MSQ / NAT). Difficulty ramps from fundamental (Q1–Q2) to a killer conceptual question (Q10).
**Conventions:** CRC uses modulo-2 (XOR) polynomial division; the generator is written as a bit string, e.g. 1101 represents x³ + x² + 1. The internet checksum is 16-bit one's-complement addition with end-around carry, then complemented. Bit stuffing with flag 01111110 inserts a 0 after every five consecutive 1s in the data.

---

## Questions

### Q1 · MCQ · Fundamental
A CRC generator polynomial has degree r. The number of redundant (check) bits appended to the message is

- (A) r
- (B) r + 1
- (C) 2r
- (D) r − 1

---

### Q2 · NAT · Fundamental
A generator is 1101 (degree 3). An 8-bit message is transmitted with CRC. The total length of the transmitted codeword, in bits, is ______.

---

### Q3 · NAT · Standard
Using generator G = 1101, the CRC remainder (3 bits) appended to the message 1101011011 is ______  (give the 3-bit remainder as a decimal number).

---

### Q4 · MSQ · Standard
A frame is transmitted with a valid CRC using generator G = 10011 (degree 4). Which of the following single-event corruptions of the transmitted bits is/are **guaranteed** to be detected by the CRC check, regardless of which bits are affected?

- (A) A single-bit error anywhere in the frame
- (B) A burst error of length 4 (4 consecutive bits, at least the first and last of the burst are flipped)
- (C) A burst error of length 5
- (D) An odd number of bit errors, given that G contains the factor (x + 1)

---

### Q5 · NAT · Standard
Two 16-bit words, 0x6E5A and 0x9B3C, are added using one's-complement (internet) checksum arithmetic (with end-around carry). The 16-bit sum **before** complementing, as a hexadecimal number, is ______  (write the 4 hex digits with no "0x" prefix, e.g. 1A2B).

---

### Q6 · NAT · Moderate
Generator G = 10011 (degree 4). Among all possible error patterns of length exactly 5 (a burst confined to 5 consecutive bit positions, with the first and last position necessarily in error, and the 3 middle positions arbitrary — 8 patterns in total), the number that are **not** detected by this CRC is ______.

---

### Q7 · NAT · Moderate
A code consists of the four codewords 0000000, 0011110, 1100110 and 1111000. The maximum number of bit errors this code is guaranteed to **correct** is ______.

---

### Q8 · NAT · Moderate
After bit stuffing (insert a 0 after every five consecutive 1s), the 41-bit data string 01111101111110111111111011111011111100111 becomes a string of length ______ bits.

---

### Q9 · MSQ · Moderate
Which of the following statements about the internet (one's-complement) checksum is/are TRUE?

- (A) It is computed by summing 16-bit words with end-around carry and then taking the one's complement.
- (B) It can detect all single-bit errors.
- (C) It can detect all errors that change an odd number of bits.
- (D) Swapping two 16-bit words within the data being checksummed is always detected.

---

### Q10 · NAT · Killer
A file is transmitted using a **2-D parity** scheme: the bits are arranged in a 5 × 5 grid, and a parity bit is added to every row and every column (even parity), giving a 6 × 6 grid of transmitted bits (the corner bit checks overall parity). The number of ways to flip exactly 4 of the 25 **data** bits (not the parity bits) so that **all** row and column parity checks still pass is ______.

---

## Answer Key

| Q | Answer | Quick check |
|---|--------|-------------|
| 1 | A | Degree r generator gives r check bits |
| 2 | 11 | Codeword = message + r check bits = 8 + 3 |
| 3 | 5 | Polynomial division gives remainder 101₂ = 5 |
| 4 | A, B, C, D | Degree-4 CRC catches all single-bit and all burst errors of length ≤ 4; length-5 bursts are caught unless the pattern itself is a multiple of G (this G catches all of them here) |
| 5 | 0997 | 0x6E5A + 0x9B3C = 0x10996; wrap the carry bit in: 0x0996 + 0x1 = 0x0997 |
| 6 | 1 | Exactly one of the 8 length-5 burst patterns equals a multiple of G and slips through |
| 7 | 1 | Minimum Hamming distance is 4, so ⌊(4−1)/2⌋ = 1 |
| 8 | 46 | Five separate runs of five or more 1s each get one stuffed 0 |
| 9 | A, B, D | (C) is false: an even number of compensating changes within one word can cancel |
| 10 | 100 | Flipping 4 data cells preserves all row/column parities exactly when they form the 4 intersection points of 2 chosen rows and 2 chosen columns: C(5,2) × C(5,2) = 10 × 10 |
