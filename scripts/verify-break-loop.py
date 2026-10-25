#!/usr/bin/env python3
"""
verify-break-loop.py — prove the break-phase tooling works with NO Mosquitto and
NO ESP32 board, using a pure-Python MQTT broker (amqtt).

It is an optional self-check for the "break" phase. It stands up a local broker
that REQUIRES auth, then runs the real attack-mqtt.py (next to this file) with the
credentials REA would extract from the v1 firmware, and confirms a subscriber
receives the spoofed telemetry. It also confirms a wrong password is rejected, so
the "credentials matter" lesson is genuinely enforced.

This does NOT replace the real lab (Mosquitto + a flashed board). It verifies the
scripts and the attack path on any machine, offline.

Requires:  pip install amqtt paho-mqtt
Usage:     python scripts/verify-break-loop.py
Scope:     everything targets 127.0.0.1. Local test rig only.
"""
import asyncio
import json
import logging
import sys
from pathlib import Path

try:
    from pwdlib.hashers.argon2 import Argon2Hasher
    from amqtt.broker import Broker
    from amqtt.client import MQTTClient
    from amqtt.mqtt.constants import QOS_0
except ImportError:
    sys.exit("amqtt not installed. Run:  pip install amqtt paho-mqtt")

import tempfile

logging.basicConfig(level=logging.ERROR)  # keep amqtt quiet

HOST, PORT = "127.0.0.1", 1883
USER, PASS = "sensor01", "sensor-lab-pass"       # the creds baked into v1 firmware
TOPIC = "lab/sensor01/telemetry"
ATTACK = Path(__file__).resolve().parent / "attack-mqtt.py"
COUNT = 5


async def main() -> int:
    if not ATTACK.exists():
        sys.exit(f"cannot find {ATTACK}")

    tmp = Path(tempfile.mkdtemp(prefix="amqtt-lab-"))
    pwf = tmp / "passwd"
    pwf.write_text(f"{USER}:{Argon2Hasher().hash(PASS)}\n", encoding="utf-8")

    broker = Broker({
        "listeners": {"default": {"type": "tcp", "bind": f"{HOST}:{PORT}"}},
        "plugins": {
            "amqtt.plugins.authentication.FileAuthPlugin": {"password_file": str(pwf)},
        },
    })
    await broker.start()
    print(f">> broker up on {HOST}:{PORT} (auth required, anonymous disabled)\n")

    dash = MQTTClient(client_id="dashboard")
    await dash.connect(f"mqtt://{USER}:{PASS}@{HOST}:{PORT}")
    await dash.subscribe([(TOPIC, QOS_0)])
    print(f">> dashboard subscribed to {TOPIC}\n")

    # negative check: a wrong password must be refused
    bad = MQTTClient(client_id="wrongcreds", config={"auto_reconnect": False})
    try:
        await asyncio.wait_for(
            bad.connect(f"mqtt://{USER}:WRONG-PASSWORD@{HOST}:{PORT}"), timeout=8)
        rejected = False
        await bad.disconnect()
    except Exception:
        rejected = True
    print(f">> wrong-password client rejected? {rejected}  "
          f"({'PASS' if rejected else 'FAIL — broker accepted bad creds'})\n")

    print(f">> running attack-mqtt.py with extracted creds ...\n{'-' * 60}")
    proc = await asyncio.create_subprocess_exec(
        sys.executable, str(ATTACK),
        "--host", HOST, "--port", str(PORT),
        "--user", USER, "--password", PASS,
        "--topic", TOPIC, "--count", str(COUNT), "--interval", "0.2",
        stdout=asyncio.subprocess.PIPE, stderr=asyncio.subprocess.STDOUT,
    )

    received = []

    async def collect():
        while len(received) < COUNT:
            msg = await dash.deliver_message(timeout_duration=10)
            if msg is None:
                break
            received.append(json.loads(bytes(msg.data).decode()))

    try:
        await asyncio.wait_for(collect(), timeout=25)
    except asyncio.TimeoutError:
        pass

    out, _ = await proc.communicate()
    print(out.decode(errors="replace").rstrip())
    print("-" * 60)

    spoofed = [m for m in received if m.get("temp_c") == 999.0 and m.get("spoofed")]
    print(f"\n>> dashboard received {len(received)} message(s); "
          f"{len(spoofed)} spoofed (temp_c=999, spoofed=true)")

    await dash.disconnect()
    await broker.shutdown()

    ok = rejected and len(spoofed) == COUNT
    print(f"\n>> RESULT: {'PASS — break-phase loop verified' if ok else 'FAIL'}")
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(asyncio.run(main()))
