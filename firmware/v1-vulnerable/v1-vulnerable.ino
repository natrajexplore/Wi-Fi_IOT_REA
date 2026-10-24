/*
 * v1-vulnerable.ino  —  Hack My Own IoT lab, device under test (INSECURE BY DESIGN)
 * ----------------------------------------------------------------------------
 * A typical cheap-IoT "Wi-Fi temperature sensor" that reports to an MQTT broker
 * and exposes an HTTP config page. It deliberately contains every bad habit REA
 * is good at surfacing. You will dump this firmware and let your agent find them.
 *
 * TARGET: ESP32-WROOM-32 (classic), Arduino framework.
 *   - For S3/C3/C6, the code is the same; only the flash dump size differs.
 *
 * LIBRARIES (Library Manager):
 *   - PubSubClient (by Nick O'Leary)
 *   WiFi, WebServer, ArduinoOTA ship with the esp32 core.
 *
 * DO NOT deploy this anywhere real. It is a target, not a product.
 * ----------------------------------------------------------------------------
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include "secrets.h"   // copy secrets.example.h -> secrets.h first

// ---------------------------------------------------------------------------
// BAD HABIT #1: secrets compiled straight into the image as string literals.
// REA will pull every one of these out of .rodata.
// (The values come from secrets.h, but they end up as plaintext in the binary.)
// ---------------------------------------------------------------------------
const char* wifi_ssid   = WIFI_SSID;
const char* wifi_pass   = WIFI_PASSWORD;
const char* mqtt_host   = MQTT_HOST;
const int   mqtt_port   = MQTT_PORT;
const char* mqtt_user   = MQTT_USER;
const char* mqtt_pass   = MQTT_PASS;
const char* mqtt_topic  = MQTT_TOPIC;

// BAD HABIT #2: hardcoded cloud API key, in the clear.
const char* cloud_api_key = CLOUD_API_KEY;

// BAD HABIT #3: hardcoded, hidden endpoints + an unsigned OTA URL.
const char* telemetry_endpoint = "http://api.lab-sensors.local/v1/ingest";
const char* ota_update_url      = "http://192.168.1.50:8000/firmware/latest.bin"; // no signature check
const char* diag_info_path      = "/diag/info";  // undocumented route that leaks config

// BAD HABIT #4: default admin creds for the config page.
const char* admin_user = ADMIN_USER;
const char* admin_pass = ADMIN_PASS;

WiFiClient   net;
PubSubClient mqtt(net);
WebServer    server(80);

// ---------------------------------------------------------------------------
// BAD HABIT #5: verbose debug logging that leaks secrets over UART.
// ---------------------------------------------------------------------------
#define DEBUG 1
void dbg(const String& s) {
#if DEBUG
  Serial.println("[DBG] " + s);
#endif
}

float readTemperatureC() {
  // Simulated sensor reading (no real hardware needed for the lab).
  return 24.0 + (float)(millis() % 600) / 100.0;
}

void connectWifi() {
  dbg(String("Connecting to SSID=") + wifi_ssid + " PSK=" + wifi_pass); // leaks PSK
  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_pass);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  dbg("WiFi up, IP=" + WiFi.localIP().toString());
}

void connectMqtt() {
  mqtt.setServer(mqtt_host, mqtt_port);
  while (!mqtt.connected()) {
    dbg(String("MQTT connect user=") + mqtt_user + " pass=" + mqtt_pass); // leaks creds
    if (mqtt.connect("sensor01", mqtt_user, mqtt_pass)) {
      dbg("MQTT connected");
    } else {
      dbg("MQTT failed rc=" + String(mqtt.state()));
      delay(2000);
    }
  }
}

// ---------------------------------------------------------------------------
// BAD HABIT #6: HTTP config page with default login, served in the clear.
// ---------------------------------------------------------------------------
bool checkAuth() {
  if (!server.authenticate(admin_user, admin_pass)) {
    server.requestAuthentication();
    return false;
  }
  return true;
}

void handleRoot() {
  if (!checkAuth()) return;
  String html =
    "<html><body><h2>Sensor01 Config</h2>"
    "<p>Broker: " + String(mqtt_host) + ":" + String(mqtt_port) + "</p>"
    "<p>Topic: " + String(mqtt_topic) + "</p>"
    "<p>API key: " + String(cloud_api_key) + "</p>"  // prints the key to the page
    "<form method='POST' action='/ota'><button>Update firmware</button></form>"
    "</body></html>";
  server.send(200, "text/html", html);
}

// BAD HABIT #7: OTA trigger with no auth and no signature verification.
void handleOta() {
  server.send(200, "text/plain", "Pretending to fetch unsigned firmware from " + String(ota_update_url));
  // A real version of this would download and flash ota_update_url with no checks.
}

// BAD HABIT #8: undocumented, unauthenticated diagnostic route that discloses
// the full running config (an information-disclosure anti-pattern).
void handleDiagInfo() {
  String info =
    "host=" + String(mqtt_host) + "\n" +
    "topic=" + String(mqtt_topic) + "\n" +
    "api_key=" + String(cloud_api_key) + "\n" +
    "ota_url=" + String(ota_update_url) + "\n";
  server.send(200, "text/plain", info);
}

void setupWeb() {
  server.on("/", handleRoot);
  server.on("/ota", HTTP_POST, handleOta);
  server.on(diag_info_path, handleDiagInfo);
  server.begin();
}

void setup() {
  Serial.begin(115200);
  delay(300);
  dbg("Booting v1-vulnerable. API key=" + String(cloud_api_key)); // leaks key at boot
  connectWifi();
  connectMqtt();
  setupWeb();

  // BAD HABIT #9: ArduinoOTA with no password set — anyone on the LAN can push firmware.
  ArduinoOTA.setHostname("sensor01");
  ArduinoOTA.begin();
}

void loop() {
  if (!mqtt.connected()) connectMqtt();
  mqtt.loop();
  ArduinoOTA.handle();
  server.handleClient();

  static unsigned long last = 0;
  if (millis() - last > 5000) {
    last = millis();
    float t = readTemperatureC();
    String payload = "{\"id\":\"sensor01\",\"temp_c\":" + String(t, 2) +
                     ",\"key\":\"" + String(cloud_api_key) + "\"}"; // key in every message
    mqtt.publish(mqtt_topic, payload.c_str());
    dbg("Published: " + payload);
  }
}
