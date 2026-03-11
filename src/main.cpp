#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char *ssid = "Wokwi-GUEST";
const char *password = "";

const char *mqtt_server = "12f590a554fc4cfda1e6e2edd8381c84.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char *topic = "senai/timetrial/corrida/sensor";
const char *mqtt_user = "admin-time-trial";
const char *mqtt_pass = "BrunoBut123";

const int NUM_CARROS = 20;

// Lote 1: 20 IDs únicos
const char *rfidPool[NUM_CARROS] = {
    "10 11 12 13", "20 21 22 23", "30 31 32 33", "40 41 42 43",
    "50 51 52 53", "60 61 62 63", "70 71 72 73", "80 81 82 83",
    "90 91 92 93", "A0 A1 A2 A3", "B0 B1 B2 B3", "C0 C1 C2 C3",
    "D0 D1 D2 D3", "E0 E1 E2 E3", "F0 F1 F2 F3", "11 22 33 44",
    "55 66 77 88", "99 AA BB CC", "DD EE FF 00", "AA 11 BB 22"};

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Inicializa todos os 20 espaços com zero automaticamente
int estadoVolta[NUM_CARROS] = {0};
unsigned long tempoInicioVolta[NUM_CARROS] = {0};
unsigned long tempoAleatorioDaVolta[NUM_CARROS] = {0};

void setup_wifi()
{
  Serial.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi OK!");
  espClient.setInsecure();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Conectando MQTT...");
    String clientId = "ESP32Wokwi-Lote1-" + String(random(0, 0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass))
    {
      Serial.println(" OK!");
    }
    else
    {
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  randomSeed(analogRead(0));
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop()
{
  if (!client.connected())
    reconnect();
  client.loop();

  unsigned long timestampAtual = millis();

  for (int i = 0; i < NUM_CARROS; i++)
  {

    // ESTADO 0: AGUARDANDO LARGADA
    if (estadoVolta[i] == 0)
    {
      if (random(0, 100) < 2)
      {
        tempoAleatorioDaVolta[i] = random(5000, 15001);
        tempoInicioVolta[i] = timestampAtual;

        String payload = "{\"rfid\":\"" + String(rfidPool[i]) + "\", \"timestampMs\":" + String(timestampAtual) + "}";
        Serial.print("🏁 Largada (Carro " + String(i + 1) + "): ");
        Serial.println(payload);
        client.publish(topic, payload.c_str());

        estadoVolta[i] = 1;
      }
    }

    // ESTADO 1: CORRENDO (AGUARDANDO CHEGADA)
    else if (estadoVolta[i] == 1)
    {
      if (timestampAtual - tempoInicioVolta[i] >= tempoAleatorioDaVolta[i])
      {
        String payload = "{\"rfid\":\"" + String(rfidPool[i]) + "\", \"timestampMs\":" + String(timestampAtual) + "}";
        Serial.print("✅ Chegada (Carro " + String(i + 1) + "): ");
        Serial.println(payload);
        client.publish(topic, payload.c_str());

        tempoInicioVolta[i] = timestampAtual;
        estadoVolta[i] = 2; // Vai para o cooldown
      }
    }

    // ESTADO 2: COOLDOWN (Pitstop)
    else if (estadoVolta[i] == 2)
    {
      if (timestampAtual - tempoInicioVolta[i] >= 2000)
      {
        estadoVolta[i] = 0;
      }
    }
  }

  delay(10);
}