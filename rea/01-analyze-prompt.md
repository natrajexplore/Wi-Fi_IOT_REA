# REA prompt 01 — analyze the v1 firmware dump

Paste this to your coding agent (Claude Code, etc.) once REA is set up and you
have produced `dump.bin` from your own board with `scripts/dump-flash.sh`.

> Everything below targets a firmware image I built and dumped from my own
> ESP32. I wrote it to be intentionally weak so I can learn what an analyst
> would see. Only this local file is in scope.

---

## Prompt

```
I have a firmware image at ./dump.bin, dumped from an ESP32-WROOM-32 I own.
Use REA to investigate it and show me the evidence behind each finding.

1. Identify the image: chip target, flash layout, and the partition table.
   Locate the app partition(s) and the nvs partition.

2. Carve the readable strings from the app image and triage them. I want to
   know specifically whether any of these appear in plaintext, and where:
     - Wi-Fi SSID / passphrase
     - MQTT host, port, username, password, topic
     - any API key or token
     - URLs / endpoints (telemetry, OTA, diagnostics)
     - HTTP Basic-Auth / admin credentials

3. For each secret or endpoint you find, show the offset / section it lives in
   (e.g. .rodata) so I can see why it was recoverable.

4. Map the HTTP surface: list every route the firmware registers and note which
   ones have no authentication or disclose configuration.

5. Describe the OTA update mechanism and whether anything verifies the image
   before it is applied (signature, hash, TLS).

6. Summarize the attack surface as a table: finding | evidence location | why
   it matters | how an operator on the same LAN could abuse it.

Do not modify the image. Cite the REA tool calls you used for each conclusion.
```

---

## What good output looks like

You should get back, with evidence, at least:

- the full set of hardcoded credentials and the API key, pulled from `.rodata`
- the plaintext MQTT host/topic and the unsigned OTA URL
- the `/` config page (default admin) and the undocumented `/diag/info` route
- confirmation that OTA has no signature check

Record all of it in `evidence/v1-findings.md` with the exact offsets REA reports.
