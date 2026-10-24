# REA prompt 02 — turn findings into proven impact

After prompt 01 you have a list of findings in `evidence/v1-findings.md`. This
step is about demonstrating *impact* on infrastructure you control, so the
before/after story is concrete rather than theoretical.

> Scope: the only broker, dashboard, and device involved are ones I own and run
> locally for this lab. Nothing here touches a third party.

---

## Prompt

```
Using the findings from my v1 firmware analysis, help me reason about blast
radius on my own lab setup (local Mosquitto broker + my test dashboard):

1. Given the extracted MQTT host/user/pass/topic, what could someone on my LAN
   do? Enumerate concretely: publish spoofed telemetry, subscribe to my topics,
   knock the real sensor's data out with conflicting values.

2. Given the plaintext OTA URL with no signature check, describe the update-path
   risk in one paragraph — what an attacker would need, and what signed OTA +
   Secure Boot would change. (Do NOT write exploit code for pushing firmware.)

3. Given the /diag/info route and the default-admin config page, what config is
   disclosed and what can be changed without real authentication?

4. Rank the findings by real-world severity for a shipped product, and tie each
   to the specific hardening control that removes it.
```

---

## Proving it (your broker only)

`scripts/attack-mqtt.py` demonstrates the single most tangible finding: the
extracted MQTT credentials let anyone publish to your topic. It connects to
**your** broker with the creds REA pulled out of your own image and publishes a
few obviously-fake readings (e.g. `temp_c: 999`). On your dashboard you will see
the spoofed values arrive next to the genuine sensor feed.

```bash
python3 scripts/attack-mqtt.py \
    --host 192.168.1.50 --port 1883 \
    --user sensor01 --password sensor-lab-pass \
    --topic lab/sensor01/telemetry
```

That is the whole point of the "break" phase: credentials baked into a binary
are not secret, and anyone who can read the flash can impersonate the device.

Record the outcome (screenshot of the dashboard, broker log line) in
`evidence/v1-findings.md` under "Proven impact".

> This script only publishes benign test data to a broker you operate. Do not
> point it at any broker you do not own.
