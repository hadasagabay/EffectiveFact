const int touchPin =13; // הפין שאליו מחובר ה־OUT של החיישן
const int ledPin = 2;   // לדוגמה – נורית מובנית ב־ESP32

void setup() {
  pinMode(touchPin, INPUT);
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int touchState = digitalRead(touchPin);
  Serial.println(touchState);
  
  if (touchState == HIGH) {
    digitalWrite(ledPin, HIGH);
  } else {
    digitalWrite(ledPin, LOW);
  }

  delay(100);
}