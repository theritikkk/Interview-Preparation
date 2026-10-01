# 34 — Modern Boot and EFI

## Layer map for this file

```
┌──────────────────────────────────────────────────┐
│ 1. BOOK: none — file 07 already gave the         │
│    BIOS/UEFI/GPT overview; this file is the      │
│    depth it explicitly deferred                  │
├──────────────────────────────────────────────────┤
│ 2. CORE CONCEPT: Secure Boot's actual trust      │
│    chain, and measured boot as a genuinely       │
│    different, complementary mechanism            │
├──────────────────────────────────────────────────┤
│ 3. MODERN: this file IS the modern layer —       │
│    TPM attestation, the .efi/PE format           │
└──────────────────────────────────────────────────┘
```

Book coverage: **None.** File `07` named Secure Boot and promised this
file would explain it. This is that promise, kept.

## Prerequisites

Files `01`–`33`, especially `06`, `07`, `08`, `12`, `31`.

---

## [CORE CONCEPT] Secure Boot: a Chain of Signatures

File `07` established that UEFI's structured, executable-based boot
process is what makes signature checking possible at all — a raw BIOS
sector has no format to attach a signature to. Here's the actual chain:

![Secure Boot versus measured boot: Secure Boot's chain runs from the Platform Key through the Key Exchange Key to the signature database, checking every boot stage's signature before allowing it to run and refusing to boot on any unsigned or revoked binary; measured boot instead hashes firmware, bootloader, and kernel into TPM PCRs without blocking anything, creating a tamper-evident record a remote party can later check](images/34-secure-vs-measured-boot.svg)

The **Platform Key (PK)**, owned by the device manufacturer, signs
updates to the **Key Exchange Key (KEK)**, which in turn authorizes
updates to the **signature database (db)** — the actual list of
certificates/hashes UEFI will accept — and its counterpart **dbx**, an
explicit revocation list for things that *used* to be trusted and no
longer are (a certificate compromised after the fact, say). Every
`.efi` file UEFI loads — the bootloader, and anything it loads in turn
— must carry a valid signature traceable up this chain, or boot simply
refuses to continue. This is the concrete mechanism behind file `07`'s
"UEFI can cryptographically verify each stage" line.

## [CORE CONCEPT] Measured Boot: a Genuinely Different Idea

Easy to conflate with Secure Boot, and worth being precise about,
because the two solve different problems: Secure Boot **prevents**
unsigned code from running at all. **Measured boot** doesn't prevent
anything — it computes a cryptographic hash of each stage (firmware,
bootloader, kernel and its configuration) and records that hash into a
**TPM** (Trusted Platform Module)'s **PCRs** (Platform Configuration
Registers) *before* running that stage, regardless of whether it's
signed or trusted at all. The TPM is built so these registers can only
be *extended* (hashed together with new values), never reset or
overwritten outside a full reboot — a tamper-evident log of exactly
what actually booted. A remote party can later ask the TPM to
**attest** to these values, cryptographically, and know with
confidence whether the machine booted the software it expected to, even
if Secure Boot was bypassed, disabled, or simply wasn't the mechanism
in play at all. In practice the two are used together: Secure Boot
stops most bad actors outright; measured boot creates evidence for
whatever Secure Boot didn't stop.

## [CORE CONCEPT] .efi Files Are PE Executables

File `07` named `.efi` files as "PE-format executables" in passing.
Worth making concrete: **PE (Portable Executable)** is the same
executable format Windows `.exe` and `.dll` files use — UEFI firmware
is, in this specific sense, running Windows-format binaries at boot
time, regardless of what OS eventually takes over. File `12`'s deep
dive into ELF's sections, segments, and entry point has a direct PE
counterpart: PE has its own header, its own section table, and its own
named entry point field — a genuinely different format solving the
same problem ELF solves, for a different ecosystem.

---

## [RELATED KNOWLEDGE] EFI Variables, Properly

File `07`'s experiment had you run `efibootmgr -v` without much
explanation of what it was reading. UEFI firmware exposes **EFI
variables** — persistent, named key-value storage surviving reboots,
stored in NVRAM — and the boot entry list `efibootmgr` reads and
writes is just one such variable. `BootOrder` names the sequence;
`Boot0000`, `Boot0001`, and so on each describe one bootable target
(label, device path, the `.efi` file to run). This is also where
Secure Boot's own `PK`/`KEK`/`db`/`dbx` values are actually stored.

---

## [MODERN SYSTEM] Where This Shows Up in Practice

Cloud providers and enterprise fleets use remote attestation (measured
boot, properly) to verify a machine booted an expected, unmodified
software stack before trusting it with sensitive workloads or keys —
confirming the fact, cryptographically, rather than merely hoping
Secure Boot alone was sufficient. Full-disk encryption tools (BitLocker,
and `systemd-cryptenroll`'s TPM integration on Linux) commonly bind the
disk's decryption key to specific PCR values, so the disk only unlocks
automatically if the measured boot chain matches exactly what was
recorded when the key was first set up — change the bootloader or
kernel, even subtly, and the automatic unlock fails, by design.

---

## Practical Experiment

1. `mokutil --sb-state` (if available) — confirm whether Secure Boot is
   active on your own machine right now.
2. If you have TPM tools available: `tpm2_pcrread` lists current PCR
   values directly — a live look at exactly what file `34`'s measured
   boot diagram is describing.
3. Revisit file `07`'s `efibootmgr -v` output and this time identify
   `BootOrder` and match it against the individual `Boot####` entries
   listed below it.
4. Find a `.efi` file (on a Linux system with an ESP, usually under
   `/boot/efi/EFI/`) and run `file` on it — confirm it reports as a PE
   executable, the same format identification `file` gives a Windows
   `.exe`.

## Debugging

"Secure Boot blocked this from booting" is a specific, diagnosable
failure, not a generic one: the chain above means the failure is at
exactly one link — an unsigned kernel or bootloader, a certificate not
present in `db`, or a binary explicitly listed in `dbx`. `mokutil`
(Machine Owner Key utility, on Linux) exists specifically to manage
additional trusted keys for exactly this situation — self-signed kernel
modules being the most common real case.

## Source Code

None new — this file's artifacts are read directly from your own
machine's firmware state.

---

## Common Misconceptions

- **"Secure Boot and measured boot are the same feature with different
  names."** Genuinely different mechanisms — one prevents, one records;
  used together, not interchangeably.
- **"Disabling Secure Boot disables all boot integrity checking."**
  Measured boot (if configured) still records exactly what ran,
  independent of whether Secure Boot itself is on.
- **".efi files are a UEFI-specific, exotic format."** They're ordinary
  PE executables — the same format as a Windows `.exe`.

## Interview Questions

**Basic:** What's the practical difference between Secure Boot and
measured boot?
**Intermediate:** What does the PK → KEK → db/dbx chain each step
actually authorize?
**Advanced:** Why can a TPM's PCR values be extended but never
arbitrarily reset, and why does that property matter?
**Systems Design:** Why might a full-disk-encryption tool bind its
automatic unlock to specific PCR values instead of just trusting Secure
Boot alone?
**Debugging:** A custom kernel module fails to load with Secure Boot
enabled — what's the actual cause, and what tool addresses it?
**Explain what happens when...:** a machine with both Secure Boot and
TPM-bound disk encryption boots a bootloader that's been tampered with?

## Active Recall

1. What's the one-sentence difference between Secure Boot and measured boot?
2. What do PK, KEK, and db/dbx each do in the Secure Boot chain?
3. What format are `.efi` files, and what else uses that same format?
4. What's a PCR, and what property makes it useful for integrity evidence?

## Answers

1. Secure Boot prevents unsigned/untrusted code from running at all;
   measured boot records what ran into a TPM without blocking anything.
2. PK authorizes updates to KEK; KEK authorizes updates to the
   signature database (db) and revocation list (dbx), which together
   decide what's allowed to boot.
3. PE (Portable Executable) — the same format used by Windows `.exe`
   and `.dll` files.
4. A Platform Configuration Register in a TPM; it can only be extended
   (hashed with new data), never arbitrarily reset, making its final
   value tamper-evident proof of exactly what ran before it.

---

## What You Should Now Understand

- The actual Secure Boot trust chain, finally explained after file
  `07` only named it.
- Measured boot as a genuinely distinct mechanism — evidence, not
  prevention — and why real systems use both together.
- That `.efi` files are ordinary PE executables, the same format
  Windows itself uses.
- Where EFI variables, `efibootmgr`, and TPM-bound disk encryption all
  fit into this picture.

---

**Next:** file `35-ebpf-and-kernel-observability.md`.
