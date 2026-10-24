/*
 * v2-hardened.ino  —  Hack My Own IoT lab, device under test (HARDENED)
 * ----------------------------------------------------------------------------
 * Same sensor, every v1 bad habit removed. This is the "after" image. When you
 * re-dump it with flash encryption enabled and run REA again, the secrets that
 * were trivially readable in v1 should be gone.
 *
 * What changed vs v1 (each maps to a line in SECURITY-CHECKLIST.md):
 *   #1 secrets      -> read from encrypted NVS at runtime, never compiled in
 *   #2 api key      -> provisioned once, stored in encrypted NVS
 *   #3 endpoints    -> single TLS endpoint, no hidden/plaintext URLs
 *   #4 admin creds  -> no default login; config UI removed from normal operation
 *   #5 debug logs   -> no secrets in logs; debug gated off in release
 *   #6 http config  -> removed (provision over BLE/SoftAP in a real build)
 *   #7 unsigned OTA -> signed OTA enforced by Secure Boot v2 (see lab-guide.md)
 *   #8 diag route   -> removed
 *   #9 open OTA     -> ArduinoOTA password-protected (or disabled in release)
 *
 * The real protection is NOT in this sketch — it is in the build-time settings:
 *   - Flash encryption (release mode)
 *   - Secure Boot v2
 *   - Signed OTA + anti-rollback eFuse
 *   - TLS with certificate/public-key pinning
 * See docs/lab-guide.md. Those steps BURN eFuses and can brick a board. Use a
 * spare board and development mode first.
 *
 * TARGET: ESP32-WROOM-32 (classic), Arduino framework.
 * LIBRARIES: PubSubClient, Preferences + WiFiClientSecure (ship with esp32 core).
 * ----------------------------------------------------------------------------
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Preferences.h>
#include <ArduinoOTA.h>

// Release builds turn debug off. No secret is ever printed, at any level.
#ifndef LAB_DEBUG
#define LAB_DEBUG 0
#endif
void dbg(const String& s) {
#if LAB_DEBUG
  Serial.println("[DBG] " + s);
#endif
}

// ---------------------------------------------------------------------------
// FIX #1/#2: nothing sensitive is compiled in. Config is read at runtime from
// NVS. IMPORTANT: flash encryption alone does NOT encrypt the nvs partition.
// You must ALSO enable NVS encryption (an encrypted "nvs_keys" partition, or the
// HMAC-based scheme on S3/C3/C6). See docs/lab-guide.md step 4d.
// Provision these values ONCE via a setup flow (BLE/SoftAP) or a factory step;
// the provisioning sketch is out of scope for the lab, but the shape is:
//     prefs.begin("cfg", false);
//     prefs.putString("wifi_ssid", ...); prefs.putString("wifi_pass", ...);
//     prefs.putString("mqtt_host", ...); prefs.putString("mqtt_user", ...);
//     prefs.putString("mqtt_pass", ...); prefs.putString("api_key", ...);
//     prefs.end();
// With flash encryption + NVS encryption enabled, a read_flash dump of the nvs
// partition is ciphertext, so REA cannot recover these strings.
// ---------------------------------------------------------------------------
Preferences prefs;

String wifi_ssid, wifi_pass, mqtt_host, mqtt_user, mqtt_pass, mqtt_topic, api_key;
int    mqtt_port = 8883;  // TLS

// FIX #3: one TLS endpoint, pinned. Paste YOUR broker's CA (or leaf) PEM here.
// A public key / CA in the image is not a secret — it is the trust anchor.
static const char* MQTT_CA_PEM = R"EOF(
-----BEGIN CERTIFICATE-----
...replace with your own broker's CA certificate...
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure net;
PubSubClient     mqtt(net);
bool             otaEnabled = false;

bool loadConfig() {
  prefs.begin("cfg", /*readOnly=*/true);
  wifi_ssid  = prefs.getString("wifi_ssid", "");
  wifi_pass  = prefs.getString("wifi_pass", "");
  mqtt_host  = prefs.getString("mqtt_host", "");
  mqtt_user  = prefs.getString("mqtt_user", "");
  mqtt_pass  = prefs.getString("mqtt_pass", "");
  mqtt_topic = prefs.getString("mqtt_topic", "lab/sensor01/telemetry");
  api_key    = prefs.getString("api_key", "");
  prefs.end();

  bool ok = wifi_ssid.length() && mqtt_host.length() && mqtt_user.length();
  if (!ok) {
    // Not provisioned. A real device would enter a provisioning mode here.
    Serial.println("[FATAL] device not provisioned; halting.");
  }
  return ok;
}

float readTemperatureC() {
  return 24.0 + (float)(millis() % 600) / 100.0;
}

void connectWifi() {
  dbg("Connecting Wi-Fi");               // FIX #5: no SSID/PSK in the log
  // The Wi-Fi driver otherwise writes its OWN copy of SSID/PSK into NVS
  // (namespace nvs.net80211). Keep the only copy in our "cfg" namespace.
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  while (WiFi.status() != WL_CONNECTED) { delay(500); }
  dbg("Wi-Fi up");
}

void connectMqtt() {
  // FIX #3: TLS with a pinned trust anchor. No plaintext 1883.
  net.setCACert(MQTT_CA_PEM);
  mqtt.setServer(mqtt_host.c_str(), mqtt_port);
  while (!mqtt.connected()) {
    dbg("MQTT connecting (TLS)");        // FIX #5: no creds in the log
    if (mqtt.connect("sensor01", mqtt_user.c_str(), mqtt_pass.c_str())) {
      dbg("MQTT connected");
    } else {
      dbg("MQTT retry");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  dbg("Booting v2-hardened");            // FIX #5: nothing sensitive at boot

  if (!loadConfig()) { while (true) delay(1000); }

  connectWifi();
  connectMqtt();

  // FIX #9: OTA is never open. In a true release you rely on signed OTA +
  // Secure Boot and leave ArduinoOTA off. It only starts if an OTA password
  // hash (MD5 of the password) was provisioned; otherwise it stays disabled.
  prefs.begin("cfg", /*readOnly=*/true);
  String otaHash = prefs.getString("ota_hash", "");
  prefs.end();
  if (otaHash.length() == 32) {
    otaEnabled = true;
    ArduinoOTA.setHostname("sensor01");
    ArduinoOTA.setPasswordHash(otaHash.c_str());
    ArduinoOTA.begin();
    dbg("OTA enabled (password-protected)");
  } else {
    dbg("OTA disabled (no ota_hash provisioned)");
  }

  // FIX #4/#6/#8: no HTTP config page, no default admin, no diag route. Gone.
}

void loop() {
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();
  if (otaEnabled) ArduinoOTA.handle();

  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    float t = readTemperatureC();
    // FIX #2: the API key is NOT shipped in the payload. Authentication is the
    // TLS session + MQTT credentials, not a shared secret echoed in every message.
    String payload = "{\"id\":\"sensor01\",\"temp_c\":" + String(t, 2) + "}";
    mqtt.publish(mqtt_topic.c_str(), payload.c_str());
    dbg("Published telemetry");
  }
}
