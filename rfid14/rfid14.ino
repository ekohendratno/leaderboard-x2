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

// Pin configuration for RGB LED
#define RED_PIN 26    // Pin for Red color
#define GREEN_PIN 25  // Pin for Green color
#define BLUE_PIN 33   // Pin for Blue color

// I2C address for LCD
#define LCD_ADDRESS 0x27

// Server URLs
#define SERVER_GET_URL "https://presensi.smkn1margasekampung.sch.id/esp32/api/pengaturan"
#define SERVER_POST_URL "https://presensi.smkn1margasekampung.sch.id/esp32/api/simpan"

// Wi-Fi credentials
#define WIFI_SSID "SMKN 1 MARGA SEKAMPUNG"    
#define WIFI_PASSWORD "marse2023456"

// Create LCD object
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);  // 16 columns, 2 rows

MFRC522 rfid(SS_PIN, RST_PIN);

String apikey = "dcZrym";
String mode = "reader";  // Default mode
String perangkat = "5";

unsigned long previousMillis = 0;
const long interval = 1000;  // Interval at which to blink (1000 ms = 1 second)
bool ledState = LOW;         // Initial LED state

void blinkBlueLED() {
  unsigned long currentMillis = millis();

  // Check if it's time to toggle the LED
  if (currentMillis - previousMillis >= interval) {
    // Save the last time the LED blinked
    previousMillis = currentMillis;

    // Toggle the LED state
    if (ledState == LOW) {
      ledState = HIGH;
      setLEDBlue();
    } else {
      ledState = LOW;
      turnOffLED();
    }
  }
}

// Fungsi untuk mengatur warna LED
void setColor(int red, int green, int blue) {
  // Kendalikan LED merah
  analogWrite(RED_PIN, red);

  // Kendalikan LED hijau
  analogWrite(GREEN_PIN, green);

  // Kendalikan LED biru
  analogWrite(BLUE_PIN, blue);
}

void setLEDGreen() {
  setColor(0, 255, 0);  // Hijau
}

void setLEDBlue() {
  setColor(0, 0, 255);  // Biru
}

void setLEDRed() {
  setColor(255, 0, 0);  // Merah
}

void turnOffLED() {
  setColor(LOW, LOW, LOW);  // Turn off LED
}

void blinkLEDAndBuzzer(int times, int delayMs) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    digitalWrite(BUZZER_PIN, LOW);
    delay(delayMs);
  }
}

void triggerErrorBuzzer() {
  // Error notification pattern (longer beep)

  setLEDRed();
  digitalWrite(BUZZER_PIN, HIGH);
  delay(1000);
  digitalWrite(BUZZER_PIN, LOW);
}

void displayMessage(String line1, String line2, int delayMs) {
  lcd.clear();

  // Tampilkan line1
  scrollText(line1, 0, delayMs);

  // Tampilkan line2
  scrollText(line2, 1, delayMs);
}

// Fungsi bantu: scroll teks jika lebih dari 16 karakter
void scrollText(String text, int row, int delayMs) {
  int len = text.length();
  int displayWidth = 16;

  if (len <= displayWidth) {
    lcd.setCursor(0, row);
    lcd.print(text);
    delay(delayMs);
  } else {
    for (int i = 0; i <= len - displayWidth; i++) {
      String segment = text.substring(i, i + displayWidth);
      lcd.setCursor(0, row);
      lcd.print(segment);
      delay(300); // kecepatan scroll
    }

    // Tambahan delay setelah selesai scroll
    delay(delayMs);
  }
}


bool connectToWiFi(const char* ssid, const char* password, int maxAttempts) {
  int attempts = 0;
  WiFi.begin(ssid, password);

  // Try connecting to WiFi with a limit on the number of attempts
  while (WiFi.status() != WL_CONNECTED && attempts < maxAttempts) {
    delay(500);
    Serial.print(".");
    attempts++;
    displayMessage("Menghungkan WiFi", "Tunggu Sebentar...", 500);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Gagal konek Wi-Fi");
    triggerErrorBuzzer();                            // Play error sound
    displayMessage("Koneksi WiFi", "Gagal!", 2000);  // Show error message
    return false;                                    // Connection failed
  }

  // Successfully connected to Wi-Fi
  Serial.println("Connected to Wi-Fi");
  displayMessage("Koneksi WiFi", "Tersambung", 2000);  // Show success message

  // Display ESP32 IP address
  String ipAddress = WiFi.localIP().toString();
  Serial.println("ESP32 IP Address: " + ipAddress);
  displayMessage("Alamat Perangkat", ipAddress, 4000);  // Display IP on the LCD

  return true;
}

void reconnectWiFi(int maxAttempts) {
  int attempts = 0;
  while (!checkInternetConnection() ) { //&& attempts < maxAttempts
    displayMessage("Reconnecting...", "to the internet", 2000);
    delay(5000);  // Wait before trying again
    WiFi.reconnect();
    attempts++;
  }

  if (!checkInternetConnection()) {
    triggerErrorBuzzer();  // Play error sound
    displayMessage("No internet", "Check network", 0);
    while (true) {
      delay(10000);  // Stop further execution
    }
  }
}

bool checkInternetConnection() {
  HTTPClient http;
  http.begin("http://www.google.com");  // Check for internet connection
  int httpCode = http.GET();
  http.end();

  return (httpCode > 0);
}

String getServerSettings(const char* url, const String& apikey, const String& perangkat) {
  String fullUrl = String(url) + "/" + apikey;
  Serial.println(fullUrl);

  HTTPClient http;
  http.begin(fullUrl);
  int httpCode = http.GET();
  String payload = "";
  if (httpCode == 200) {
    payload = http.getString();
  } else {
    triggerErrorBuzzer();  // Play error sound
    Serial.println("Gagal ambil pengaturan");
  }
  http.end();
  return payload;
}

void postUIDToServer(const char* url, const String& uid, const String& mode, const String& apikey, const String& perangkat) {
  HTTPClient http;
  http.begin(url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "uid=" + uid + "&apikey=" + apikey;

  Serial.println(url);
  Serial.println(postData);

  int httpCode = http.POST(postData);

  if (httpCode == 200) {
    String response = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, response);

    String status = doc["status"].as<String>();
    String message = doc["message"].as<String>();
    String name = doc["fullname"].as<String>();

    if (status == "success") {
      // Cek apakah 'name' kosong atau null, jika iya, isi dengan 'Success'
      if (name == "" || name == "null" || name == NULL) {
        name = "Berhasil";
      }

      setLEDGreen();
      displayMessage(name, message, 1000);
    } else if (status == "error") {
      triggerErrorBuzzer();  // Play error sound
      displayMessage("Gagal", message, 2000);
    } else {
      displayMessage("System in", "Maintenance", 2000);
    }
  } else {
    triggerErrorBuzzer();  // Play error sound
    displayMessage("Gagal kirim UID", "Cek koneksi/server", 5000);
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

  turnOffLED();


  lcd.init();
  lcd.backlight();

  blinkLEDAndBuzzer(1, 500);

  displayMessage("Menghidupkan...", "", 2000);
  displayMessage("Perangkat", "Perangkat Siap...", 1000);

  // Attempt to connect to WiFi using defined credentials
  while (!connectToWiFi(WIFI_SSID, WIFI_PASSWORD, 20)) {
    // If Wi-Fi connection fails, stay in the loop trying to reconnect
    displayMessage("Tidak Konek WiFi", "Periksa WiFi...", 2000);
  }

  reconnectWiFi(5);

  String payload = getServerSettings(SERVER_GET_URL, apikey, perangkat);
  if (!payload.isEmpty()) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);

    mode = doc["mode"].as<String>();
    Serial.println("Mode: " + mode);

    displayMessage("Mode Perangkat", mode, 0);
    blinkLEDAndBuzzer(2, 500);

    if (mode == "reader") {
      setLEDBlue();
      displayMessage("Selamat Datang", "Assallamuallakum...", 1000);
      displayMessage("Presensi Siap!", "Tap Kartu...", 500);
    } else if (mode == "cek") {
      setLEDBlue();
      displayMessage("Tap Kartu", "Utk Cek Data", 1000);
    } else if (mode == "set") {
      setLEDBlue();
      displayMessage("Tap Kartu", "Utk Tambah Data", 1000);
    } else if (mode == "set") {
      setLEDBlue();
      displayMessage("Tap Kartu", "Utk Ubah Data", 1000);
    } else {
      triggerErrorBuzzer();  // Play error sound
      displayMessage("Unknown mode", "Cek pengaturan", 1000);
    }
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    // Ensure WiFi is connected
    displayMessage("Ulangi WiFi", "Tunggu Sebentar...", 2000);
    reconnectWiFi(5);
  }

  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    // Blink the blue LED while waiting for the card
    blinkBlueLED();
    // displayMessage("Siap Presensi!", "Tempel Kartu...", 1000);

    if (mode == "reader") {
      displayMessage("Presensi Siap!", "Tap Kartu...", 500);
    } else if (mode == "cek") {
      displayMessage("Tap Kartu", "Utk Cek Data", 1000);
    } else if (mode == "add") {
      displayMessage("Tap Kartu", "Utk Tambah Data", 1000);
    } else if (mode == "set") {
      displayMessage("Tap Kartu", "Utk Ubah Data", 1000);
    } else {
      displayMessage("Unknown mode", "Check settings", 1000);
    }

    return;
  }

  String uidString = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    if (rfid.uid.uidByte[i] < 0x10) {
      uidString += "0";
    }
    uidString += String(rfid.uid.uidByte[i], HEX);
  }


   // Stop blinking and send the data to the server
  turnOffLED();
  displayMessage("Kirim Presensi", "Mohon Tunggu...", 0);
  blinkLEDAndBuzzer(2, 100);
  postUIDToServer(SERVER_POST_URL, uidString, mode, apikey, perangkat);

  rfid.PICC_HaltA();
  rfid.PCD_StopCrypto1();
}
