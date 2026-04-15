#include "esp_camera.h"
#include <HTTPClient.h>
#include <WebServer.h>
#include <WiFi.h>

#include "config.h"

#define FIRMWARE_VERSION "esp32-camera-streamer-0.1.0"

#if defined(CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#else
#error "Set a supported camera model in config.h or add your module pin map."
#endif

WebServer controlServer(80);
WebServer streamServer(81);

unsigned long lastRegisterAt = 0;

String cameraId() {
  String configured = String(CAMERA_ID);
  configured.trim();
  if (configured.length() > 0) return configured;
  uint64_t chipId = ESP.getEfuseMac();
  char id[32];
  snprintf(id, sizeof(id), "esp32-cam-%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  return String(id);
}

String streamHost() {
  String overrideHost = String(STREAM_HOST_OVERRIDE);
  overrideHost.trim();
  if (overrideHost.length() > 0) return overrideHost;
  return WiFi.localIP().toString();
}

String streamUrl() {
  return "http://" + streamHost() + ":81/stream";
}

String controlUrl() {
  return "http://" + streamHost() + "/";
}

String jsonEscape(const String &value) {
  String out;
  out.reserve(value.length() + 8);
  for (size_t i = 0; i < value.length(); i += 1) {
    char c = value[i];
    if (c == '"' || c == '\\') out += '\\';
    if (c == '\n') out += "\\n";
    else if (c == '\r') out += "\\r";
    else out += c;
  }
  return out;
}

void addCors() {
  controlServer.sendHeader("Access-Control-Allow-Origin", "*");
  controlServer.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  controlServer.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

String statusJson() {
  String payload = "{";
  payload += "\"id\":\"" + jsonEscape(cameraId()) + "\",";
  payload += "\"name\":\"" + jsonEscape(String(CAMERA_NAME)) + "\",";
  payload += "\"type\":\"esp32-cam\",";
  payload += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  payload += "\"rssi\":" + String(WiFi.RSSI()) + ",";
  payload += "\"streamUrl\":\"" + jsonEscape(streamUrl()) + "\",";
  payload += "\"controlUrl\":\"" + jsonEscape(controlUrl()) + "\",";
  payload += "\"firmware\":\"" + String(FIRMWARE_VERSION) + "\",";
  payload += "\"uptimeMs\":" + String(millis());
  payload += "}";
  return payload;
}

bool registerWithServer() {
  String registerUrl = String(DETECTION_SERVER_REGISTER_URL);
  registerUrl.trim();
  if (registerUrl.length() == 0 || registerUrl.indexOf("YOUR_SERVER_LAN_IP") >= 0) {
    Serial.println("[register] skipped: configure DETECTION_SERVER_REGISTER_URL in config.h");
    return false;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[register] skipped: WiFi not connected");
    return false;
  }

  HTTPClient http;
  http.begin(registerUrl);
  http.addHeader("Content-Type", "application/json");

  String payload = statusJson();
  int status = http.POST(payload);
  String response = http.getString();
  http.end();

  Serial.printf("[register] POST %s -> %d\n", registerUrl.c_str(), status);
  if (response.length() > 0) {
    Serial.println(response);
  }
  return status >= 200 && status < 300;
}

bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = CAMERA_FRAME_SIZE;
  config.jpeg_quality = CAMERA_JPEG_QUALITY;
  config.fb_count = CAMERA_FB_COUNT;
  config.grab_mode = CAMERA_GRAB_LATEST;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[camera] init failed: 0x%x\n", err);
    return false;
  }

  sensor_t *sensor = esp_camera_sensor_get();
  if (sensor) {
    sensor->set_vflip(sensor, 0);
    sensor->set_hmirror(sensor, 0);
  }
  return true;
}

void handleRoot() {
  addCors();
  String html = "<!doctype html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
  html += "<title>" + String(CAMERA_NAME) + "</title></head><body style=\"font-family:sans-serif;background:#0b1514;color:#eef8ed\">";
  html += "<h1>" + String(CAMERA_NAME) + "</h1>";
  html += "<p>ID: " + cameraId() + "</p>";
  html += "<p>Stream URL: <code>" + streamUrl() + "</code></p>";
  html += "<p><a style=\"color:#89c36b\" href=\"/status\">Status JSON</a> ";
  html += "<a style=\"color:#89c36b\" href=\"/capture\">Snapshot</a> ";
  html += "<a style=\"color:#89c36b\" href=\"http://" + streamHost() + ":81/stream\">MJPEG Stream</a></p>";
  html += "<img src=\"http://" + streamHost() + ":81/stream\" style=\"max-width:100%;height:auto;background:#000\">";
  html += "</body></html>";
  controlServer.send(200, "text/html", html);
}

void handleStatus() {
  addCors();
  controlServer.send(200, "application/json", statusJson());
}

void handleRegisterNow() {
  addCors();
  bool ok = registerWithServer();
  controlServer.send(ok ? 200 : 500, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false}");
}

void handleCapture() {
  addCors();
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    controlServer.send(503, "text/plain", "camera capture failed");
    return;
  }
  controlServer.sendHeader("Content-Disposition", "inline; filename=capture.jpg");
  controlServer.send_P(200, "image/jpeg", (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
}

void handleStream() {
  WiFiClient client = streamServer.client();
  String response = "HTTP/1.1 200 OK\r\n";
  response += "Access-Control-Allow-Origin: *\r\n";
  response += "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n";
  client.print(response);

  while (client.connected()) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[stream] frame capture failed");
      delay(50);
      continue;
    }

    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.print("\r\n");
    esp_camera_fb_return(fb);

    if (!client.connected()) break;
    delay(20);
  }
}

void connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  Serial.printf("[wifi] connecting to %s", WIFI_SSID);
  unsigned long startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 30000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[wifi] connection failed; restarting in 5 seconds");
    delay(5000);
    ESP.restart();
  }

  Serial.print("[wifi] connected: ");
  Serial.println(WiFi.localIP());
}

void setupServers() {
  controlServer.on("/", HTTP_GET, handleRoot);
  controlServer.on("/status", HTTP_GET, handleStatus);
  controlServer.on("/register", HTTP_GET, handleRegisterNow);
  controlServer.on("/capture", HTTP_GET, handleCapture);
  controlServer.onNotFound([]() {
    addCors();
    controlServer.send(404, "application/json", "{\"error\":\"not found\"}");
  });

  streamServer.on("/stream", HTTP_GET, handleStream);

  controlServer.begin();
  streamServer.begin();
  Serial.println("[server] control: " + controlUrl());
  Serial.println("[server] stream:  " + streamUrl());
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(false);
  delay(300);
  Serial.println();
  Serial.println(FIRMWARE_VERSION);

  connectWifi();
  if (!initCamera()) {
    delay(5000);
    ESP.restart();
  }
  setupServers();
  registerWithServer();
  lastRegisterAt = millis();
}

void loop() {
  controlServer.handleClient();
  streamServer.handleClient();

  if (millis() - lastRegisterAt >= REGISTER_INTERVAL_MS) {
    registerWithServer();
    lastRegisterAt = millis();
  }
}
