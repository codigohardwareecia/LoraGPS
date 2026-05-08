#### Lora 32 com GPS
 
 Módulo Lora 32
https://www.mercadolivre.com.br/placa-wifi-lora-32--esp32-lora-display-oled/up/MLBU1483033688
Módulo GPS
https://www.mercadolivre.com.br/modulo-gps-ublox-gygps6mv2-gyneo6mv2--arduino-drone/up/MLBU3230028787?pdp_filters=item_id:MLB904367400

Preparação do Ambiente Para que o código reconheça sua placa corretamente, você precisa instalar o "Core" da Heltec:

Abra a Arduino IDE.

Vá em File -> Preferences.

No campo "Additional Boards Manager URLs", cole este link: [https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.9/package_heltec_esp32_index.json](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/releases/download/0.0.9/package_heltec_esp32_index.json)

Vá em Tools -> Board -> Boards Manager, procure por Heltec ESP32 e instale.

Selecione sua placa em Tools -> Board: provavelmente será a WiFi LoRa 32 (V2) .

Faça um teste de comunicação e se ele enviar e gravar o firmware vazio está tudo certo.

Vá em Sketch -> Include Library -> Manage Libraries vamos instalar várias bibliotecas

Instalação da Biblioteca de Rádio: A biblioteca oficial da Heltec é funcional, mas a comunidade prefere a RadioLib ou a LoRa (by Sandeep Mistry) por serem mais limpas para depurar.

Instalação da biblioteva do Lora: procure por LoRa e instale a versão do Sandeep Mistry

Biblioteca da Heltec: agora instale a biblioeca Heltec ESP32 Dev-Boards by Heltec

Suporte ao GPS: agora precisamos adicionar as biblotecas extras instale TinyGPSPlus (por Mikal art).

Suporte ao display: Agora instale a biblioteca "SSD1306Wire"

Suporte ao display: Agora instale a biblioteca ESP32 e ESP8266 OLED driver for SSD1306

Instale o ArduinoJson

Conexão dos pinos:

| **NEO-6M GPS** | **Heltec LoRa 32** | **Nota**                                                  |
| -------------- | ------------------ | --------------------------------------------------------- |
| **VCC**        | 3V3 ou 5V          | Verifique se sua placa GPS tem regulador (a maioria tem). |
| **GND**        | GND                | Terra comum.                                              |
| **TX**         | GPIO 12 (Sugerido) | O TX do GPS envia dados para o RX do ESP32.               |
| **RX**         | GPIO 13 (Sugerido) | Opcional (para configurar o GPS).                         |

Código do Transmissor
```C++
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
    while (1);
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
```

Código do Receptro
```C++
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
      SerialBT.println(jsonString); 
      SerialBT.flush();

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
  }
}
```

para o C# temos que criar um projeto .NET 8 do tipo Windows Forms

Instalar no Nuget GMap.NET.WindowsForms. Gmap.Net.WinForms, System.Data.SQLite, System.IO.Ports e SQLitePCLRaw.bundle_e_sqlite3

```C#
using GMap.NET;
using GMap.NET.MapProviders;
using GMap.NET.WindowsForms;
using GMap.NET.WindowsForms.Markers;
using System.Globalization;
using System.IO.Ports;
using System.Text.Json;

namespace LoraGPSMonitor
{
    public partial class Form1 : Form
    {
        private GMapOverlay? markersOverlay;
        private SerialPort? btPort;
        private readonly string cachePath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "MapCache");
        private GMarkerGoogle? realTimeMarker;

        public Form1()
        {
            InitializeComponent();
            this.Load += (s, e) => SetupMap(); // Garante que o mapa carrega após o form estar pronto
            this.FormClosing += (s, e) => btPort?.Close(); // Fecha a porta ao sair
        }

        private void SetupMap()
        {
            GMap.NET.MapProviders.GMapProvider.UserAgent = "seuemail@hotmail.com";
            System.Net.ServicePointManager.SecurityProtocol = System.Net.SecurityProtocolType.Tls12;
            gmap.Dock = DockStyle.Fill;

            // Configuração de Cache
            if (!Directory.Exists(cachePath)) Directory.CreateDirectory(cachePath);
            gmap.CacheLocation = cachePath;
            gmap.Manager.Mode = AccessMode.ServerAndCache;

            // Provedor e Posicionamento
            gmap.MapProvider = GMapProviders.OpenStreetMap;
            gmap.Position = new PointLatLng(-23.5489, -46.6388); // Ex: São Paulo
            gmap.MinZoom = 2;
            gmap.MaxZoom = 20;
            gmap.Zoom = 15;
            gmap.DragButton = MouseButtons.Left;

            markersOverlay = new GMapOverlay("rastreio");
            gmap.Overlays.Add(markersOverlay);
            this.Controls.Add(gmap);

            IniciarBluetooth();
        }

        private void IniciarBluetooth()
        {
            btPort = new SerialPort("COM16", 9600)
            {
                DtrEnable = true,
                RtsEnable = true,
                ReadTimeout = 2000
            };

            try
            {
                btPort.Open();
                Task.Run(LerDadosLoop); // Loop de leitura ativa (Polling)
                MessageBox.Show("Jarvis Conectado!");
            }
            catch (Exception ex) { MessageBox.Show(ex.Message); }
        }

        private void LerDadosLoop()
        {
            while (btPort != null && btPort.IsOpen)
            {
                try
                {
                    string json = btPort.ReadLine().Trim();
                    var dados = JsonSerializer.Deserialize<Telemetria>(json,
                                new JsonSerializerOptions { PropertyNameCaseInsensitive = true });

                    if (dados != null)
                    {
                        this.BeginInvoke(() => {
                            AtualizarMapa(dados);
                        });
                    }
                }
                catch { }
                Thread.Sleep(100);
            }
        }



        private void AtualizarMapa(Telemetria d)
        {
            PointLatLng novoPonto = new PointLatLng(d.Lat, d.Lng);

            // 2. Verifica se o marcador já existe
            if (realTimeMarker == null)
            {
                // Se não existe, cria pela primeira vez
                realTimeMarker = new GMarkerGoogle(novoPonto, GMarkerGoogleType.red_dot);
                realTimeMarker.ToolTipMode = MarkerTooltipMode.Always; // MANTÉM SEMPRE VISÍVEL
                markersOverlay.Markers.Add(realTimeMarker);
            }
            else
            {
                // 3. Se já existe, apenas move e atualiza o texto (O balão não fecha!)
                realTimeMarker.Position = novoPonto;
            }

            // Atualiza o conteúdo do balão
            realTimeMarker.ToolTipText = $"ID: {d.Id}\nSats: {d.Sat}\nLat: {d.Lat:F6}\nLng: {d.Lng:F6}";

            // Centraliza o mapa no novo ponto
            gmap.Position = novoPonto;
        }
    }

    public class Telemetria
    {
        public string Id { get; set; } = "";
        public double Lat { get; set; }
        public double Lng { get; set; }
        public int Sat { get; set; }
        public double Alt { get; set; }
    }
}

```
