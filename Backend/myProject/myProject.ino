#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <map>

// const char* ssid = "Kita-2";
// const char* password = "Xnhbrrfxho";

const char* ssid = "HP";
const char* password = "matana10";

const int IRSensorPin = 27;
const int touchPin = 13;
const int buzzerPin = 25;
#define DHTPIN 26
#define DHTTYPE DHT11

// יצירת אובייקטים 
WebServer server(80);
HardwareSerial mySerial(2); // טביעת אצבע משתמש בתקשורת טורית
Adafruit_Fingerprint finger(&mySerial);// יוצר אובייקט של חיישן טביעת אצבע שמתקשר עם הסרייל
DHT dht(DHTPIN, DHTTYPE);// יצירת אובייקט לחיישן DHT

// מבנה נתוני עובד 
struct Worker {
  int id;
  unsigned long totalWorkSeconds;       // סה"כ זמן עבודה
  unsigned long effectiveWorkSeconds;   // זמן עבודה רצינית
  unsigned long sessionStart;           // תחילת סשן
  bool isWorking;
};

// משתנים גלובליים 
std::map<int, Worker> workers;
uint8_t nextID = 1;
unsigned long lastBuzzTime = 0;  // זמן אחרון שנשמע באזר

//  אתחול חיישן טביעת אצבע, פותחת תקשורת טורית 
void setupFingerprintSensor() {
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  Serial.println("בודק את חיישן טביעת האצבע...");
  if (!finger.verifyPassword()) {
    Serial.println("שגיאה: החיישן לא זוהה. בדוק חיבורים.");
    while (true) delay(1);
  }

// אם האצבע לא קיימת במאגר מגדיר חדש ואומר כמה יש במערכת
  finger.getTemplateCount();
  nextID = finger.templateCount + 1;
  Serial.print("נמצאו ");
  Serial.print(finger.templateCount);
  Serial.println(" טביעות אצבע רשומות.");
}

// אתחול המערכת, חיבור לרשת, הגדרת נתיבי HTTP, הפעלת השרת ואתחול הטיבעת אצבע 
void setup() {
  Serial.begin(115200);
  pinMode(IRSensorPin, INPUT);
  pinMode(touchPin, INPUT);
  pinMode(buzzerPin, OUTPUT);
  dht.begin();

  WiFi.begin(ssid, password);
  Serial.print("מתחבר ל-WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.on("/scan", HTTP_OPTIONS, []() {
    server.sendHeader("Access-Control-Allow-Origin", "*");//זמינות לכל הבקשות 
    server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS"); //השיטות המותרות בגישה שלי
    server.sendHeader("Access-Control-Allow-Headers", "*"); // כל הכותרות כלולות בבקשות הדפדפן
    server.send(204);
  });

  server.on("/scan", HTTP_GET, handleScan);
  server.begin();

  setupFingerprintSensor();
  Serial.println("השרת מוכן!");
}

// לולאה ראשית שבודקת אם נמצא בעמדה ונוגע בעכבר  
void loop() {
  server.handleClient();

  unsigned long now = millis() / 1000;

  for (auto &entry : workers) {
    Worker &w = entry.second;

    if (w.isWorking) {
      bool isPresent = (digitalRead(IRSensorPin) == LOW);
      bool isActive = (digitalRead(touchPin) == HIGH);

      if (isPresent && !isActive) {
        if (now - lastBuzzTime >= 10) {
          tone(buzzerPin, 1000);
          delay(300);
          noTone(buzzerPin);
          lastBuzzTime = now;
        }
      }
    }
  }
}

//  טיפול בסריקת טביעת אצבע 
void handleScan() {
  server.sendHeader("Access-Control-Allow-Origin", "*");

  int id = getOrRegisterFingerprint();
  if (id == 0) {
    sendError();
    return;
  }

  unsigned long now = millis() / 1000;
  bool isPresent = (digitalRead(IRSensorPin) == LOW);
  bool isActive = (digitalRead(touchPin) == HIGH);

  if (workers.count(id) == 0) {
    workers[id] = { id, 0, 0, 0, false };
  }

  Worker &w = workers[id];
  String statusMessage = "";
  unsigned long currentSessionSeconds = 0;

  if (!w.isWorking) {
    if (isPresent && isActive) {
      w.isWorking = true;
      w.sessionStart = now;
      statusMessage = "בוקר טוב! התחלת עבודה.";
    } else {
      statusMessage = "אנא שב וגע בעכבר להתחלת העבודה.";
    }
  } else {
    if (!isPresent) {
      unsigned long sessionTime = now - w.sessionStart;
      if (sessionTime > 60) {
        w.totalWorkSeconds += sessionTime;
      }
      w.isWorking = false;
      statusMessage = "סיימת את יום העבודה. נתראה מחר!";
    } else if (!isActive) {
      currentSessionSeconds = now - w.sessionStart;
      statusMessage = "נראה שאינך פעיל. אנא גע בעכבר כדי להמשיך לעבוד.";
    } else {
      unsigned long sessionTime = now - w.sessionStart;
      w.totalWorkSeconds += sessionTime;
      w.effectiveWorkSeconds += sessionTime;
      w.sessionStart = now;
      currentSessionSeconds = sessionTime;
      statusMessage = "אתה בעבודה, תמשיך ככה!";
    }
  }

// בדיקה טמפרטורה ולחות ואם לא קיים מחזיר 0
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  float heatIndex = dht.computeHeatIndex(temperature, humidity, false);

  if (isnan(humidity) || isnan(temperature)) { // אם יש כשל בחיישן 
    humidity = 0;
    temperature = 0;
    heatIndex = 0;
  }

//  בניית JSON כתשובה 
  String json = "{";
  json += "\"status\":\"" + statusMessage + "\",";
  json += "\"id\":" + String(id) + ",";
  json += "\"totalWorkSeconds\":" + String(w.totalWorkSeconds) + ",";
  json += "\"effectiveWorkSeconds\":" + String(w.effectiveWorkSeconds) + ",";
  json += "\"currentSessionSeconds\":" + String(currentSessionSeconds) + ",";
  json += "\"isPresent\":" + String(isPresent ? "true" : "false") + ",";
  json += "\"isActive\":" + String(isActive ? "true" : "false") + ",";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidity, 1) + ",";
  json += "\"heatIndex\":" + String(heatIndex, 1);
  json += "}";

  server.send(200, "application/json", json);
}

// שליחת שגיאה 
void sendError() {
  server.send(200, "application/json", "{\"status\":\"error\"}");
}

// זיהוי או רישום טביעת אצבע
int getOrRegisterFingerprint() {
  Serial.println("Place your finger...");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz() != FINGERPRINT_OK) return 0;

  if (finger.fingerSearch() == FINGERPRINT_OK) {
    Serial.print("זוהתה טביעה עם ID: ");
    Serial.println(finger.fingerID);
    return finger.fingerID;
  } else {
    Serial.println("New fingerprint, enrolling...");
    uint16_t id = nextID;
    if (enrollFingerprint(id)) {
      nextID++;
      return id;
    } else {
      return 0;
    }
  }
}

// רישום טביעת אצבע חדשה במקרה שהID לא קיים 
bool enrollFingerprint(uint8_t id) {
  Serial.println("Place your finger to enroll...");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(1) != FINGERPRINT_OK) return false;

  Serial.println("Remove finger...");
  delay(1500);
  while (finger.getImage() != FINGERPRINT_NOFINGER);

  Serial.println("Place same finger again...");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz(2) != FINGERPRINT_OK) return false;

  if (finger.createModel() != FINGERPRINT_OK) return false;
  return (finger.storeModel(id) == FINGERPRINT_OK);
}
