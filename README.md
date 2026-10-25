# Hack My Own IoT

An **ESP32 build → break → harden** security lab, driven by [REA (Reverse Engineer Anything)](https://github.com/morluto/rea).

You build a deliberately insecure IoT device, dump its firmware, let an AI agent reverse engineer it with REA, prove the impact, then harden the device and show the same attack fail.

> ⚠️ **Everything here runs on hardware and code you own.** Only analyze firmware you built yourself. Do not point the attack scripts at anyone else's broker or dashboard.

---

## The loop

```
  ┌─────────────┐     ┌──────────────┐     ┌─────────────┐     ┌──────────────┐
  │  1. BUILD   │ ──▶ │  2. BREAK     │ ──▶ │ 3. HARDEN   │ ──▶ │ 4. RE-ATTACK │
  │  v1 firmware│     │ dump + REA +  │     │ v2 firmware │     │ dump + REA   │
  │ (vulnerable)│     │ prove impact  │     │ (secure)    │     │ (secrets gone)│
  └─────────────┘     └──────────────┘     └─────────────┘     └──────────────┘
```

| Phase | What you do | Folder |
|-------|-------------|--------|
| 1. Build | Flash the intentionally weak sensor firmware | `firmware/v1-vulnerable/` |
| 2. Break | `esptool read_flash` → feed `dump.bin` to REA → extract secrets → publish fake data | `rea/`, `scripts/` |
| 3. Harden | Move secrets to encrypted NVS, enable flash encryption, Secure Boot, signed OTA, TLS | `firmware/v2-hardened/` |
| 4. Re-attack | Dump v2, run REA again, confirm the findings are gone | `rea/`, `evidence/` |

---

## Target hardware

Default: **ESP32-WROOM-32 (classic)**, Arduino framework.

Adjust for your board:

| Board | Flash dump size | Notes |
|-------|----------------|-------|
| ESP32 classic / WROOM-32 | `0x400000` (4 MB) | 2.4 GHz only |
| ESP32-S3 | `0x800000`+ | USB-native, larger flash common |
| ESP32-C3 / C6 | `0x400000` | RISC-V core, C6 adds Wi-Fi 6 |
| ESP32-C5 | `0x400000` | Dual-band 2.4/5 GHz |

Check your actual size first: `esptool.py flash_id`

---

## Prerequisites

- Arduino IDE (or `arduino-cli`) with the **esp32 board package**
- Python 3 + `esptool` and `paho-mqtt`: `pip install esptool paho-mqtt`
  - On Windows, if the pip scripts dir isn't on PATH, invoke esptool as `python -m esptool`.
  - Optional: `pip install amqtt` to run the no-hardware break-loop check (see below).
- A **local MQTT broker** you control (Mosquitto is fine) — a ready config is in `broker/mosquitto.conf`
- `mosquitto-clients` for the live view (`scripts/dashboard.sh`)
- REA installed **only** from the official source — `npx skills add morluto/rea` — and `npx -y rea-agents setup --yes`
- A coding agent (Claude Code, etc.) with REA loaded

---

## Quick start

1. Start your broker: `mosquitto_passwd -c broker/passwd sensor01` then `mosquitto -c broker/mosquitto.conf -v`.
2. In another terminal, watch the feed: `scripts/dashboard.sh`.
3. Copy `firmware/secrets.example.h` → `firmware/v1-vulnerable/secrets.h` and fill in **throwaway** test values (never your real home Wi-Fi password).
4. Flash `firmware/v1-vulnerable/v1-vulnerable.ino`.
5. Run `scripts/dump-flash.sh` → produces `dump.bin`.
6. Open your agent and paste `rea/01-analyze-prompt.md`. Record findings in `evidence/v1-findings.md`.
7. Run `scripts/attack-mqtt.py` to prove the extracted creds work against your own broker — the spoofed readings show up in the dashboard.
8. Provision the hardened device: flash `firmware/provision/provision.ino` once, then flash `firmware/v2-hardened/`. Follow `docs/lab-guide.md` for flash encryption + Secure Boot.
9. Re-dump, re-run `rea/03-verify-hardened-prompt.md`, fill `evidence/v2-findings.md`.
10. Publish the before/after table.

**No hardware yet?** You can verify the break-phase tooling offline — no board, no
Mosquitto — with a pure-Python broker:

```
pip install amqtt paho-mqtt
python scripts/verify-break-loop.py
```

It stands up a local broker that *requires* auth, confirms a wrong password is
rejected, then runs `attack-mqtt.py` with the v1 creds and checks the spoofed
`temp_c=999` messages reach a subscriber. Expect `RESULT: PASS`.

Full walkthrough: [`docs/lab-guide.md`](docs/lab-guide.md).

---

## Repo layout

```
hack-my-own-iot/
├── README.md
├── SECURITY-CHECKLIST.md         # the deliverable: ESP32 IoT security checklist
├── firmware/
│   ├── secrets.example.h         # template — copy to secrets.h (gitignored)
│   ├── v1-vulnerable/v1-vulnerable.ino
│   ├── v2-hardened/v2-hardened.ino
│   └── provision/provision.ino   # one-time NVS provisioning for v2
├── broker/
│   ├── mosquitto.conf            # local lab broker (auth required; TLS block for v2)
│   └── make-certs.sh             # lab CA + broker cert for the TLS phase
├── rea/
│   ├── 01-analyze-prompt.md      # ask REA what the firmware exposes
│   ├── 02-attack-prompt.md       # map findings to impact
│   └── 03-verify-hardened-prompt.md
├── scripts/
│   ├── dump-flash.sh             # esptool flash dump (esptool v5; Windows-aware)
│   ├── attack-mqtt.py           # publish fake data with extracted creds (your broker only)
│   ├── dashboard.sh             # live view of the telemetry feed
│   └── verify-break-loop.py     # optional: prove the break loop offline (pure-Python broker)
├── evidence/
│   ├── v1-findings.md            # fill from REA output
│   └── v2-findings.md
└── docs/
    └── lab-guide.md             # step-by-step, incl. eFuse/Secure Boot warnings
```

## License

MIT. Educational use on your own hardware only.
