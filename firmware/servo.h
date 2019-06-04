void unlock() {
  if (lockedState != true) {
    Serial.println("Already unlocked");
    return;
  }

  Serial.println("Unlocking");

  for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {
    pwm.setPWM(SERVO_SHIELD_PIN, 0, pulselen);
  }

  delay(500);
}

void lock() {
  if (lockedState == true) {
    Serial.println("Already locked");
    return;
  }

  Serial.println("Locking");

  for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {
    pwm.setPWM(SERVO_SHIELD_PIN, 0, pulselen);
  }

  delay(500);
}

void toggle() {
  if (lockedState) {
    unlock();
  } else {
    lock();
  }

  lockedState = !lockedState;
}
