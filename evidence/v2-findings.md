# v2 findings — hardened image

Fill this in from REA's output (prompt 03) against your own `dump-v2.bin`, taken
from the board after flash encryption + Secure Boot were enabled.

## Image identity

- Chip target: _(should match v1)_
- Partition table: _(nvs / app offsets)_
- Flash encryption: **enabled** — _(evidence: nvs/app read as ciphertext)_
- Secure Boot v2: **enabled** — _(evidence, to the extent the dump shows it)_

## Re-test of every v1 finding

| v1 finding | Recoverable in v1? | Recoverable in v2? | Control responsible |
|---|---|---|---|
| Wi-Fi SSID | yes | | flash encryption (encrypted NVS) |
| Wi-Fi passphrase | yes | | flash encryption |
| MQTT username | yes | | flash encryption |
| MQTT password | yes | | flash encryption |
| MQTT host/topic | yes | | flash encryption |
| Cloud API key | yes | | runtime provisioning + flash encryption |
| OTA URL (unsigned) | yes | | signed OTA + Secure Boot v2 |
| `/diag/info` route | yes | | route removed in v2 |
| Admin config page | yes | | page removed / no default creds |
| ArduinoOTA open | yes | | password-gated / disabled in release |

Target result: every "Recoverable in v2?" cell says **no**.

## Evidence that partitions are encrypted

- nvs partition: _(entropy value / raw byte sample showing ciphertext)_
- app partition: _(same)_

## Residual notes

- Anything still readable (and why): _(if anything leaks, see lab-guide troubleshooting)_
- Trust anchor (CA/public key) present in image — expected, not a secret.

## The before/after table

_(This is the headline deliverable. Once both columns are filled, it's the proof
for your README before/after section and your LinkedIn post.)_

## REA tool calls cited

- _(list them)_
