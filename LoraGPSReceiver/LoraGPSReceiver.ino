#include <SPI.h>
#include <LoRa.h>
#include "SSD1306Wire.h"
#include <ArduinoJson.h>
#include "BluetoothSerial.h"

// --- PINAGEM FIXA HELTEC V2 ---
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26
#define OLED_RST 16

BluetoothSerial SerialBT;
SSD1306Wire display(0x3c, 4, 15);

// Documento JSON: 300 bytes é suficiente para o seu pacote
StaticJsonDocument<300> doc;

void atualizarDisplay(String t1, String t2) {
  display.clear();
  display.drawString(0, 0, t1);
  display.drawString(0, 16, t2);
  display.display();
}

void setup() {
  Serial.begin(115200);

  // Inicializa Bluetooth
  SerialBT.begin("LoRa_GPS_Receptor");

  // Reset OLED
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW); delay(50); digitalWrite(OLED_RST, HIGH);
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  atualizarDisplay("RECEPTOR JARVIS", "Iniciando...");

  // Inicializa LoRa
  SPI.begin(SCK, MISO, MOSI, SS);
  LoRa.setPins(SS, RST, DIO0);
  
  if (!LoRa.begin(915E6)) {
    atualizarDisplay("ERRO", "Falha no LoRa!");
    while (1);
  }
  
  LoRa.setSyncWord(0xF3);
  atualizarDisplay("AGUARDANDO...", "Bluetooth: Ativo");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) {
    // Lê o pacote LoRa
    String jsonString = LoRa.readString();

    // LIMPA o documento antes de usar novamente (IMPORTANTE por ser global)
    doc.clear();

    // Deserializa o JSON
    DeserializationError error = deserializeJson(doc, jsonString);

    if (!error) {
      // Extrai os dados (Opcional se você só for repassar, mas bom para o OLED)
      String id = doc["id"] | "S/N";
      float lat = doc["lat"];
      float lng = doc["lng"];
      int sat = doc["sat"];
      double alt = doc["alt"];

      // 1. Envia para o Bluetooth (CORRIGIDO: println para o C# reconhecer o fim da linha)
      if (SerialBT.hasClient()) { // SÓ ENVIA SE HOUVER ALGUÉM CONECTADO
            SerialBT.println(jsonString);
            SerialBT.flush();
        } else {
            // Opcional: Avisar no OLED que o Bluetooth desconectou
            display.drawString(0, 50, "BT Desconectado!");
            display.display();
        }

      // 2. Debug Serial USB
      Serial.println("RX OK: " + jsonString);

      // 3. Atualiza o display com dados tratados
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "ID: " + id + " | Sats: " + String(sat));
      display.setFont(ArialMT_Plain_16);
      display.drawString(0, 20, String(lat, 6));
      display.drawString(0, 40, String(lng, 6));
      display.display();

    } else {
      // Caso o JSON venha corrompido pelo rádio
      display.clear();
      display.setFont(ArialMT_Plain_10);
      display.drawString(0, 0, "JSON CORROMPIDO:");
      display.drawString(0, 16, jsonString);
      display.drawString(0, 45, "Erro: " + String(error.c_str()));
      display.display();  
      
      Serial.println("Erro no JSON: " + String(error.c_str()));
    }

    delay(100);
  }
}