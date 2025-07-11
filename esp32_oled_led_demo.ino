
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "time.h"

const char* ssid = "TP-Link_2.4G";
const char* password = "123456789#";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;
const char* apiKey = "YOUR_API_KEY";

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LED_PIN 4
#define LED_COUNT 16
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

float temperature = 0.0;
String description = "";
bool wifiConnected = false;
int lastSec = -1;
uint32_t currentColor = 0;
unsigned long lastWeatherCheck = 0;
unsigned long lastBlink = 0;
bool ledOn = false;
int wifiAnimPos = 0;
unsigned long lastWiFiAnim = 0;

void getWeather() {
  String url = "http://api.openweathermap.org/data/2.5/weather?q=Thai%20Nguyen,vn&appid=" + String(apiKey) + "&units=metric";
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error) {
      temperature = doc["main"]["temp"].as<float>();
      description = doc["weather"][0]["description"].as<String>();
    }
  }
  http.end();
}

void drawWiFiIcon(int x, int y) {
  int rssi = WiFi.RSSI();
  int level = 0;
  if (rssi >= -55) level = 4;
  else if (rssi >= -65) level = 3;
  else if (rssi >= -75) level = 2;
  else if (rssi >= -85) level = 1;
  else level = 0;

  int heights[4] = {3, 5, 7, 9};
  for (int i = 0; i < 4; i++) {
    int h = heights[i];
    int colX = x + i * 3;
    int colY = y + 10 - h;
    if (i < level) {
      display.fillRect(colX, colY, 2, h, SSD1306_WHITE);
    } else {
      display.drawRect(colX, colY, 2, h, SSD1306_WHITE);
    }
  }
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED failed");
    while (1);
  }

  ring.begin();
  ring.show();
  currentColor = ring.Color(255, 0, 0);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Connecting WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStart < 5000) {
    unsigned long now = millis();
    if (now - lastWiFiAnim > 200) {
      lastWiFiAnim = now;
      ring.clear();
      for (int i = 0; i < 3; i++) {
        int pos = (wifiAnimPos + i) % LED_COUNT;
        ring.setPixelColor(pos, ring.Color(255, 0, 0));
      }
      ring.show();
      wifiAnimPos = (wifiAnimPos + 1) % LED_COUNT;
    }
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    getWeather();
  } else {
    wifiConnected = false;
  }
}

void loop() {
  if (!wifiConnected) {
    if (millis() - lastBlink > 500) {
      lastBlink = millis();
      ledOn = !ledOn;
      if (ledOn) {
        for (int i = 0; i < LED_COUNT; i++) {
          ring.setPixelColor(i, ring.Color(255, 0, 0));
        }
      } else {
        ring.clear();
      }
      ring.show();
    }
    return;
  }

  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    int sec = timeinfo.tm_sec % 60;
    if (sec != lastSec) {
      lastSec = sec;
      ring.clear();
      int index = sec % LED_COUNT;
      ring.setPixelColor(index, currentColor);
      ring.show();
      if (index == 0) {
        currentColor = ring.Color(random(0, 255), random(0, 255), random(0, 255));
      }
    }

    display.clearDisplay();
    display.drawLine(0, 10, 127, 10, SSD1306_WHITE);
    drawWiFiIcon(0, 0);
    display.setCursor(100, 0);
    display.printf("%.1fC", temperature);

    display.setTextSize(2);
    char timeStr[9];
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", &timeinfo);
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(timeStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 20);
    display.println(timeStr);

    display.setTextSize(1);
    display.getTextBounds(description, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 42);
    display.println(description);

    display.drawLine(0, 54, 127, 54, SSD1306_WHITE);
    char dateStr[20];
    strftime(dateStr, sizeof(dateStr), "%a %d/%m/%Y", &timeinfo);
    display.getTextBounds(dateStr, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, 56);
    display.println(dateStr);

    display.display();
  }

  if (millis() - lastWeatherCheck > 600000) {
    getWeather();
    lastWeatherCheck = millis();
  }

  delay(200);
}
