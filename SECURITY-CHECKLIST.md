# ESP32 IoT Firmware Security Checklist

Derived from the Hack My Own IoT lab. Each item maps to a finding REA surfaces in a weak build and what removes it.

## Secrets

- [ ] **No credentials compiled into firmware.** Wi-Fi passwords, MQTT creds, API keys, and tokens must not appear as string literals. REA pulls these straight out of `.rodata`.
- [ ] Secrets provisioned at runtime (BLE/SoftAP setup, or factory provisioning), stored in **encrypted NVS**. NVS encryption is a separate setting from flash encryption; turn on both.
- [ ] `WiFi.persistent(false)` (or equivalent), so the Wi-Fi driver doesn't keep its own copy of the credentials in NVS.
- [ ] No secrets in serial logs, even at debug level.
- [ ] Private keys live in NVS or eFuse, never in the app image.

## Flash protection

- [ ] **Flash encryption enabled** (release mode for production). Without it, `read_flash` gives plaintext.
- [ ] **Secure Boot v2 enabled** so only signed firmware runs.
- [ ] eFuse download-mode / UART download disabled in production.
- [ ] JTAG disabled via eFuse in production.

## Network

- [ ] All cloud/broker traffic over **TLS**, with **certificate (or public-key) pinning**.
- [ ] No plaintext HTTP for config or telemetry.
- [ ] MQTT broker requires authentication; device uses least-privilege topics.
- [ ] No hardcoded IPs or hidden/default endpoints.

## Update path

- [ ] **OTA images are signed**; the device verifies the signature before applying (Secure Boot enforces this).
- [ ] OTA served over TLS.
- [ ] Rollback protection enabled (anti-rollback eFuse) so downgrade attacks fail.

## Management surface

- [ ] No default admin login on any config page.
- [ ] Config web UI disabled in normal operation, or auth-gated + HTTPS.
- [ ] Debug UART output disabled or minimized in production builds.
- [ ] No undocumented debug/diagnostic routes.

## Verification (prove it, don't assume it)

- [ ] Re-dump the production image and run REA — confirm no plaintext secrets, endpoints, or routes.
- [ ] Attempt the original attack against v2 — confirm it fails.
- [ ] Document each v1 finding next to its v2 result.

---

### Finding → fix map

| REA finding in v1 | Fix in v2 |
|-------------------|-----------|
| Wi-Fi / MQTT creds in plaintext strings | Encrypted NVS + flash encryption |
| API key literal | Runtime provisioning, encrypted NVS |
| OTA endpoint, unsigned | Signed OTA + Secure Boot v2 |
| HTTP config page, default admin | Remove/auth-gate, HTTPS only |
| Debug UART strings, verbose logs | Disable debug output in release |
| Readable partition table + app logic | Flash encryption |
