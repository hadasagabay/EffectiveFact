#define BUZZER_PIN 25;


void setup() {
  pinMode(BUZZER_PIN, OUTPUT);
}


void loop() {
  tone(BUZZER_PIN, 1000); // תדר 1KHz
  Serial.print("hi");
  delay(1000);
  noTone(BUZZER_PIN);
  Serial.print("no");
  delay(1000);
}

// #define bazzer 26
// void setup() {
//   Serial.begin(9600);
//   pinMode(bazzer, OUTPUT);
// }
// void loop()
// {
//   digitalWrite(bazzer, HIGH);
//   delay(500);
//   digitalWrite(bazzer, LOW);
//   delay(500);
// }