#pragma once

// Copy this folder into Arduino IDE or open esp32-camera-streamer.ino directly.
// Edit these values for each ESP32-CAM before upload.

#define WIFI_SSID "YOUR_WIFI_NAME"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// Give every ESP32-CAM a stable unique ID. This is what the app stores.
#define CAMERA_ID "esp32-cam-01"
#define CAMERA_NAME "ESP32 Camera 01"

// Backend registration endpoint. Use the LAN IP of the machine running backend.
// Example: "http://192.168.1.50:5050/api/cameras/register"
#define DETECTION_SERVER_REGISTER_URL "http://YOUR_SERVER_LAN_IP:5050/api/cameras/register"

// Leave empty in normal LAN use. Set only if the app should use a different
// hostname/IP than WiFi.localIP() for this camera's stream URL.
#define STREAM_HOST_OVERRIDE ""

// AI Thinker ESP32-CAM is the common module. Other modules need pin changes in
// esp32-camera-streamer.ino.
#define CAMERA_MODEL_AI_THINKER

// Stream tuning. Lower frame size is more stable on weak WiFi.
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA
#define CAMERA_JPEG_QUALITY 12
#define CAMERA_FB_COUNT 2

// How often this camera re-announces itself to the Node backend.
#define REGISTER_INTERVAL_MS 60000
