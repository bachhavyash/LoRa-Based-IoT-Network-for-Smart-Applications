#define BLYNK_TEMPLATE_ID   "TMPL3qZ9danDa"
#define BLYNK_TEMPLATE_NAME "LoRa Gateway"
#define BLYNK_AUTH_TOKEN    "1xCBRxpMTmohaJ2CJkd7bHYYSe2AGi4m"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>

// ── WiFi ──────────────────────────────────────────────────────
const char* ssid     = "MyWiFi_2.4G";
const char* password = "8010225376";

// ── LoRa UART ─────────────────────────────────────────────────
HardwareSerial loraSerial(2); // RX=GPIO16, TX=GPIO17
String rxBuffer = "";

// ── LCD ───────────────────────────────────────────────────────
LiquidCrystal_PCF8574 lcd(0x27); // try 0x3F if screen blank

// ── Buzzer ────────────────────────────────────────────────────
#define BUZZER_PIN 25
unsigned long buzzerOnTime  = 0;
unsigned long buzzerDuration = 0;
bool buzzerActive = false;

// ── Blynk ─────────────────────────────────────────────────────
BlynkTimer timer;
unsigned long packetCount = 0;

// ── Sensor data struct ────────────────────────────────────────
struct SensorData {
  String nodeID;
  String type;
  float  value;
  String status;
  int    rssi;
  int    snr;
  bool   valid;
};

// ─────────────────────────────────────────────────────────────
// Parse LoRa packet
// +RCV=<addr>,<len>,<payload>,<rssi>,<snr>
// payload = NODE01|WATER|85.00|HIGH
// ─────────────────────────────────────────────────────────────
SensorData parsePacket(String raw) {
  SensorData d;
  d.valid = false;
  d.rssi  = 0;
  d.snr   = 0;

  int rcvIdx = raw.indexOf("+RCV=");
  if (rcvIdx < 0) return d;

  int c1 = raw.indexOf(",", rcvIdx + 5);
  int c2 = raw.indexOf(",", c1 + 1);
  int c3 = raw.indexOf(",", c2 + 1);
  int c4 = raw.indexOf(",", c3 + 1);
  if (c1 < 0 || c2 < 0 || c3 < 0 || c4 < 0) {
    Serial.println("[WARN] Bad +RCV format: " + raw);
    return d;
  }

  String payload = raw.substring(c2 + 1, c3);
  String rssiStr = raw.substring(c3 + 1, c4);
  String snrStr  = raw.substring(c4 + 1);
  rssiStr.trim();
  snrStr.trim();
  d.rssi = rssiStr.toInt();
  d.snr  = snrStr.toInt();

  int p1 = payload.indexOf("|");
  int p2 = payload.indexOf("|", p1 + 1);
  int p3 = payload.indexOf("|", p2 + 1);
  if (p1 < 0 || p2 < 0 || p3 < 0) {
    Serial.println("[WARN] Bad payload: " + payload);
    return d;
  }

  d.nodeID = payload.substring(0, p1);
  d.type   = payload.substring(p1 + 1, p2);
  d.value  = payload.substring(p2 + 1, p3).toFloat();
  d.status = payload.substring(p3 + 1);
  d.status.trim();
  d.valid  = true;
  return d;
}

// ─────────────────────────────────────────────────────────────
// LCD update
// Line 1: type + value   e.g.  WATER: 85%
// Line 2: status         e.g.  Status: NORMAL
// ─────────────────────────────────────────────────────────────
void updateLCD(SensorData d) {
  lcd.clear();
  lcd.setCursor(0, 0);

  if (d.type == "WATER") {
    lcd.print("Water: " + String((int)d.value) + "%");
  } else if (d.type == "GAS") {
    lcd.print("Gas: " + String((int)d.value));
  } else {
    lcd.print(d.type + ": " + String((int)d.value));
  }

  lcd.setCursor(0, 1);
  lcd.print("Status: " + d.status);
}

// ─────────────────────────────────────────────────────────────
// Non-blocking buzzer start
// ─────────────────────────────────────────────────────────────
void startBuzzer(unsigned long durationMs) {
  digitalWrite(BUZZER_PIN, HIGH);
  buzzerOnTime   = millis();
  buzzerDuration = durationMs;
  buzzerActive   = true;
}

// Call this every loop() — turns off buzzer after duration
void handleBuzzerTick() {
  if (buzzerActive && millis() - buzzerOnTime >= buzzerDuration) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerActive = false;
  }
}

// ─────────────────────────────────────────────────────────────
// Trigger buzzer based on alert type
// ─────────────────────────────────────────────────────────────
void triggerAlert(SensorData d) {
  if (d.type == "WATER" && d.status == "HIGH") {
    startBuzzer(400);  // short beep for water
    Serial.println("[ALERT] HIGH WATER LEVEL on " + d.nodeID);
  }
  if (d.type == "GAS" && d.status == "LEAK") {
    startBuzzer(800);  // long beep for gas leak
    Serial.println("[ALERT] GAS LEAK DETECTED on " + d.nodeID);
  }
}

// ─────────────────────────────────────────────────────────────
// Send all data to Blynk
// ─────────────────────────────────────────────────────────────
void sendToBlynk(SensorData d) {
  if (!Blynk.connected()) {
    Serial.println("[WARN] Blynk not connected — skipping");
    return;
  }

  if (d.type == "WATER") {
    Blynk.virtualWrite(V0, (int)d.value);
    Blynk.virtualWrite(V1, d.status);
    if (d.status == "HIGH")
      Blynk.logEvent("water_high",
        "HIGH water on " + d.nodeID + ": " + String((int)d.value) + "%");
    if (d.status == "ERROR")
      Blynk.logEvent("water_sensor_error", "Sensor fault on " + d.nodeID);
  }

  if (d.type == "GAS") {
    Blynk.virtualWrite(V2, (int)d.value);
    Blynk.virtualWrite(V3, d.status);
    if (d.status == "LEAK")
      Blynk.logEvent("gas_leak",
        "GAS LEAK on " + d.nodeID + "! ADC=" + String((int)d.value));
  }

  Blynk.virtualWrite(V4, d.rssi);
  Blynk.virtualWrite(V5, d.snr);
  Blynk.virtualWrite(V6, (int)++packetCount);

  Serial.printf("[BLYNK] V0/V2=%.0f V4(RSSI)=%d V5(SNR)=%d V6=%lu\n",
                d.value, d.rssi, d.snr, packetCount);
}

// ─────────────────────────────────────────────────────────────
// Reconnect check — runs every 10 seconds
// ─────────────────────────────────────────────────────────────
void checkConnection() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WARN] WiFi lost — reconnecting...");
    WiFi.reconnect();
  }
  if (!Blynk.connected()) {
    Serial.println("[WARN] Blynk lost — reconnecting...");
    Blynk.connect(5000);
  }
}

// ─────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  loraSerial.begin(115200, SERIAL_8N1, 16, 17);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // I2C pins for ESP32 — must be before lcd.begin()
  Wire.begin(21, 22);  // SDA=GPIO21, SCL=GPIO22

  // LCD init
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("Starting...");
  delay(1000);

  // WiFi + Blynk
  Serial.print("[INFO] Connecting WiFi...");
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  Serial.println(" OK");
  Serial.println("[INFO] Blynk connected: " + String(Blynk.connected()));

  // LoRa init
  loraSerial.print("AT\r\n");                       delay(500);
  loraSerial.print("AT+ADDRESS=1\r\n");             delay(500);
  loraSerial.print("AT+NETWORKID=6\r\n");           delay(500);
  loraSerial.print("AT+BAND=865000000\r\n");        delay(500);
  loraSerial.print("AT+PARAMETER=9,7,1,12\r\n");    delay(500);

  Serial.println("[INFO] Gateway Node Ready");
  timer.setInterval(10000L, checkConnection);

  // Ready message on LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Gateway Ready");
  lcd.setCursor(0, 1);
  lcd.print("Waiting LoRa...");
}

// ─────────────────────────────────────────────────────────────
// Loop
// ─────────────────────────────────────────────────────────────
void loop() {
  Blynk.run();
  timer.run();
  handleBuzzerTick();  // non-blocking buzzer check

  while (loraSerial.available()) {
    char c = loraSerial.read();
    rxBuffer += c;

    if (c == '\n') {
      rxBuffer.trim();
      if (rxBuffer.indexOf("+RCV=") >= 0) {
        SensorData d = parsePacket(rxBuffer);
        if (d.valid) {
          Serial.printf("[RX] Node=%s Type=%s Val=%.2f Status=%s RSSI=%d SNR=%d\n",
            d.nodeID.c_str(), d.type.c_str(), d.value,
            d.status.c_str(), d.rssi, d.snr);
          updateLCD(d);
          triggerAlert(d);
          sendToBlynk(d);
        } else {
          Serial.println("[WARN] Malformed: " + rxBuffer);
        }
      }
      rxBuffer = "";
    }
  }
}