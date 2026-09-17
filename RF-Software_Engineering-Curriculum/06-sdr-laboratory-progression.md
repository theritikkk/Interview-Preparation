[← Table of Contents](README.md)

---

# 6. SDR Laboratory Progression

| Stage | What you do | Hardware needed |
|---|---|---|
| 1. Python simulation | Build every basic block (modulator, matched filter, AWGN channel) in NumPy before touching a framework | None |
| 2. GNU Radio Companion | Flowgraph-based blocks; understand scheduling/buffers; build simple flowgraphs (AM/FM demod) | None |
| 3. Recorded IQ datasets | Practice on real captured IQ files (public signal-identification datasets, ADS-B, POCSAG captures) | None |
| 4. SDR receive experiments | Decode real broadcast FM, ADS-B, POCSAG pagers, NOAA weather satellites | **RTL-SDR (~$25-35)** — receive-only, no license typically needed |
| 5. Transmission experiments (where legal) | Closed-loop TX/RX experiments | **HackRF One (~$300, half-duplex)** or **LimeSDR (~$300-600, full-duplex)** |
| 6. Real-time DSP | Move from "batch-process a file" to "process a live stream without dropping samples" | Same as above |
| 7. Custom C/C++ DSP blocks | Write your own GNU Radio out-of-tree blocks in C++ instead of relying on built-in Python blocks | Same as above |
| 8. Hardware-in-the-loop | Closed transmit→channel→receive loop; measure real BER curves | Two SDRs or one + a channel emulator, if available |

**When hardware becomes worth buying:** not until Stage 4. An RTL-SDR is cheap enough to get early anyway, but stages 1-3 genuinely need nothing but a laptop. A USRP (Ettus, $1,400+) is grad-lab-grade equipment most working PHY engineers don't personally own — it's provided by an employer or university lab; don't buy one for self-study.

**Legal/regulatory note (not legal advice):** *Receiving* is legal in essentially every country for the bands used in the exercises above. *Transmitting* generally requires either an amateur radio license (for ham bands) or staying strictly within license-exempt ISM bands at very low power — rules vary by country. In India specifically, radio transmission is regulated by the WPC (Wireless Planning & Coordination Wing); check current WPC/amateur-license rules before any TX experiment, even low-power ones.

---

[← 5. 3GPP Specification Reading Curriculum](05-3gpp-specification-reading-guide.md) · [7. Major Projects →](07-major-projects-and-portfolio.md) · [Table of Contents](README.md)
