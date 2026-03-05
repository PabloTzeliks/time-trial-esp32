#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

const char* ssid = "Wokwi-GUEST";
const char* password = "";

const char* mqtt_server = "12f590a554fc4cfda1e6e2edd8381c84.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* topic = "senai/timetrial/corrida/sensor";
const char* mqtt_user = "admin-time-trial";
const char* mqtt_pass = "BrunoBut123";

const char* rfidPool[] = {
  "EA 15 22 11",
  "A1 B2 C3 D4",
  "E5 F6 G7 H8",
  "BB 22 CC 33",
  "DD 44 EE 55"
};

WiFiClientSecure espClient;
PubSubClient client(espClient);

// Agora temos um Array para controlar os 5 carros INDEPENDENTEMENTE
int estadoVolta[5] = {0, 0, 0, 0, 0}; 
unsigned long tempoInicioVolta[5] = {0, 0, 0, 0, 0};
unsigned long tempoAleatorioDaVolta[5] = {0, 0, 0, 0, 0};

void setup_wifi() {
  Serial.print("Conectando WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.println("\nWiFi OK!");
  espClient.setInsecure();
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    String clientId = "ESP32Wokwi-" + String(random(0, 0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println(" OK!");
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  randomSeed(analogRead(0));
  setup_wifi();
  client.setServer(mqtt_server, mqtt_port);
}

void loop() {
  if (!client.connected()) reconnect();
  client.loop();

  unsigned long timestampAtual = millis();

  // Vamos rodar a lógica para CADA UM dos 5 carros a cada loop
  for (int i = 0; i < 5; i++) {
    
    // ESTADO 0: AGUARDANDO LARGADA
    if (estadoVolta[i] == 0) {
      // Dá uma chance de apenas 2% do carro largar neste exato milissegundo.
      // Isso evita que os 5 larguem no exato mesmo centésimo de segundo.
      if (random(0, 100) < 2) {
        
        // Sorteia o tempo de volta (ex: de 5 a 15 segundos para ser mais rápido nos testes)
        tempoAleatorioDaVolta[i] = random(5000, 15001); 
        tempoInicioVolta[i] = timestampAtual;
        
        String payload = "{\"rfid\":\"" + String(rfidPool[i]) + "\", \"timestampMs\":" + String(timestampAtual) + "}";
        
        Serial.print("🏁 Largada (Carro " + String(i+1) + "): ");
        Serial.println(payload);
        client.publish(topic, payload.c_str());
        
        estadoVolta[i] = 1; 
      }
    }
    
    // ESTADO 1: CORRENDO (AGUARDANDO CHEGADA)
    else if (estadoVolta[i] == 1) {
      if (timestampAtual - tempoInicioVolta[i] >= tempoAleatorioDaVolta[i]) {
        
        String payload = "{\"rfid\":\"" + String(rfidPool[i]) + "\", \"timestampMs\":" + String(timestampAtual) + "}";
        
        Serial.print("✅ Chegada (Carro " + String(i+1) + "): ");
        Serial.println(payload);
        client.publish(topic, payload.c_str());
        
        tempoInicioVolta[i] = timestampAtual; 
        estadoVolta[i] = 2; // Vai para o cooldown
      }
    }

    // ESTADO 2: COOLDOWN (Pitstop rápido antes de correr de novo)
    else if (estadoVolta[i] == 2) {
      // 2 segundos de pausa para o carro respirar antes de dar outra volta
      if (timestampAtual - tempoInicioVolta[i] >= 2000) {
          estadoVolta[i] = 0; 
      }
    }
  }
  
  // Um delayzinho minúsculo só pra não fritar a CPU do simulador Wokwi atoa
  delay(10);
}