#include <HardwareSerial.h>

HardwareSerial loraSerial(2); // UART2: RX=GPIO16, TX=GPIO17

// ── Node & Pin Config ─────────────────────────────────────────
#define NODE_ID          "NODE01"
#define TRIG_PIN         5
#define ECHO_PIN         18
#define GAS_PIN          34

// ── Thresholds ────────────────────────────────────────────────
#define WATER_THRESHOLD  80
#define GAS_THRESHOLD    500
#define MAX_DISTANCE_CM  100.0
#define PULSE_TIMEOUT_US 30000

// ── LoRa Config ───────────────────────────────────────────────
String receiverID         = "1";
int    retryCount         = 3;
unsigned long lastSendTime  = 0;
unsigned long sendInterval  = 5000;

// ─────────────────────────────────────────────────────────────
float readWaterLevel() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, PULSE_TIMEOUT_US);
  if (duration == 0) {
    Serial.println("[WARN] Ultrasonic: no echo");
    return -1.0;
  }
  float dist = duration * 0.034 / 2.0;
  return constrain(MAX_DISTANCE_CM - dist, 0.0, 100.0);
}

int readGasSensor() {
  return analogRead(GAS_PIN);
}

String createPacket(String type, float value, String status) {
  return String(NODE_ID) + "|" + type + "|" + String(value, 2) + "|" + status;
}

bool sendLoRa(String data) {
  if (data.length() > 250) return false;

  String cmd = "AT+SEND=" + receiverID + "," + data.length() + "," + data + "\r\n";

  for (int i = 0; i < retryCount; i++) {
    loraSerial.print(cmd);

    unsigned long t = millis();
    while (millis() - t < 2000) {
      if (loraSerial.available()) {
        String resp = "";
        while (loraSerial.available()) {
          resp += (char)loraSerial.read();
          delay(1);
        }
        if (resp.indexOf("+OK") >= 0) return true;
        if (resp.indexOf("+ERR") >= 0) break;
      }
    }
    delay(300 * (i + 1)); // Backoff: 300ms, 600ms, 900ms
  }
  return false;
}

String waterStatus(float level) {
  if (level < 0)               return "ERROR";
  if (level > WATER_THRESHOLD) return "HIGH";
  return "NORMAL";
}

String gasStatus(int value) {
  return value > GAS_THRESHOLD ? "LEAK" : "SAFE";
}

// ─────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  loraSerial.begin(115200, SERIAL_8N1, 16, 17);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  delay(2000);

  // Configure RYLR998
  loraSerial.print("AT\r\n");                      delay(500);
  loraSerial.print("AT+ADDRESS=2\r\n");            delay(500);
  loraSerial.print("AT+NETWORKID=6\r\n");          delay(500);
  loraSerial.print("AT+BAND=865000000\r\n");       delay(500);
  loraSerial.print("AT+PARAMETER=9,7,1,12\r\n");   delay(500);

  Serial.println("[INFO] Sensor Node Started - " NODE_ID);
}

void loop() {
  if (millis() - lastSendTime >= sendInterval) {

    float water = readWaterLevel();
    int   gas   = readGasSensor();

    bool wOK = sendLoRa(createPacket("WATER", water, waterStatus(water)));
    bool gOK = sendLoRa(createPacket("GAS",   (float)gas, gasStatus(gas)));

    Serial.printf("[TX] Water=%s  Gas=%s\n", wOK ? "OK" : "FAIL", gOK ? "OK" : "FAIL");

    lastSendTime = millis();
  }
}