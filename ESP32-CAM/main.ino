#include <WebServer.h>
#include <WiFi.h>
#include <esp32cam.h>

// WiFi credentials
const char* WIFI_SSID = "wifi";
const char* WIFI_PASS = "pass";

// Create a web server on port 80
WebServer server(80);

// GPIO for flash
#define FLASH_GPIO_NUM 4

// Set the camera resolution
static auto midRes = esp32cam::Resolution::find(350, 530);

// Function to capture and serve a JPEG image
void serveJpg() {
  auto frame = esp32cam::capture();
  if (frame == nullptr) {
    //Serial.println("CAPTURE FAIL");
    server.send(503, "text/plain", "Capture failed");
    return;
  }

  server.setContentLength(frame->size());
  server.sendHeader("Content-Type", "image/jpeg");
  server.send(200);

  WiFiClient client = server.client();
  frame->writeTo(client);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  pinMode(FLASH_GPIO_NUM, OUTPUT);

  // Initialize the camera
  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(midRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(80);

    bool ok = Camera.begin(cfg);
    //Serial.println(ok ? "CAMERA OK" : "CAMERA FAIL");
    //digitalWrite(FLASH_GPIO_NUM, ok ? HIGH : LOW);
  }

  // Initialize WiFi
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long startAttemptTime = millis();
  const unsigned long wifiTimeout = 10000;  // 10 seconds timeout

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < wifiTimeout) {
    delay(500);
    //Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    //Serial.println("WiFi connection failed!");
    return;
  }

  Serial.print("http://");
  Serial.println(WiFi.localIP());
  Serial.println("  /capture");

  // Define route and start the server
  server.on("/capture", serveJpg);
  server.begin();
}

void loop() {
  static unsigned long lastCaptureTime = 0;
  const unsigned long captureInterval = 10000;  // 10 seconds

  // Automatically capture and serve image at intervals
  if (millis() - lastCaptureTime >= captureInterval) {
    serveJpg();
    lastCaptureTime = millis();
  }

  // Handle client requests
  server.handleClient();
}
