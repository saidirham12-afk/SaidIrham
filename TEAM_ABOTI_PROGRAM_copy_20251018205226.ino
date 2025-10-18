 #include <RTClib.h>
#include "UbidotsEsp32Mqtt.h"
#include <Wire.h>
#include "Adafruit_SHT31.h"
#include "Adafruit_HTU21DF.h"
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include <SD.h>
#include <WiFiManager.h>  

#define TOKEN "BBUS-Qk04BfTpVqQfEVE4M9qiOl9nEUTG9T"
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_RESET -1
#define SD_CS 5
 
Ubidots ubidots(TOKEN);
Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_HTU21DF htu = Adafruit_HTU21DF();
Adafruit_SSD1306 display(OLED_WIDTH, OLED_HEIGHT, &Wire, OLED_RESET);
RTC_DS3231 rtc;
File dataFile;  

// =============================
// FUNGSI TAMBAHAN
// =============================
void tampilkanErrorOLED(String pesan) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(pesan, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  int16_t y = (OLED_HEIGHT - h) / 2;

  display.setCursor(x, y);
  display.println(pesan);
  display.display();
  delay(3000);
}

void tampilkanTeksTengah(String teks, int delayMs = 1500) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(teks, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  int16_t y = (OLED_HEIGHT - h) / 2;
  display.setCursor(x, y);
  display.println(teks);
  display.display();
  delay(delayMs);
}

// Fungsi animasi loading
void tampilkanLoading(String teks) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(teks, 0, 0, &x1, &y1, &w, &h);
  int16_t x = (OLED_WIDTH - w) / 2;
  int16_t y = (OLED_HEIGHT - h) / 2;

  for (int i = 0; i < 3; i++) {
    display.clearDisplay();
    display.setCursor(x, y);
    display.print(teks);
    for (int j = 0; j <= i; j++) {
      display.print(".");
    }
    display.display();
    delay(500);
  }
}

// =============================
// SETUP
// =============================
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("Inisialisasi sistem...");

  // OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal ditemukan!");
    while (1);
  }

  // INTRO
  tampilkanTeksTengah("Project Monitoring IOT");
  delay(1000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 10);
  display.println("Kelompok ABOTI");
  display.println("");
  display.println("1. Said Irham");
  display.println("2. Angel Debora");
  display.println("3. Muh Rizky");
  display.display();
  delay(3000);

  // ANIMASI LOADING SAAT MULAI
  tampilkanLoading("Memuat Sistem");

  // ==============================
  // WiFi Manager dengan Timeout & Loading
  // ==============================
  WiFiManager wifiManager;
  wifiManager.setTimeout(10);  // batas waktu 10 detik

  unsigned long startTime = millis();
  int titik = 0;
  bool wifiConnected = false;

  // Animasi loading OLED selama menunggu WiFi
  while ((millis() - startTime) < 10000) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(20, 25);
    display.print("Menghubungkan WiFi");

    for (int i = 0; i < titik; i++) {
      display.print(".");
    }
    display.display();

    titik++;
    if (titik > 3) titik = 0;
    delay(500);

    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
      break;
    }
  }

  if (!wifiConnected && !wifiManager.autoConnect("ESP32-Monitor")) {
    Serial.println("Gagal terhubung ke WiFi!");
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(10, 20);
    display.println("WiFi Tidak Terhubung!");
    display.println("Silakan hubungkan");
    display.println("melalui WiFiManager.");
    display.display();
    delay(3000);
  } else {
    Serial.println("WiFi terhubung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    tampilkanTeksTengah("WiFi Terhubung!");
  }

  // Hubungkan ke Ubidots
  ubidots.setup();

  // Sensor
  if (!sht31.begin(0x44)) {
    tampilkanErrorOLED("Error: SHT31");
    Serial.println("SHT31 tidak ditemukan!");
    while (1);
  }
  if (!htu.begin()) {
    tampilkanErrorOLED("Error: HTU21D");
    Serial.println("HTU21D tidak ditemukan!");
    while (1);
  }

  // RTC
  if (!rtc.begin()) {
    tampilkanErrorOLED("Error: RTC");
    Serial.println("RTC tidak terdeteksi!");
  }
  if (rtc.lostPower()) {
    tampilkanErrorOLED("Setel ulang RTC...");
    Serial.println("RTC kehilangan daya! Menyetel waktu baru...");
  }

  // SD CARD
  if (!SD.begin(SD_CS)) {
    tampilkanErrorOLED("Error: SD Card");
    Serial.println("Gagal inisialisasi SD Card!");
  } else {
    Serial.println("SD Card OK");
  }

  // Sistem Siap
  tampilkanTeksTengah("Sistem Siap");
}

// =============================
// LOOP
// =============================
void loop() {
  if (!ubidots.connected()) {
    ubidots.reconnect();
  }

  // Baca waktu
  DateTime now;
  bool rtcOK = true;
  if (rtc.begin()) {
    now = rtc.now();
  } else {
    rtcOK = false;
  }

  // Baca sensor
  float suhu1 = sht31.readTemperature();
  float humd1 = sht31.readHumidity();
  float suhu2 = htu.readTemperature();
  float humd2 = htu.readHumidity();

  if (isnan(suhu1) || isnan(humd1) || isnan(suhu2) || isnan(humd2)) {
    tampilkanErrorOLED("Error: Sensor Tidak Terbaca");
    Serial.println("Error membaca sensor!");
    return;
  }

  // Serial
  Serial.println("===========================");
  Serial.print("SHT31 -> Suhu: "); Serial.print(suhu1);
  Serial.print(" °C | Hum: "); Serial.println(humd1);
  Serial.print("HTU21 -> Suhu: "); Serial.print(suhu2);
  Serial.print(" °C | Hum: "); Serial.println(humd2);

  // Kirim ke Ubidots
  ubidots.add("suhu_sht31", suhu1);
  ubidots.add("kelembaban_sht31", humd1);
  ubidots.publish("Kamar");

  ubidots.add("suhu_htu21", suhu2);
  ubidots.add("kelembaban_htu21", humd2);
  ubidots.publish("RuangTamu");

  // Simpan ke SD
  if (SD.begin(SD_CS)) {
    dataFile = SD.open("/data.csv", FILE_APPEND);
    if (dataFile) {
      if (rtcOK) dataFile.print(now.timestamp());
      else dataFile.print("No RTC Time");
      dataFile.print(",");
      dataFile.print(suhu1); dataFile.print(",");
      dataFile.print(humd1); dataFile.print(",");
      dataFile.print(suhu2); dataFile.print(",");
      dataFile.println(humd2);
      dataFile.close();
      Serial.println("Data tersimpan ke SD.");
    } else {
      tampilkanErrorOLED("Error: SD Write");
      Serial.println("Gagal menulis ke SD!");
    }
  }

  // OLED tampil data
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  String baris1, baris2, baris3;
  if (rtcOK) {
    char buffer[25];
    sprintf(buffer, "%02d/%02d/%04d %02d:%02d",
            now.day(), now.month(), now.year(),
            now.hour(), now.minute());
    baris1 = String(buffer);
  } else baris1 = "RTC ERROR";

  baris2 = "SHT31: " + String(suhu1, 2) + "C " + String(humd1, 2) + "%";
  baris3 = "HTU21: " + String(suhu2, 2) + "C " + String(humd2, 2) + "%";

  int totalBaris = 3;
  int tinggiTeks = 8;
  int totalTinggi = totalBaris * tinggiTeks + (totalBaris - 1) * 4;
  int startY = (OLED_HEIGHT - totalTinggi) / 2;

  auto tulisTengah = [&](String teks, int y) {
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(teks, 0, 0, &x1, &y1, &w, &h);
    int16_t x = (OLED_WIDTH - w) / 2;
    display.setCursor(x, y);
    display.println(teks);
  };

  tulisTengah(baris1, startY);
  tulisTengah(baris2, startY + tinggiTeks + 4);
  tulisTengah(baris3, startY + 2 * (tinggiTeks + 4));
  display.display();

  delay(5000);
}
