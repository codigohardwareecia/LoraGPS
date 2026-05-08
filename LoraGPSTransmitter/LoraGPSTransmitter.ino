#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <TinyGPSPlus.h>
#include <ArduinoJson.h>
#include "SSD1306Wire.h"

// --- PINAGEM FIXA HELTEC V2 ---
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define OLED_RST 16

// --- PINOS GPS (Conecte o TX do GPS no 12 e RX no 13) ---
#define GPS_RX 12
#define GPS_TX 13

TinyGPSPlus gps;
HardwareSerial ss(2); // Usa a Serial 2 do ESP32
SSD1306Wire display(0x3c, 4, 15);
StaticJsonDocument<300> doc;

void atualizarDisplay(String t1, String t2, String t3 = "") {
  display.clear();
  display.drawString(0, 0, t1);
  display.drawString(0, 16, t2);
  display.drawString(0, 32, t3);
  display.display();
}

void setup() {
  Serial.begin(115200);
  ss.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX);

  // Inicializa Display (Exatamente como no seu código Jarvis)
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(50); digitalWrite(OLED_RST, HIGH);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  atualizarDisplay("SISTEMA GPS", "Iniciando LoRa...", "");

  // Inicializa LoRa Manualmente (Evita erro 'LoRa not declared')
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(915E6)) {
    atualizarDisplay("ERRO", "LoRa falhou!", "Verifique hardware");
    while (1000);
  }
  
  LoRa.setSyncWord(0xF3); // Deve ser igual ao receptor
  atualizarDisplay("ONLINE", "Aguardando GPS...", "Va para ceu aberto");
}

void loop() {
  // Alimenta o objeto GPS com dados da Serial2
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  static unsigned long lastTime = 0;
  if (millis() - lastTime > 5000) {
    if (gps.location.isValid()) {

      doc["id"] = "NAUTILUS";
      doc["lat"] = gps.location.lat();
      doc["lng"] = gps.location.lng();
      doc["sat"] = gps.satellites.value();
      doc["alt"] = gps.altitude.meters();
      
      String payload;
      serializeJson(doc, payload);

      LoRa.beginPacket();
      LoRa.print(payload);
      LoRa.endPacket();

      atualizarDisplay("ENVIADO!", "Lat: " + String(gps.location.lat(), 4), "Lon: " + String(gps.location.lng(), 4));
      Serial.println("Transmitido: " + payload);
    } else {
      // Se não houver fix, mostra quantos satélites está vendo
      atualizarDisplay("SEM SINAL", "Satelites: " + String(gps.satellites.value()), "Buscando fix...");
    }
    lastTime = millis();
  }
}