#include <Wire.h>
#include <SPI.h>
#include <MFRC522.h>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Pin configuration
#define SS_PIN 5       // Pin for SDA/SS
#define RST_PIN 4      // Pin for RST
#define BUZZER_PIN 17  // Pin for Buzzer
#define LED_PIN 2      // Pin for LED

// RGB LED pins
#define RED_PIN 26
#define GREEN_PIN 25
#define BLUE_PIN 33

// LCD configuration
#define LCD_ADDRESS 0x27
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

// Server configuration
#define SERVER_GET_URL "https://presensi.smkn1margasekampung.sch.id/esp32/api/pengaturan"
#define SERVER_POST_URL "https://presensi.smkn1margasekampung.sch.id/esp32/api/simpan"

// WiFi credentials
#define WIFI_SSID "SMKN 1 MARGA SEKAMPUNG"
#define WIFI_PASSWORD "marse2023456"

MFRC522 rfid(SS_PIN, RST_PIN);

// Global variables
String apikey = "dcZrym";
String mode = "reader";
String perangkat = "5";
unsigned long lastCardReadTime = 0;
const unsigned long cardReadInterval = 2000; // 2-second cooldown between reads
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 5000; // Check WiFi every 5 seconds

void setColor(int red, int green, int blue) {
  analogWrite(RED_PIN, red);
  analogWrite(GREEN_PIN, green);
  analogWrite(BLUE_PIN, blue);
}

void setLEDGreen() { setColor(0, 255, 0); }
void setLEDBlue() { setColor(0, 0, 255); }
void setLEDRed() { setColor(255, 0, 0); }
void turnOffLED() { setColor(0, 0, 0); }

void quickBeep(int duration = 100) {
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration);
  digitalWrite(BUZZER_PIN, LOW);
}

void displayMessage(String line1, String line2 = "", bool clear = true) {
  if (clear) lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(line1);
  if (line2 != "") {
    lcd.setCursor(0, 1);
    lcd.print(line2);
  }
}

void setupWiFi() {
  WiFi.disconnect(true);
  delay(100);
  
  // Configure WiFi for maximum performance
  WiFi.mode(WIFI_STA);
  esp_wifi_set_ps(WIFI_PS_NONE);           // Disable power save
  esp_wifi_set_max_tx_power(84);           // Max TX power (20dBm)
  WiFi.setSleep(false);                    // Disable sleep
  WiFi.setAutoReconnect(true);             // Auto-reconnect
  WiFi.persistent(false);                  // Don't save config
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  unsigned long startTime = millis();
  displayMessage("Connecting to", "WiFi...", true);
  
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
    delay(250);
    Serial.print(".");
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected!");
    Serial.print("RSSI: ");
    Serial.println(WiFi.RSSI());
    displayMessage("WiFi Connected!", String("Signal: ") + WiFi.RSSI() + "dBm", true);
    quickBeep(100);
  } else {
    Serial.println("\nConnection failed");
    displayMessage("WiFi Failed", "Retrying...", true);
    setLEDRed();
    quickBeep(500);
  }
}

void checkWiFiConnection() {
  if (millis() - lastWiFiCheck >= wifiCheckInterval) {
    lastWiFiCheck = millis();
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi disconnected. Reconnecting...");
      setupWiFi();
    }
  }
}

String getServerSettings() {
  HTTPClient http;
  http.begin(String(SERVER_GET_URL) + "/" + apikey);
  http.setTimeout(3000); // Reduced timeout to 3 seconds
  
  if (http.GET() == 200) {
    String payload = http.getString();
    http.end();
    return payload;
  }
  http.end();
  return "";
}

void postUIDToServer(String uid) {
  HTTPClient http;
  http.begin(SERVER_POST_URL);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  http.setTimeout(3000); // Reduced timeout to 3 seconds

  String postData = "uid=" + uid + "&apikey=" + apikey;
  
  displayMessage("Sending data...", "", true);
  quickBeep(50);

  int httpCode = http.POST(postData);
  
  if (httpCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(256);
    deserializeJson(doc, response);

    String status = doc["status"];
    String message = doc["message"];
    String name = doc["fullname"];

    if (status == "success") {
      setLEDGreen();
      displayMessage(name != "null" ? name : "Success", message);
      quickBeep(100);
    } else {
      setLEDRed();
      displayMessage("Error", message);
      quickBeep(300);
    }
  } else {
    setLEDRed();
    displayMessage("Server Error", "Code: " + String(httpCode));
    quickBeep(300);
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  turnOffLED();

  displayMessage("System Booting", "Please wait...");
  quickBeep(100);

  setupWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    String settings = getServerSettings();
    if (!settings.isEmpty()) {
      DynamicJsonDocument doc(256);
      deserializeJson(doc, settings);
      mode = doc["mode"].as<String>();
      displayMessage("Mode:", mode);
      delay(1000);
    }
  }

  displayMessage("Ready for", "Attendance");
  setLEDBlue();
}

void loop() {
  checkWiFiConnection();

  // Check for new cards with minimal delay
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    delay(10); // Very short delay for responsiveness
    return;
  }

  // Prevent rapid successive reads
  if (millis() - lastCardReadTime < cardReadInterval) {
    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
    return;
  }
  lastCardReadTime = millis();

  // Read UID
  String uidString;
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) uidString += "0";
    uidString += String(rfid.uid.uidByte[i], HEX);
  }

  // Immediate feedback
  quickBeep(30); // Very short beep
  setLEDBlue();
  displayMessage("Processing...", "");

  // Only send if WiFi is connected
  if (WiFi.status() == WL_CONNECTED) {
    postUIDToServer(uidString);
  } else {
    setLEDRed();
    displayMessage("Offline Mode", "Data not sent");
    quickBeep(500);
  }

  // Clear card and return to ready state
  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
  delay(100); // Short delay before next read
  setLEDBlue();
  displayMessage("Ready for", "Attendance");
}