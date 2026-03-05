#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

// ---------- PINOS ----------
#define SS_PIN 5
#define RST_PIN 22
#define IR_PIN 4
#define LED_PIN 21

// ---------- WIFI ----------
const char* ssid = "pablo";
const char* password = "chipangolia";

// ---------- MQTT ----------
const char* mqtt_server = "12f590a554fc4cfda1e6e2edd8381c84.s1.eu.hivemq.cloud";
const int mqtt_port = 8883;
const char* topic = "senai/timetrial/corrida/sensor";
const char* mqtt_user = "admin-time-trial";
const char* mqtt_pass = "BrunoBut123";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// ---------- RFID ----------
MFRC522 mfrc522(SS_PIN, RST_PIN);

// ---------- CONTROLE ----------
bool aguardandoRFID = false;
unsigned long timestampIR = 0;

// ---------- WIFI ----------
void conectarWiFi() {

  Serial.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");

  espClient.setInsecure();
}

// ---------- MQTT ----------
void conectarMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "ESP32-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("conectado");
    } 
    else {
      Serial.print("erro ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

// ---------- SETUP ----------
void setup() {

  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  SPI.begin();
  mfrc522.PCD_Init();

  conectarWiFi();

  client.setServer(mqtt_server, mqtt_port);

  Serial.println("Sistema iniciado");
}

// ---------- LOOP ----------
void loop() {

  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();

  // ---------- SENSOR IR ----------
  if (!aguardandoRFID && digitalRead(IR_PIN) == LOW) {

    timestampIR = millis();
    aguardandoRFID = true;

    Serial.print("IR detectado - timestamp: ");
    Serial.println(timestampIR);

    delay(200); // debounce
  }

  // ---------- RFID ----------
  if (aguardandoRFID) {

    if (!mfrc522.PICC_IsNewCardPresent())
      return;

    if (!mfrc522.PICC_ReadCardSerial())
      return;

    String rfid = "";

    for (byte i = 0; i < mfrc522.uid.size; i++) {

      if (mfrc522.uid.uidByte[i] < 0x10)
        rfid += "0";

      rfid += String(mfrc522.uid.uidByte[i], HEX);

      if (i < mfrc522.uid.size - 1)
        rfid += " ";
    }

    rfid.toUpperCase();

    Serial.print("RFID detectado: ");
    Serial.println(rfid);

    // LED
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);

    // ---------- JSON ----------
    String payload =
      "{\"rfid\":\"" + rfid +
      "\",\"timestampMs\":" + String(timestampIR) + "}";

    Serial.print("Enviando MQTT: ");
    Serial.println(payload);

    client.publish(topic, payload.c_str());

    aguardandoRFID = false;

    mfrc522.PICC_HaltA();
  }
}