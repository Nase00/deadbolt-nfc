/*
 * Based heavily on Adafruit's example sketch for reading NFC input.
 * https://learn.adafruit.com/adafruit-pn532-rfid-nfc/faq
 */

#include <Adafruit_PN532.h>
#include <Wire.h>
#include <SPI.h>

#include <Servo.h>
#include "authorized_ids.h"

#define IRQ 6
#define RESET 8

Servo servo;

const int LOCKED_POS = 45;
const int UNLOCKED_POS = 0;
const int BUTTON_PIN = 7;
const int LOCKED_ALERT_PIN = 4;
const int UNLOCKED_ALERT_PIN = 3;
const int LED_PIN = 13;
int ALERT_PIN;

int buttonState = 0;
bool locked = false;
unsigned digit = 0;
char val = 0;

Adafruit_PN532 nfc(IRQ, RESET);

void setup() {
  Serial.begin(9600);

  servo.attach(9);

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  Serial.println((versiondata>>24) & 0xFF, HEX);

  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(1);

  nfc.SAMConfig();

  servo.write(UNLOCKED_POS);

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);
  pinMode(LOCKED_ALERT_PIN, OUTPUT);
  pinMode(UNLOCKED_ALERT_PIN, OUTPUT);

  digitalWrite(LOCKED_ALERT_PIN, LOW);
  digitalWrite(UNLOCKED_ALERT_PIN, LOW);
}

void feedback(bool lockedState) {
  ALERT_PIN = lockedState ? LOCKED_ALERT_PIN : UNLOCKED_ALERT_PIN;

  digitalWrite(LED_PIN, HIGH);
  digitalWrite(ALERT_PIN, HIGH);

  delay(500);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(ALERT_PIN, LOW);
}

void toggle(bool lockedState) {
  if (lockedState) {
    servo.write(UNLOCKED_POS);
    locked = false;
    delay(500);
  } else {
    servo.write(LOCKED_POS);
    locked = true;
    delay(500);
  }
}

void loop() {
  buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == HIGH) {
    toggle(locked);
    feedback(locked);
  }

  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  uint32_t cardidentifier = 0;

  if (success) {
    int isAuthorized = 0;

    cardidentifier = uid[3];
    cardidentifier <<= 8; cardidentifier |= uid[2];
    cardidentifier <<= 8; cardidentifier |= uid[1];
    cardidentifier <<= 8; cardidentifier |= uid[0];

    // Check if authorized
    int i;
    for (i = 0; i < 6; i = i + 1) {
      if (authorizedIDs[i] == cardidentifier) {
        isAuthorized = 1;
      }
    }

    if (isAuthorized) {
      toggle(locked);
      feedback(locked);
    }
  }
}
