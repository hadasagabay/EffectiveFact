#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"


// חיבור DHT
#define DHTPIN 26
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// יצירת אובייקט שרת
AsyncWebServer server(80);

void setup() {
  Serial.begin(115200);
  dht.begin();

  // התחברות ל-WiFi
  WiFi.begin(ssid, password);
  Serial.print("מתחבר ל-WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi מחובר");
  Serial.println(WiFi.localIP());

  // הגדרת ראוט לדף הראשי "/"
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    if (isnan(humidity) || isnan(temperature)) {
      request->send(500, "text/plain", "שגיאה בקריאת החיישן");
      return;
    }

    String response = "טמפרטורה: " + String(temperature) + " C\n";
    response += "לחות: " + String(humidity) + " %";
    request->send(200, "text/plain", response);
  });

  // התחלת השרת
  server.begin();
}

void loop() {
  // לא נדרש קוד בלולאה - הכל מתבצע באירועים
}

