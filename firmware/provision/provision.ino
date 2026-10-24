/*
 * provision.ino  —  one-time config provisioning for the hardened device (v2)
 * ----------------------------------------------------------------------------
 * The v2-hardened sketch reads its config from encrypted NVS at runtime instead
 * of having anything compiled in. This small sketch writes those values into NVS
 * once. Flash it, let it run once, confirm "provisioned OK" on serial, then flash
 * v2-hardened over it. The NVS namespace survives the reflash.
 *
 * Note: flash encryption on its own does NOT encrypt the "nvs" partition. Enable
 * NVS encryption as well (docs/lab-guide.md step 4d), and provision AFTER both
 * are on. Only then is a read_flash dump of nvs ciphertext — the whole point.
 *
 * Fill in your own throwaway lab values below (same ones you control on your
 * broker). These live only in this provisioning sketch, which you do NOT commit
 * and do NOT ship; it is a bench tool.
 *
 * TARGET: same ESP32 you use for v2.
 * LIBRARIES: Preferences (ships with the esp32 core).
 * ----------------------------------------------------------------------------
 */

#include <Preferences.h>

Preferences prefs;

// --- edit these for your lab, then flash once ---
static const char* P_WIFI_SSID  = "LAB_TEST_SSID";
static const char* P_WIFI_PASS  = "lab-throwaway-pass";
static const char* P_MQTT_HOST  = "192.168.1.50";
static const char* P_MQTT_USER  = "sensor01";
static const char* P_MQTT_PASS  = "sensor-lab-pass";
static const char* P_MQTT_TOPIC = "lab/sensor01/telemetry";
static const char* P_API_KEY    = "lab_demo_api_key_0000000000";
// Optional: MD5 hex (32 chars) of an OTA password. Leave empty to keep
// ArduinoOTA disabled in v2 (recommended). Generate with:
//   python3 -c "import hashlib;print(hashlib.md5(b'your-ota-pass').hexdigest())"
static const char* P_OTA_HASH = "";

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n[provision] writing config to NVS namespace 'cfg' ...");

  prefs.begin("cfg", /*readOnly=*/false);
  prefs.putString("wifi_ssid",  P_WIFI_SSID);
  prefs.putString("wifi_pass",  P_WIFI_PASS);
  prefs.putString("mqtt_host",  P_MQTT_HOST);
  prefs.putString("mqtt_user",  P_MQTT_USER);
  prefs.putString("mqtt_pass",  P_MQTT_PASS);
  prefs.putString("mqtt_topic", P_MQTT_TOPIC);
  prefs.putString("api_key",    P_API_KEY);
  prefs.putString("ota_hash",   P_OTA_HASH);
  prefs.end();

  // Read back a couple of non-sensitive keys to confirm the write stuck.
  prefs.begin("cfg", /*readOnly=*/true);
  String host  = prefs.getString("mqtt_host", "");
  String topic = prefs.getString("mqtt_topic", "");
  prefs.end();

  if (host.length() && topic.length()) {
    Serial.println("[provision] provisioned OK.");
    Serial.println("[provision]   mqtt_host = " + host);
    Serial.println("[provision]   mqtt_topic= " + topic);
    Serial.println("[provision] now flash firmware/v2-hardened over this sketch.");
  } else {
    Serial.println("[provision] FAILED — NVS read-back was empty.");
  }
}

void loop() {
  // nothing; provisioning is a one-shot.
  delay(10000);
}
