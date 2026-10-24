# REA prompt 03 — verify the hardened image

Run this after you have flashed `v2-hardened`, enabled flash encryption and
Secure Boot per `docs/lab-guide.md`, and produced a fresh `dump-v2.bin` from the
hardened board.

> Scope: same as before — a firmware image I built and dumped from my own ESP32.

---

## Prompt

```
I have a second firmware image at ./dump-v2.bin from the same ESP32, now built
with flash encryption and Secure Boot enabled. Using REA, verify the hardening:

1. Confirm the chip target and partition layout match the v1 dump.

2. Attempt the SAME string triage you did on v1. For each v1 finding, report
   whether it is still recoverable:
     - Wi-Fi SSID / passphrase
     - MQTT host / user / pass / topic
     - API key / token
     - OTA URL, /diag/info route, admin creds
   I expect the nvs and app partitions to be ciphertext now. Show me the entropy
   or the raw bytes that demonstrate they are encrypted rather than readable.

3. Check whether the app image is signed / whether Secure Boot metadata is
   present, to the extent the dump reveals it.

4. Produce a side-by-side table: v1 finding | recoverable in v1? | recoverable
   in v2? | control responsible.

Cite the REA tool calls for each conclusion. Do not modify either image.
```

---

## What good output looks like

- every plaintext secret from v1 is **no longer recoverable** in v2
- the nvs/app partitions read as high-entropy ciphertext, not strings
- no `/diag/info`, no default-admin page, no unsigned OTA URL in the clear

Paste the side-by-side table into `evidence/v2-findings.md`. That table is the
deliverable — it is the before/after proof for your write-up and LinkedIn post.

If any secret is *still* readable, the hardening is incomplete. The usual cause
is flashing in development mode or skipping the flash-encryption eFuse step —
see the troubleshooting section of `docs/lab-guide.md`.
