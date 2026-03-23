#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <PZEM004Tv30.h> 
#include <time.h> 
#include "secrets.h"

// --- THINGSPEAK CONFIG ---
String thingSpeakApiKey = TS_WRITE_API_KEY; 
const char* tsServer = "api.thingspeak.com";

// --- HARDWARE SETTINGS ---
const int RELAY_PIN = 26;
#define RELAY_ON LOW   
#define RELAY_OFF HIGH 

// --- SENSOR PINS (RX/TX) ---
#define PZEM_RX_PIN 16 
#define PZEM_TX_PIN 17 
#define PZEM_SERIAL Serial2 

// ==========================================
// 2. OBJECTS & TIMERS
// ========================================== 
WiFiClientSecure client;
UniversalTelegramBot bot(BOT_TOKEN, client);
PZEM004Tv30 pzem(PZEM_SERIAL, PZEM_RX_PIN, PZEM_TX_PIN);

unsigned long lastTimeBotRan = 0;
int botRequestDelay = 500; // Check Telegram every 0.5 seconds

unsigned long lastCloudUpdate = 0;
const long cloudUpdateInterval = 20000; 

bool relayState = false;

// ==========================================
// 3. HELPER FUNCTIONS
// ==========================================

float calculateTNBill(float units) {
  float bill = 0.0;
  if (units <= 100.0) return 0.0;
  if (units <= 500.0) {
      if (units <= 200.0) bill = (units - 100.0) * 2.35;
      else if (units <= 400.0) bill = (100.0 * 2.35) + ((units - 200.0) * 4.70);
      else bill = (100.0 * 2.35) + (200.0 * 4.70) + ((units - 400.0) * 6.30);
  } else {
      if (units <= 600.0) bill = (300.0 * 4.70) + (100.0 * 6.30) + ((units - 500.0) * 8.40);
      else if (units <= 800.0) bill = (300.0 * 4.70) + (100.0 * 6.30) + (100.0 * 8.40) + ((units - 600.0) * 9.45);
      else if (units <= 1000.0) bill = (300.0 * 4.70) + (100.0 * 6.30) + (100.0 * 8.40) + (200.0 * 9.45) + ((units - 800.0) * 10.50);
      else bill = (300.0 * 4.70) + (100.0 * 6.30) + (100.0 * 8.40) + (200.0 * 9.45) + (200.0 * 10.50) + ((units - 1000.0) * 11.55);
  }
  return bill;
}

// "Fire and Forget" Cloud Push
void sendToCloud(float v, float c, float p, float e, float pf) {
  WiFiClient tsClient; 
  if (tsClient.connect(tsServer, 80)) {
    String url = "/update?api_key=" + thingSpeakApiKey +
                 "&field1=" + String(v) +
                 "&field2=" + String(c) +
                 "&field3=" + String(p) +
                 "&field4=" + String(e) +
                 "&field5=" + String(pf);
                 
    tsClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + String(tsServer) + "\r\n" +
                   "Connection: close\r\n\r\n");
                   
    delay(10); 
    tsClient.stop(); 
    Serial.println("☁️ Data fired to ThingSpeak!");
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;

    if (chat_id != CHAT_ID) continue; 

    if (text == "/start") {
      String welcome = "👋 **Smart Energy Monitor**\n";
      welcome += "/on - Load ON\n";
      welcome += "/off - Load OFF\n";
      welcome += "/status - Real Sensor Data & Bill\n";
      bot.sendMessage(chat_id, welcome, "");
    }

    if (text == "/on") {
      digitalWrite(RELAY_PIN, RELAY_ON);
      relayState = true;
      bot.sendMessage(chat_id, "✅ Load Turned ON", "");
    }

    if (text == "/off") {
      digitalWrite(RELAY_PIN, RELAY_OFF);
      relayState = false;
      bot.sendMessage(chat_id, "⚠️ Load Turned OFF", "");
    }

    if (text == "/status") {
      float voltage = pzem.voltage();
      float current = pzem.current();
      float power = pzem.power();
      float energy = pzem.energy();
      float frequency = pzem.frequency();
      float pf = pzem.pf(); 

      if(isnan(voltage)) {
        voltage = 0.0; current = 0.0; power = 0.0; energy = 0.0; pf = 0.0;
        bot.sendMessage(chat_id, "⚠️ Sensor Error! Check Wiring.", "");
      }

      float predictedCost = calculateTNBill(energy);

      String msg = "📊 **REAL Live Readings**\n";
      msg += "Voltage: " + String(voltage, 1) + " V\n";
      msg += "Current: " + String(current, 3) + " A\n";
      msg += "Power: " + String(power, 1) + " W\n";
      msg += "Power Factor: " + String(pf, 2) + "\n"; 
      msg += "Energy: " + String(energy, 3) + " kWh\n";
      msg += "Freq: " + String(frequency, 1) + " Hz\n";
      msg += "💰 **Est. Bill (TN): ₹" + String(predictedCost, 2) + "**\n"; 
      
      if(relayState) msg += "\nLoad is: 🟢 ON";
      else msg += "\nLoad is: 🔴 OFF";
      
      bot.sendMessage(chat_id, msg, "");
    }
  }
}

// ==========================================
// 4. SETUP
// ==========================================
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); 

  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, SECRET_PASS);
  client.setInsecure(); // Required for Telegram

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected!");

  // ---> NEW: Sync the ESP32 clock with the Internet <---
  Serial.print("Syncing internal clock with NTP...");
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    Serial.print(".");
    delay(500);
    now = time(nullptr);
  }
  Serial.println("\nTime synced successfully! Telegram will now be INSTANT.");
  // -----------------------------------------------------

  bot.sendMessage(CHAT_ID, "🚀 System Online! (Instant Mode)", "");
}

// ==========================================
// 5. LOOP
// ==========================================
void loop() {
  // 1. Handle Telegram Messages 
  if (millis() - lastTimeBotRan > botRequestDelay) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  // 2. Handle Cloud Database Sync (Every 20 seconds)
  if (millis() - lastCloudUpdate > cloudUpdateInterval) {
    float v = pzem.voltage();
    float c = pzem.current();
    float p = pzem.power();
    float e = pzem.energy();
    float pf = pzem.pf();

    if(!isnan(v)) {
      sendToCloud(v, c, p, e, pf);
    }
    lastCloudUpdate = millis();
  }
}