#include <WiFi.h>
#include <HTTPClient.h>
#include <Adafruit_Fingerprint.h>
#include <HardwareSerial.h>

// WiFi credentials
const char* ssid = "Kita-2";
const char* password = "Xnhbrrfxho";

HardwareSerial mySerial(2); // UART2
Adafruit_Fingerprint finger(&mySerial);

uint8_t nextID = 1;
bool arrived = true; // מצב נוכחי

void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected!");

  setupFingerprintSensor();
}

void loop() {
  uint16_t id = getOrRegisterFingerprint();
  if (id == 0) return;

  String status = arrived ? "ENTER" : "EXIT";

  Serial.print(status);
  Serial.print(":");
  Serial.println(id);

  sendStatusToServer(id, status);

  arrived = !arrived; // הפוך מצב
  delay(3000);
}

void setupFingerprintSensor() {
  mySerial.begin(57600, SERIAL_8N1, 16, 17);
  finger.begin(57600);

  Serial.println("Checking fingerprint sensor...");
  if (!finger.verifyPassword()) {
    Serial.println("Fingerprint sensor not found!");
    while (true) delay(1);
  }

  finger.getTemplateCount();
  nextID = finger.templateCount + 1;

  Serial.print("Found ");
  Serial.print(finger.templateCount);
  Serial.println(" fingerprint templates.");
}

uint16_t getOrRegisterFingerprint() {
  Serial.println("Place your finger...");
  while (finger.getImage() != FINGERPRINT_OK);
  if (finger.image2Tz() != FINGERPRINT_OK) return 0;

  if (finger.fingerSearch() == FINGERPRINT_OK) {
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

// פונקציה לשליחת הסטטוס לשרת
void sendStatusToServer(uint16_t id, String status) {
  if ((WiFi.status() == WL_CONNECTED)) {
    HTTPClient http;

    // כתובת השרת - החלף לפי כתובת ה-IP והפורט שלך
    String url = "http://192.168.1.100:5000/update?id=" + String(id) + "&status=" + status;

    http.begin(url);
    int httpResponseCode = http.GET();

    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Response from server: ");
      Serial.println(response);
    } else {
      Serial.print("Error on sending request: ");
      Serial.println(httpResponseCode);
    }

    http.end();
  } else {
    Serial.println("WiFi not connected");
  }
}
