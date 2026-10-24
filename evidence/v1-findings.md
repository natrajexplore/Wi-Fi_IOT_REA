# v1 findings — intentionally vulnerable image

Fill this in from REA's output (prompt 01) against your own `dump.bin`. Paste the
exact offsets/sections REA reports — that evidence is what makes the write-up
credible.

## Image identity

- Chip target: _(e.g. ESP32-D0WD, 4 MB flash)_
- Partition table: _(nvs / otadata / app0 / app1 / spiffs offsets)_
- App partition analyzed: _(offset, size)_

## Recovered secrets and endpoints

| Finding | Value (redact if needed) | Evidence location | Why it matters |
|---|---|---|---|
| Wi-Fi SSID | | `.rodata` @ 0x…… | network identification |
| Wi-Fi passphrase | | `.rodata` @ 0x…… | network access |
| MQTT host:port | | | plaintext broker target |
| MQTT username | | | credential |
| MQTT password | | | credential — enables impersonation |
| MQTT topic | | | tells attacker what to publish |
| Cloud API key | | `.rodata` @ 0x…… | shipped in every message too |
| Telemetry endpoint | | | hidden endpoint |
| OTA update URL | | | **unsigned** update path |
| `/diag/info` route | | registered in web handlers | config disclosure |
| Admin creds (config page) | | HTTP Basic | default `admin:admin` |

## HTTP surface

| Route | Method | Auth? | Notes |
|---|---|---|---|
| `/` | GET | Basic (default creds) | prints broker, topic, API key |
| `/ota` | POST | none | triggers unsigned OTA |
| `/diag/info` | GET | none | dumps running config |

## OTA mechanism

- Update source: _(URL from image)_
- Signature / hash verification: **none** — _(evidence)_
- ArduinoOTA password: **not set** — _(evidence)_

## Proven impact (from prompt 02 / attack-mqtt.py)

- [ ] Reused extracted MQTT creds to publish `temp_c=999` to my own broker
- Dashboard result: _(screenshot path / description)_
- Broker log line: _(paste)_

## Severity ranking

1. _(highest)_
2.
3.

## REA tool calls cited

- _(list the rea analyze/decompile/string calls behind each conclusion)_
