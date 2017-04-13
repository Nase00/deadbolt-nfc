#define TRIGGER_PIN D0
#define LOCKED_ALERT_PIN D1
#define UNLOCKED_ALERT_PIN D2
#define AJAR_PIN D4

bool locked = true;
bool wasOpened = false;
unsigned long Timer = millis();
unsigned long DEBOUNCE_TIMEOUT = 15000UL;

int handleToggle(String pw) {
  if (pw == "hunter2") {
    digitalWrite(TRIGGER_PIN, HIGH);
    delay(100);
    digitalWrite(TRIGGER_PIN, LOW);
    return 1;
  }

  return 0;
}

void setup() {
  pinMode(TRIGGER_PIN, OUTPUT);
  pinMode(LOCKED_ALERT_PIN, INPUT);
  pinMode(UNLOCKED_ALERT_PIN, INPUT);
  pinMode(AJAR_PIN, INPUT_PULLUP);

  Particle.function("toggle", handleToggle);
}

void loop() {
  int locked = digitalRead(LOCKED_ALERT_PIN);
  int unlocked = digitalRead(UNLOCKED_ALERT_PIN);
  bool lockStateChanged = locked == HIGH || unlocked == HIGH;
  bool ajarState = digitalRead(AJAR_PIN);

  if (lockStateChanged) {
    String data = String::format("{\"KEY\":\"%s\"\n, \"VALUE\":\"%d\"\n}", "locked", locked == HIGH);
    Particle.publish("SINGLE_EVENT", data);
    delay(500);
  }

  if (ajarState == HIGH && millis() - Timer >= DEBOUNCE_TIMEOUT) {
    String data = String::format("{\"KEY\":\"%s\"\n, \"VALUE\":\"%d\"\n}", "ajar", true);
    Particle.publish("SINGLE_EVENT", data);
    Timer = millis();
    wasOpened = true;
  } else if (wasOpened && ajarState == LOW) {
    String data = String::format("{\"KEY\":\"%s\"\n, \"VALUE\":\"%d\"\n}", "ajar", false);
    Particle.publish("SINGLE_EVENT", data);
    Timer = millis() - DEBOUNCE_TIMEOUT;
    wasOpened = false;
  }

  delay(100);
}
