#!/usr/bin/env bash
#
# make-certs.sh — create a private lab CA and a broker certificate for the v2
# (TLS) phase. Self-signed is fine: the device pins this CA, so no public CA is
# involved.
#
# Usage:
#   ./broker/make-certs.sh [BROKER_IP] [BROKER_DNS]
#   ./broker/make-certs.sh 192.168.1.50 broker.lab.local
#
# Output (all gitignored):
#   broker/certs/ca.crt       -> paste into MQTT_CA_PEM in v2-hardened.ino
#   broker/certs/ca.key       -> keep private; only used to sign the server cert
#   broker/certs/server.crt   -> mosquitto certfile
#   broker/certs/server.key   -> mosquitto keyfile
#
set -euo pipefail

IP="${1:-192.168.1.50}"
DNS="${2:-broker.lab.local}"
DIR="$(cd "$(dirname "$0")" && pwd)/certs"
DAYS=825

command -v openssl >/dev/null || { echo "error: openssl not found" >&2; exit 1; }
mkdir -p "$DIR"
cd "$DIR"

echo ">> lab CA"
openssl req -x509 -newkey rsa:2048 -nodes -sha256 -days "$DAYS" \
  -keyout ca.key -out ca.crt -subj "/CN=Hack My Own IoT Lab CA"

echo ">> broker key + CSR"
openssl req -newkey rsa:2048 -nodes -sha256 \
  -keyout server.key -out server.csr -subj "/CN=$DNS"

# SAN carries both the DNS name and the IP. Name verification on the ESP32 is
# most reliable against a DNS name, so prefer setting mqtt_host to $DNS.
cat > server.ext <<EOF
basicConstraints=CA:FALSE
keyUsage=digitalSignature,keyEncipherment
extendedKeyUsage=serverAuth
subjectAltName=DNS:$DNS,IP:$IP
EOF

echo ">> sign broker cert with lab CA"
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial \
  -out server.crt -days "$DAYS" -sha256 -extfile server.ext

rm -f server.csr server.ext ca.srl
chmod 600 ca.key server.key

echo
echo ">> verify chain:"
openssl verify -CAfile ca.crt server.crt
echo
echo ">> done. Next:"
echo "   1. Paste the contents of broker/certs/ca.crt into MQTT_CA_PEM (v2-hardened.ino)."
echo "   2. Enable the 8883 TLS block in broker/mosquitto.conf and remove the 1883 listener."
echo "   3. Provision mqtt_host as '$DNS' (needs a local DNS entry), or '$IP' if your core verifies IP SANs."
echo "   4. Test from a PC:  mosquitto_sub -h $DNS -p 8883 --cafile broker/certs/ca.crt -u sensor01 -P <pass> -t 'lab/#' -v"
