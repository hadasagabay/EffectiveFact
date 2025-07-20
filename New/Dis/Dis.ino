#include <WiFi.h>
#include <WebServer.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <map>


// רשת
// const char* ssid = "Kita-2";
// const char* password = "Xnhbrrfxho";

const char* ssid ="HP";
const char* password = "matana10";

// חיבור לשרת
WebServer server(80);

// חיבור לטביעת אצבע
HardwareSerial mySerial(2);  // UART2: RX=16, TX=17
Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// פין של חיישן IR
const int IRSensorPin = 27;

struct Worker {
  int id;
  unsigned long totalWorkSeconds;
  unsigned long entryTime;
};

std::map<int, Worker> workers;

void setup() {
  Serial.begin(115200);
  pinMode(IRSensorPin, INPUT);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
  server.on("/scan", handleScan);
  server.begin();

  finger.begin(57600);
  if (finger.verifyPassword()) {
    Serial.println("Fingerprint sensor ready");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }
}

void loop() {
  server.handleClient();
}

void handleScan() {
  int result = finger.getImage();
  if (result != FINGERPRINT_OK) {
    server.send(200, "application/json", "{\"status\":\"error\"}");
    return;
  }

  result = finger.image2Tz();
  if (result != FINGERPRINT_OK) {
    server.send(200, "application/json", "{\"status\":\"error\"}");
    return;
  }

  result = finger.fingerSearch();
  if (result != FINGERPRINT_OK) {
    server.send(200, "application/json", "{\"status\":\"error\"}");
    return;
  }

  int id = finger.fingerID;

  // בדיקה אם העובד נוכח לפי IR
  bool isPresent = (digitalRead(IRSensorPin) == LOW);

  unsigned long now = millis() / 1000;
  if (workers.count(id) == 0) {
    workers[id] = { id, 0, now };
  } else {
    if (isPresent) {
      unsigned long session = now - workers[id].entryTime;
      workers[id].totalWorkSeconds += session;
    }
    workers[id].entryTime = now;
  }

  String json = "{\"status\":\"נכנס\",";
  json += "\"id\":" + String(id) + ",";
  json += "\"totalWorkSeconds\":" + String(workers[id].totalWorkSeconds) + ",";
  json += "\"isPresent\":" + String(isPresent ? "true" : "false") + "}";

  server.send(200, "application/json", json);
}
