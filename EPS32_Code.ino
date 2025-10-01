// ESP32-WROOM-32D Smart Irrigation + MQTT (HiveMQ Cloud TLS) + Always AP Mode
// Modified: Device pairing/unpairing handled by Next.js backend
// device_id regenerates on reset, used for everything

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <time.h>
#include <Preferences.h>
#include <WebServer.h>

// ---------------- Debug helpers ----------------
#define LOGI(tag, fmt, ...) Serial.printf("[INFO]  %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOGW(tag, fmt, ...) Serial.printf("[WARN]  %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOGE(tag, fmt, ...) Serial.printf("[ERROR] %s: " fmt "\n", tag, ##__VA_ARGS__)
#define LOGD(tag, fmt, ...) Serial.printf("[DBG]   %s: " fmt "\n", tag, ##__VA_ARGS__)

// ---------------- HiveMQ Cloud ----------------
const char* MQTT_HOST = "dc55e7a89e1747adb06e52b26e9343da.s1.eu.hivemq.cloud";
uint16_t MQTT_PORT = 8883;  // TLS
const char* MQTT_USER = "hivemq.webclient.1756187891168";
const char* MQTT_PASS = "G52Q?eCnL0vZj,s1g>O!";

// Root CA (your working CA)
static const char HMQ_ROOT_CA[] PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// ---------------- Identity, topics ----------------
String deviceId;                 // Unique device ID - regenerates on reset, used for everything
String topicTele, topicStatus, topicCmd;
const char* FW_VERSION = "2.1.0";

// --- Retained cleanup helpers ---
String cleanupOldId;               // previous deviceId to clean
String cleanupTopicPending = "";   // topic to retry purge
unsigned long cleanupRetryAt = 0;
uint8_t cleanupRetriesLeft = 0;

// ---------------- Pins (UNCHANGED) ----------------
#define PIN_SOIL   34
#define PIN_DHT    32
#define PIN_LIGHT  35
#define PIN_BUTTON 33
#define PIN_MOTOR  2

// ---------------- Sensors/Config (UNCHANGED) ----------------
#define DHT_TYPE DHT11
DHT dht(PIN_DHT, DHT_TYPE);
int  DRY_MV = 2800;
int  WET_MV = 1200;
const bool LIGHT_INVERT = false;
const bool BUTTON_ACTIVE_HIGH = false;
const unsigned long DEBOUNCE_MS = 50;
const unsigned long PUB_MS = 10000;  // 10s

volatile int   soil_mv = 0, soil_pct = 0;
volatile int   light_mv = 0, light_pct = 0;
volatile float air_temp = NAN, air_hum = NAN;

// ---------------- Motor safety & state ----------------
int lastRead = HIGH, lastStable = HIGH; unsigned long tDebounce = 0;
bool pumpRunning = false; unsigned long pumpStopMs = 0;
unsigned long PUMP_MS_DEFAULT = 5000;
unsigned long PUMP_HARD_CAP_MS = 60000;  // absolute maximum runtime (60 seconds)
unsigned long BOOT_LOCK_MS     = 3000;   // clamp motor OFF on boot
unsigned long bootAt = 0;

// Button press timing
bool lastButtonState = HIGH;

// Force motor LOW as early as possible (before setup())
extern "C" void __attribute__((constructor)) preSetupForceMotorOff() {
  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);
}

// ---------------- Globals that must be visible to web handlers ----------------
Preferences prefs;
WebServer apServer(80);

// ---------------- Net/MQTT (moved up so /reset can use it) ----------------
WiFiClientSecure net;
PubSubClient mqtt(net);

// ---------------- Wi-Fi provisioning ----------------
String prov_ssid = "Irrig-" + String((uint32_t)(ESP.getEfuseMac() & 0xFFFFFF), HEX);
const char* prov_pass = "water123";

// Generate a unique device ID (for pairing and identification)
String generateDeviceId() {
  prefs.begin("device", true);
  String id = prefs.getString("id", "");
  prefs.end();

  if (id.length() == 0) {
    String chars = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";  // Exclude confusing chars
    id = "";
    for (int i = 0; i < 8; i++) {
      if (i == 4) id += "-";
      id += chars[random(0, chars.length())];
    }
    prefs.begin("device", false);
    prefs.putString("id", id);
    prefs.end();
    LOGI("DEVICE", "Generated new device ID: %s", id.c_str());
  } else {
    LOGI("DEVICE", "Loaded existing device ID: %s", id.c_str());
  }
  return id;
}

bool loadWifiCreds(String &ssid, String &pass){
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  LOGI("WIFI", "Loaded creds: %s / %s", ssid.length() ? ssid.c_str() : "<empty>", pass.length() ? "********" : "<empty>");
  return ssid.length() > 0;
}
void saveWifiCreds(const String &ssid, const String &pass){
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  LOGI("WIFI", "Saved creds: %s / %s", ssid.c_str(), pass.length() ? "********" : "<empty>");
}

// ---- Helper: delete a retained message (0-byte retained) ----
bool deleteRetained(const String& topic) {
  if (!mqtt.connected()) return false;
  bool a = mqtt.publish(topic.c_str(), (const uint8_t*)nullptr, 0, true);
  mqtt.beginPublish(topic.c_str(), 0, true);
  bool b = mqtt.endPublish();
  LOGI("CLEAN", "deleteRetained(%s) -> A:%s B:%s", topic.c_str(), a ? "ok" : "fail", b ? "ok" : "fail");
  return a || b;
}

void startProvisioningAP(){
  WiFi.mode(WIFI_AP);
  WiFi.softAP(prov_ssid.c_str(), prov_pass);
  LOGI("PROV", "AP SSID=%s PASS=%s IP=%s",
       prov_ssid.c_str(), prov_pass, WiFi.softAPIP().toString().c_str());

  apServer.on("/", [](){
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Plant Care Device</title><style>";
    html += "body{font-family:Arial;margin:20px;background:#f5f5f5}.container{max-width:600px;margin:0 auto;background:#fff;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,.1)}";
    html += ".header{text-align:center;color:#2c3e50;margin-bottom:30px}.device-code{background:#3498db;color:#fff;padding:20px;border-radius:8px;text-align:center;font-size:32px;font-weight:bold;letter-spacing:3px;margin:20px 0}";
    html += ".status{padding:15px;border-radius:8px;margin:10px 0}.online{background:#d4edda;color:#155724;border:1px solid #c3e6cb}.offline{background:#f8d7da;color:#721c24;border:1px solid #f5c6cb}";
    html += ".admin-panel{background:#fff3cd;color:#856404;border:1px solid #ffeaa7;padding:15px;border-radius:8px;margin:15px 0}.sensor-grid{display:grid;grid-template-columns:1fr 1fr;gap:15px;margin:20px 0}";
    html += ".sensor-card{background:#e8f4fd;padding:15px;border-radius:8px;text-align:center}.sensor-value{font-size:24px;font-weight:bold;color:#2c3e50}.sensor-label{color:#7f8c8d;font-size:14px}";
    html += ".button{background:#3498db;color:#fff;border:none;padding:10px 20px;border-radius:5px;cursor:pointer;margin:5px}.button:hover{background:#2980b9}.button.danger{background:#e74c3c}.button.danger:hover{background:#c0392b}";
    html += ".form-group{margin:15px 0}.form-group label{display:block;margin-bottom:5px;font-weight:bold}.form-group input{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px}.info-box{background:#e8f4fd;padding:15px;border-radius:8px;margin:15px 0}";
    html += "</style><script>function setPumpDuration(){const d=document.getElementById('pump-duration').value;fetch('/setpump?duration='+d).then(r=>r.text()).then(alert)}function refreshPage(){location.reload()}</script></head><body>";
    html += "<div class='container'><div class='header'><h1>ESP32 Device</h1></div>";
    html += "<div class='device-code'>" + deviceId + "</div>";
    html += "<div class='info-box'><strong>Pairing Instructions:</strong><br>Use this code in the app to pair this device with your account.</div>";

    bool wifiConnected = (WiFi.status() == WL_CONNECTED);
    if (wifiConnected) {
      html += "<div class='status online'><strong>🟢 System ONLINE</strong><br>Connected to WiFi</div>";
      html += "<div class='admin-panel'><h3>🔧 Control Panel</h3>";
      html += "<button class='button' onclick='refreshPage()' style='margin-bottom:15px'>🔄 Refresh Page</button>";
      html += "<div class='sensor-grid'>";
      html += "<div class='sensor-card'><div class='sensor-value'>" + String(soil_pct) + "%</div><div class='sensor-label'>Soil Moisture</div></div>";
      html += "<div class='sensor-card'><div class='sensor-value'>" + String(light_pct) + "%</div><div class='sensor-label'>Light Level</div></div>";
      html += "<div class='sensor-card'><div class='sensor-value'>" + String(air_temp) + "°C</div><div class='sensor-label'>Temperature</div></div>";
      html += "<div class='sensor-card'><div class='sensor-value'>" + String(air_hum) + "%</div><div class='sensor-label'>Humidity</div></div>";
      html += "</div>";
      html += "<div class='form-group'><label>Pump Duration (ms):</label><input type='number' id='pump-duration' value='" + String(PUMP_MS_DEFAULT) + "' min='1000' max='30000'><button class='button' onclick='setPumpDuration()'>Update Duration</button></div>";
      html += "<p><strong>Pump Status: </strong><span style='color:" + String(pumpRunning ? "#e74c3c" : "#27ae60") + "'>" + String(pumpRunning ? "RUNNING" : "STOPPED") + "</span></p>";
      html += "<form method='POST' action='/reset' style='margin-top:20px'><button type='submit' class='button danger'>🔴 RESET DEVICE</button></form><p><small>This will clear WiFi credentials and unpair the device</small></p></div>";
    } else {
      html += "<div class='status offline'><strong>🔴 System OFFLINE</strong><br>WiFi Setup Required</div>";
      html += "<div class='form-group'><h3>WiFi Configuration</h3><form method='POST' action='/save'><label>Network Name (SSID):</label><input name='s' placeholder='Enter WiFi network name'><label>Password:</label><input name='p' type='password' placeholder='Enter WiFi password'><button type='submit' class='button'>Save & Connect</button></form></div>";
    }

    html += "</div></body></html>";
    apServer.send(200, "text/html", html);
  });

  apServer.on("/save", [](){
    String s = apServer.arg("s"), p = apServer.arg("p");
    if (s.length()){
      saveWifiCreds(s, p);
      apServer.send(200, "text/plain", "Saved. Rebooting...");
      delay(300);
      digitalWrite(PIN_MOTOR, LOW); // safety before restart
      ESP.restart();
    } else {
      apServer.send(400, "text/plain", "SSID required");
    }
  });

  apServer.on("/reset", [](){
    LOGI("RESET", "Device reset requested - clearing WiFi credentials and device ID");

    // If connected, delete CURRENT retained /status and disconnect cleanly to avoid LWT "offline"
    if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
      LOGI("RESET", "Clearing CURRENT retained status before reboot: %s", topicStatus.c_str());
      mqtt.beginPublish(topicStatus.c_str(), 0, true); // retained empty => delete
      mqtt.endPublish();
      mqtt.disconnect(); // graceful -> no LWT
      delay(200);
    }

    // Remember old ID for cleanup on next boot
    prefs.begin("cleanup", false);
    prefs.putString("oldid", deviceId);
    prefs.end();

    // Clear WiFi credentials
    LOGI("RESET", "Clearing WiFi credentials");
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.end();

    // Clear device ID (will regenerate on next boot)
    LOGI("RESET", "Clearing device ID - new ID will be generated");
    prefs.begin("device", false);
    prefs.remove("id");
    prefs.clear();
    prefs.end();

    apServer.send(200, "text/plain", "Device reset. WiFi and device ID cleared. Rebooting...");
    delay(800);
    digitalWrite(PIN_MOTOR, LOW);  // safety before restart
    ESP.restart();
  });

  apServer.on("/setpump", [](){
    if (apServer.hasArg("duration")) {
      int newDuration = apServer.arg("duration").toInt();
      LOGI("CONFIG", "Received duration request: %d ms", newDuration);
      if (newDuration > 0 && newDuration <= 30000) {
        PUMP_MS_DEFAULT = newDuration;
        LOGI("CONFIG", "Pump duration set to %lums", PUMP_MS_DEFAULT);
        apServer.send(200, "text/plain", "Pump duration updated to " + String(PUMP_MS_DEFAULT) + "ms");
      } else {
        LOGI("CONFIG", "Invalid duration: %d (must be 1-30000)", newDuration);
        apServer.send(400, "text/plain", "Invalid duration. Must be 1-30000ms");
      }
    } else {
      LOGI("CONFIG", "Missing duration parameter");
      apServer.send(400, "text/plain", "Missing duration parameter");
    }
  });

  apServer.on("/status", [](){
    String json = "{";
    json += "\"device_id\":\"" + deviceId + "\",";
    json += "\"soil_mv\":" + String(soil_mv) + ",";
    json += "\"soil_pct\":" + String(soil_pct) + ",";
    json += "\"light_mv\":" + String(light_mv) + ",";
    json += "\"light_pct\":" + String(light_pct) + ",";
    json += "\"temp\":" + String(air_temp) + ",";
    json += "\"humidity\":" + String(air_hum) + ",";
    json += "\"pump_duration\":" + String(PUMP_MS_DEFAULT) + ",";
    json += "\"pump_running\":" + String(pumpRunning ? "true" : "false") + ",";
    json += "\"wifi_connected\":" + String(WiFi.status() == WL_CONNECTED ? "true" : "false");
    json += "}";
    apServer.send(200, "application/json", json);
  });

  apServer.begin();
  LOGI("PROV", "HTTP portal started");
}

// ---------------- Sensor helpers ----------------
int readMedianMilliVolts(int pin, int samples = 15) {
  static int buf[32]; samples = min(samples, 32);
  for (int i=0;i<samples;i++){ buf[i]=analogReadMilliVolts(pin); delay(3);} 
  for (int i=1;i<samples;i++){ int k=buf[i], j=i-1; while(j>=0 && buf[j]>k){buf[j+1]=buf[j]; j--;} buf[j+1]=k; }
  return buf[samples/2];
}
int soilMvToPercent(int mv){
  long den = (long)DRY_MV - WET_MV; if (den<=0) return 0;
  int pct = (int)(100L * ((long)DRY_MV - mv) / den);
  return constrain(pct,0,100);
}
int lightMvToPercent(int mv){
  mv = constrain(mv, 0, 3300);
  // gamma-ish mapping for nicer midrange feel (no floats needed)
  int pct = map(mv, 0, 3300, 0, 100);
  // light “feels” more natural with a gentle curve:
  pct = (pct * pct + 100) / 100; // approx gamma ~2
  return constrain(pct, 0, 100);
}
String buildTelemetryJSON(){
  String j = "{";
  j += "\"device_id\":\"" + deviceId + "\",";
  j += "\"soil_mv\":" + String(soil_mv) + ",";
  j += "\"soil_pct\":" + String(soil_pct) + ",";
  j += "\"light_mv\":" + String(light_mv) + ",";
  j += "\"light_pct\":" + String(light_pct) + ",";
  j += "\"temp_c\":" + (isnan(air_temp)? String("null") : String(air_temp,1)) + ",";
  j += "\"humidity_pct\":" + (isnan(air_hum)? String("null") : String(air_hum,1)) + ",";
  j += "\"pump_running\":" + String(pumpRunning ? "true" : "false") + ",";
  j += "\"fw_version\":\"" + String(FW_VERSION) + "\"";
  j += "}";
  return j;
}

void syncTime() {
  configTime(0, 0, "pool.ntp.org", "time.nist.gov"); // UTC
  Serial.print("[TIME] Syncing time");
  time_t now = 0; int tries = 0;
  while (now < 8 * 3600 && tries < 40) { delay(250); Serial.print("."); now = time(nullptr); tries++; }
  Serial.println();
  if (now >= 8*3600) {
    struct tm tm; gmtime_r(&now, &tm);
    LOGI("TIME", "Set UTC %04d-%02d-%02d %02d:%02d:%02d", tm.tm_year+1900, tm.tm_mon+1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
  } else {
    LOGW("TIME", "NTP FAILED (TLS may fail)");
  }
}

// ---------------- Pump control (SAFE) ----------------
void startPump(unsigned long ms){
  if (pumpRunning) { LOGW("PUMP", "start ignored: already running"); return; }
  LOGI("PUMP", "startPump called with %lums, hard cap is %lums", ms, PUMP_HARD_CAP_MS);
  if (ms > PUMP_HARD_CAP_MS) { LOGW("PUMP", "requested %lums > cap %lums, clamping", ms, PUMP_HARD_CAP_MS); ms = PUMP_HARD_CAP_MS; }

  pumpRunning = true;
  pumpStopMs = millis() + ms;
  digitalWrite(PIN_MOTOR, HIGH);
  LOGI("PUMP", "ON for %lums (stop@%lu)", ms, pumpStopMs);
}

void stopPumpIfDue(){
  if (pumpRunning && (long)(millis() - pumpStopMs) >= 0){
    pumpRunning = false;
    digitalWrite(PIN_MOTOR, LOW);
    LOGI("PUMP", "OFF (timer)");
  }
}

// ---------------- MQTT ----------------
void publishTelemetry(){
  String payload = buildTelemetryJSON();
  bool ok = mqtt.publish(topicTele.c_str(), payload.c_str(), false);
  LOGD("MQTT", "pub tele=%s -> %s", payload.c_str(), ok ? "ok" : "fail");
}

bool ensureMqtt(){
  if (mqtt.connected()) return true;

  String cid = deviceId + "-" + String((uint32_t)ESP.getCycleCount(), HEX);
  String willTopic = topicStatus;

  LOGI("MQTT", "Connecting to HiveMQ as %s", cid.c_str());
  // Retained LWT "offline" (presence best-practice)
  bool ok = mqtt.connect(cid.c_str(), MQTT_USER, MQTT_PASS, willTopic.c_str(), 0, true, "offline");
  if (ok){
    LOGI("MQTT", "Connected");

    // Retained "online" so new subscribers instantly know state
    mqtt.publish(topicStatus.c_str(), "online", true);

    // Clear retained 'status' of the previous device ID (primary attempt)
    if (cleanupOldId.length() && cleanupOldId != deviceId) {
      String oldTopic = String("irrig/") + cleanupOldId + "/status";
      deleteRetained(oldTopic);

      // One delayed retry (~65s) in case old LWT fires late and re-adds "offline"
      cleanupTopicPending = oldTopic;
      cleanupRetryAt = millis() + 65000; // ~2x keepalive (keepAlive=30s)
      cleanupRetriesLeft = 1;

      prefs.begin("cleanup", false);
      prefs.remove("oldid");
      prefs.end();
      cleanupOldId = "";
    }

    // Subscribe to command topic only
    mqtt.subscribe(topicCmd.c_str());

    return true;
  } else {
    LOGE("MQTT", "connect failed, rc=%d", mqtt.state());
    return false;
  }
}

void onMqttMessage(char* topic, byte* payload, unsigned int length){
  String s; s.reserve(length+1); for(unsigned int i=0;i<length;i++) s += (char)payload[i];
  s.trim(); if (s.length()==0) return;

  String t(topic);
  LOGI("MQTT", "rx topic=%s payload=%s", t.c_str(), s.c_str());

  // Watering command: {"water_ms":5000} or "water:5000"
  long ms = 0;
  int idx = s.indexOf("water_ms");
  if (idx>=0){
    int colon = s.indexOf(':', idx);
    if (colon>=0){ String num = s.substring(colon+1); ms = num.toInt(); }
  } else if (s.startsWith("water")){
    int colon = s.indexOf(':'); if(colon>=0){ ms = s.substring(colon+1).toInt(); }
  }
  if (ms <= 0) ms = PUMP_MS_DEFAULT;
  startPump(ms);
}

// ---------------- Setup ----------------
void setup(){
  Serial.begin(115200); delay(200);
  bootAt = millis();

  // IO first: motor LOW
  pinMode(PIN_MOTOR, OUTPUT); digitalWrite(PIN_MOTOR, LOW);
  pinMode(PIN_BUTTON, INPUT);

  // Sensors
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_SOIL,  ADC_11db);
  analogSetPinAttenuation(PIN_LIGHT, ADC_11db);
  dht.begin();

  // Generate or load device ID
  uint32_t seed = esp_random() ^ millis() ^ (uint32_t)ESP.getEfuseMac();
  randomSeed(seed);
  deviceId = generateDeviceId();

  // Load old id marker for retained cleanup (if reset happened)
  prefs.begin("cleanup", true);
  { String old = prefs.getString("oldid", ""); if (old.length() && old != deviceId) { cleanupOldId = old; LOGI("CLEAN", "Will clear retained status for old id: %s", old.c_str()); } }
  prefs.end();

  // MQTT topics
  String base = String("irrig/") + deviceId;
  topicTele   = base + "/telemetry";
  topicStatus = base + "/status";
  topicCmd    = base + "/cmd";

  LOGI("BOOT", "deviceId=%s fw=%s", deviceId.c_str(), FW_VERSION);
  LOGI("TOPIC", "tele=%s", topicTele.c_str());
  LOGI("TOPIC", "stat=%s", topicStatus.c_str());
  LOGI("TOPIC", "cmd =%s", topicCmd.c_str());

  // ALWAYS start AP first for easy access
  startProvisioningAP();

  // Try WiFi connection in background if credentials exist
  String ssid, pass;
  bool haveCreds = loadWifiCreds(ssid, pass);
  if (haveCreds) {
    LOGI("WIFI", "Attempting background connection to %s", ssid.c_str());
    WiFi.mode(WIFI_AP_STA); // Enable both AP and STA
    WiFi.setSleep(false);
    WiFi.begin(ssid.c_str(), pass.c_str());
  }

  // TLS & MQTT client
  syncTime();
  net.setCACert(HMQ_ROOT_CA);
  net.setHandshakeTimeout(15);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(1024);
  mqtt.setKeepAlive(30);
}

// ---------------- Loop ----------------
void loop(){
  // 0) Boot lockout: clamp motor LOW for a few seconds
  if (millis() - bootAt < BOOT_LOCK_MS) {
    digitalWrite(PIN_MOTOR, LOW);
  }

  // 1) Button handling: Pin 33 = water
  int raw = digitalRead(PIN_BUTTON);
  bool buttonActive = (raw == (BUTTON_ACTIVE_HIGH ? HIGH : LOW));
  bool currentButtonState = buttonActive;

  // Water button (Pin 33) - single press for pump
  if (currentButtonState && !lastButtonState) {
    LOGI("BTN", "Water button pressed - starting pump for %lums", PUMP_MS_DEFAULT);
    startPump(PUMP_MS_DEFAULT);
  }
  lastButtonState = currentButtonState;

  // 2) Always run AP server
  apServer.handleClient();

  // 3) Sanity clamp: if pumpRunning==false but pin HIGH -> force LOW
  if (!pumpRunning && digitalRead(PIN_MOTOR) == HIGH) {
    digitalWrite(PIN_MOTOR, LOW);
    LOGW("PUMP", "forced OFF (sanity clamp)");
  }

  // 4) Pump auto-off
  stopPumpIfDue();

  // 5) Sample sensors ~2s
  static unsigned long lastSample=0; 
  if (millis() - lastSample >= 2000){
    lastSample = millis();
    soil_mv  = readMedianMilliVolts(PIN_SOIL);
    soil_pct = soilMvToPercent(soil_mv);
    light_mv  = readMedianMilliVolts(PIN_LIGHT);
    light_pct = lightMvToPercent(light_mv);
    air_hum  = dht.readHumidity();
    air_temp = dht.readTemperature();
    LOGD("SENS", "Soil:%d mV(%d%%) Light:%d mV(%d%%) Temp:%.1fC Hum:%.1f%%",
         soil_mv, soil_pct, light_mv, light_pct, air_temp, air_hum);
  }

  // 6) MQTT service + backoff
  static unsigned long nextTry = 0;
  if (WiFi.status()==WL_CONNECTED){
    if (!mqtt.connected()){
      if ((long)(millis() - nextTry) >= 0){
        ensureMqtt();
        nextTry = millis() + 3000;
      }
    } else {
      mqtt.loop();
      static unsigned long lastPub=0; 
      if (millis() - lastPub >= PUB_MS){ lastPub = millis(); publishTelemetry(); }

      // Delayed retained-cleanup retry (covers late LWT re-add)
      if (cleanupRetriesLeft && (long)(millis() - cleanupRetryAt) >= 0 && cleanupTopicPending.length()) {
        if (deleteRetained(cleanupTopicPending)) {
          cleanupRetriesLeft--;
          cleanupRetryAt = millis() + 65000;
        }
        if (cleanupRetriesLeft == 0) { cleanupTopicPending = ""; }
      }
    }
  }
}