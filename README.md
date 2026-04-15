# ESP32-CAM Streamer

Arduino firmware for AI Thinker ESP32-CAM modules. It exposes:

- `http://<esp32-ip>/` control/status page
- `http://<esp32-ip>/capture` single JPEG snapshot
- `http://<esp32-ip>:81/stream` MJPEG live stream
- periodic registration to the Node backend at `/api/cameras/register`

## Upload

1. Install the ESP32 board package in Arduino IDE.
2. Select board: `AI Thinker ESP32-CAM`.
3. Edit `config.h`:
   - `WIFI_SSID`
   - `WIFI_PASSWORD`
   - `CAMERA_ID`
   - `CAMERA_NAME`
   - `DETECTION_SERVER_REGISTER_URL`
4. Upload `esp32-camera-streamer.ino`.
5. Open Serial Monitor at `115200` baud and press reset.

Example backend URL:

```cpp
#define DETECTION_SERVER_REGISTER_URL "http://192.168.1.50:5050/api/cameras/register"
```

## App Integration

When the ESP32 boots, it posts this camera record to the backend:

```json
{
  "id": "esp32-cam-01",
  "name": "ESP32 Camera 01",
  "type": "esp32-cam",
  "streamUrl": "http://192.168.1.77:81/stream",
  "controlUrl": "http://192.168.1.77/",
  "ip": "192.168.1.77",
  "firmware": "esp32-camera-streamer-0.1.0"
}
```

Registered ESP32 cameras appear in the app's camera source dropdown. You can
also manually set a camera source to:

```text
http://<esp32-ip>:81/stream
```

The frontend displays ESP32 streams directly as MJPEG. If you want the backend
to restream MJPEG through mediamtx/WHEP for server-side detection, start the
backend with:

```bash
MEDIA_BRIDGE_MJPEG=1 npm run dev
```

That requires `ffmpeg` and `mediamtx`.

## Per-Camera IDs

Use a unique `CAMERA_ID` per ESP32. Recommended naming:

```cpp
#define CAMERA_ID "garage-esp32-01"
#define CAMERA_NAME "Garage ESP32 01"
```

If `CAMERA_ID` is blank, the firmware generates one from the ESP32 chip ID.
