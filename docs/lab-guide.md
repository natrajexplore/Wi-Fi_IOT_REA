# Lab guide — Hack My Own IoT

A step-by-step run through the full loop: **build → break → harden → re-attack**.
Everything targets hardware and software you own.

> ⚠️ **Read the eFuse warning in step 5 before you enable flash encryption or
> Secure Boot.** Those steps burn one-time fuses and can permanently brick a
> board. Do them on a *spare* ESP32 first, in development mode.

---

## 0. What you need

- An ESP32 board you own (default target: ESP32-WROOM-32 classic). An S3/C3/C6/C5
  works too — only the flash dump size changes.
- Arduino IDE or `arduino-cli` with the **esp32 board package** installed.
- Python 3 with `esptool` and `paho-mqtt`:  `pip install esptool paho-mqtt`
- A **local MQTT broker you control** (Mosquitto is ideal) and, optionally, a
  simple dashboard (MQTT Explorer, Node-RED, or `mosquitto_sub`).
- REA, installed **only** from the official source:
  `npx skills add morluto/rea` then `npx -y rea-agents setup --yes`.
  Verify with `npx -y rea-agents doctor`.
- A coding agent (Claude Code, etc.) with REA loaded.
- A **throwaway test Wi-Fi** (phone hotspot or spare router). Never put your real
  home Wi-Fi password in `secrets.h`.

---

## 1. Build the vulnerable device (v1)

1. `cp firmware/secrets.example.h firmware/v1-vulnerable/secrets.h`
2. Edit `secrets.h` with **throwaway** values pointing at your local broker.
3. Open `firmware/v1-vulnerable/v1-vulnerable.ino`, install the **PubSubClient**
   library, select your board + port, and flash it.
4. Open the Serial Monitor at 115200. You'll see it connect and start publishing
   — and (by design) leak its secrets in the debug log. That's bad habit #5.
5. Confirm telemetry arrives at your broker:
   `mosquitto_sub -h 192.168.1.50 -t 'lab/sensor01/telemetry' -v`

At this point you have a working, deliberately-weak device.

---

## 2. Break it with REA

1. Dump the flash from your board:
   ```bash
   ./scripts/dump-flash.sh            # 4 MB classic -> dump.bin
   # or: ./scripts/dump-flash.sh /dev/ttyUSB0 0x400000 dump.bin
   ```
   Run `esptool flash_id` first (the script does this) to confirm the real size.
2. Open your agent and paste `rea/01-analyze-prompt.md`. Let REA identify the
   image, carve strings, map the HTTP routes, and describe the OTA path.
3. Record everything in `evidence/v1-findings.md` with the exact offsets REA
   reports.
4. **Prove impact.** Paste `rea/02-attack-prompt.md`, then run the demonstration
   against **your** broker with the creds REA extracted:
   ```bash
   python3 scripts/attack-mqtt.py --host 192.168.1.50 --port 1883 \
       --user sensor01 --password sensor-lab-pass \
       --topic lab/sensor01/telemetry
   ```
   You'll see `temp_c=999` spoofed readings land on your dashboard. Baked-in
   credentials are not secret — that's the headline.

---

## 3. Harden the device (v2)

Flash `firmware/v2-hardened/v2-hardened.ino`. The sketch already removes the
code-level bad habits (no compiled-in secrets, no HTTP config page, no diag
route, TLS instead of plaintext, config read from encrypted NVS). But the real
protection is build-time, in step 5.

1. **Switch the broker to TLS.** Run `./broker/make-certs.sh <broker-ip> broker.lab.local`.
   In `broker/mosquitto.conf`, enable the 8883 TLS block and remove the 1883
   listener, then restart Mosquitto.
2. **Pin the CA.** Paste the contents of `broker/certs/ca.crt` into `MQTT_CA_PEM`
   in `v2-hardened.ino`.
3. **Give the broker a name the ESP32 can resolve.** Name verification is most
   reliable against a DNS name, so add `broker.lab.local` as a local DNS entry on
   your router and provision that as `mqtt_host`.
4. **Provision.** Edit and flash `firmware/provision/provision.ino` once (do this
   after step 4's encryption is on, for real hardening), then flash
   `v2-hardened.ino` over it.

---

## 4. Enable the platform protections

These are what actually defeat the flash dump.

### 4a. Flash encryption
Encrypts the flash so a `read_flash` dump is ciphertext. In production
(release) mode the key lives in an eFuse and never leaves the chip.

### 4b. Secure Boot v2
Ensures only firmware signed with your key will boot, which also enforces
signed OTA.

### 4c. Signed OTA + anti-rollback
OTA images are signed and verified before apply; the anti-rollback eFuse blocks
downgrade attacks.

### 4d. NVS encryption (do not skip)
Flash encryption does **not** cover the `nvs` partition, and that partition is
where v2 keeps its secrets. Enable NVS encryption separately:
- **Classic ESP32:** add an `nvs_keys` partition (flagged `encrypted`) to the
  partition table and turn on `CONFIG_NVS_ENCRYPTION`.
- **S3 / C3 / C6:** you can use the HMAC-based scheme instead, which derives the
  NVS key from an eFuse HMAC key.

Run `provision.ino` **after** flash and NVS encryption are both active, so the
secrets are written encrypted from the start.

> **Toolchain note:** the Arduino IDE builds against a precompiled sdkconfig, so
> you can't switch on Secure Boot or NVS encryption from the IDE alone. For the
> harden phase, build with **ESP-IDF using Arduino as a component** (or plain
> ESP-IDF). The sketch code doesn't change.

The exact commands depend on your toolchain (ESP-IDF `idf.py` menuconfig, or the
Arduino + esptool flow). Follow Espressif's official flash-encryption and
Secure Boot v2 documentation for your chip revision — the steps differ between
the classic ESP32 and the S3/C-series.

> ### 5. ⚠️ eFuse warning — read this twice
> - Enabling flash encryption and Secure Boot **burns eFuses**. This is **not
>   reversible**.
> - A wrong setting can leave the board unable to boot or be reflashed —
>   **permanently bricked**.
> - **Development mode** lets you re-flash while you learn; **release mode** locks
>   it down. Start in development mode, on a **spare board**.
> - Never burn release-mode fuses on your only board or on anything you can't
>   afford to lose.

---

## 6. Re-attack and verify

1. Dump the hardened board:
   ```bash
   ./scripts/dump-flash.sh /dev/ttyUSB0 0x400000 dump-v2.bin
   ```
2. Paste `rea/03-verify-hardened-prompt.md` to your agent. REA should now find
   the nvs/app partitions as high-entropy ciphertext and recover **none** of the
   v1 secrets.
3. Try the credential-reuse demo again — against your TLS endpoint it should fail
   to connect on plaintext 1883.
4. Fill in `evidence/v2-findings.md`, especially the before/after table.

---

## 7. Publish

Combine the two evidence files into the before/after table in your README, and
tie each row to `SECURITY-CHECKLIST.md`. That table — "here's what an analyst
could pull from v1, here's the same analysis finding nothing in v2" — is the
whole story.

---

## Troubleshooting

- **REA still finds secrets in v2.** Almost always: you flashed in development
  mode without the encryption eFuse actually set, or you dumped the board before
  the first encrypted boot completed. Re-check that flash encryption is really
  enabled (`esptool get_security_info` / read the eFuse summary) and re-dump.
- **App partition is ciphertext but REA still finds Wi-Fi/MQTT strings in nvs.**
  NVS encryption isn't on (step 4d). Flash encryption alone leaves nvs readable.
  Also check that v2 calls `WiFi.persistent(false)`, or the Wi-Fi driver keeps a
  second plaintext copy of the SSID/PSK under `nvs.net80211`.
- **Board won't boot after enabling Secure Boot.** Signing key mismatch. This is
  why you use a spare board and development mode first.
- **`dump-flash.sh` can't find the port.** Pass it explicitly, e.g.
  `./scripts/dump-flash.sh /dev/ttyUSB0`. On macOS it's usually
  `/dev/cu.usbserial-*` or `/dev/cu.SLAB_USBtoUART`.
- **Wrong dump size.** Run `esptool flash_id` and use the reported size (classic
  4 MB = `0x400000`, many S3 boards 8 MB = `0x800000`).
