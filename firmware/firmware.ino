#include <Adafruit_PN532.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <Adafruit_PWMServoDriver.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#ifdef ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
ESP8266WebServer server(80);
#else
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClient.h>
WebServer server(80);
#endif

#include "secrets.h"

#define PN532_IRQ 14
#define PN532_RESET 3
#define SERVO_SHIELD_PIN 0
#define BUTTON_PIN_BLUE 15
#define BUTTON_PIN_YELLOW 16
#define DOOR_SWITCH_PIN 2

// Neomatrix configuration
#define NEO_PIN 15
#define MATRIX_TILE_WIDTH 4  // width of each individual matrix tile
#define MATRIX_TILE_HEIGHT 8 // height of each individual matrix tile
#define MATRIX_TILE_H 1      // number of matrices arranged horizontally
#define MATRIX_TILE_V 1      // number of matrices arranged vertically
#define mw (MATRIX_TILE_WIDTH * MATRIX_TILE_H)
#define mh (MATRIX_TILE_HEIGHT * MATRIX_TILE_V)
#define NUMMATRIX (mw * mh)

#define LED_RED_MEDIUM (15 << 11)
#define LED_GREEN_MEDIUM (31 << 5)

#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pulse length count (out of 4096)

#define OLED_RESET 3

const int LOCKED_POS = 45;
const int UNLOCKED_POS = 0;
const int NUM_PIXELS = 32;
const int FADE_DELAY = 1;

int buttonState = 0;
int doorState = 0; // 0 == closed, 1 == ajar
bool lockedState = 0; // TODO might be able to dervice from reading Servo state
unsigned long Timer = millis();
unsigned long DEBOUNCE_TIMEOUT = 15000UL;

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

#include "servo.h"
#include "server.h"
#include "client.h"

Adafruit_SSD1306 display(128, 32, &Wire, OLED_RESET);
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
uint8_t matrix_brightness = 10;
CRGB matrixleds[NUMMATRIX];
FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(
    matrixleds, MATRIX_TILE_WIDTH,
    MATRIX_TILE_HEIGHT,
    MATRIX_TILE_H,
    MATRIX_TILE_V,
    NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG + NEO_TILE_TOP + NEO_TILE_LEFT + NEO_TILE_ZIGZAG);
const uint16_t colors[] = {
  matrix->Color(255, 0, 0),
  matrix->Color(0, 255, 0),
  matrix->Color(0, 0, 255)
};

void setup() {
  /* Serial */
  #ifndef ESP8266
    while (!Serial); // for Leonardo/Micro/Zero
  #endif
  Serial.begin(9600);

  /* OLED */
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.display();
  delay(2000);
  display.clearDisplay();

  /* Servo motor */
  pwm.begin();
  pwm.setPWMFreq(60);

  /* NFC/RFID */
  nfc.begin();
  Wire.setClockStretchLimit(2000);
  nfc.setPassiveActivationRetries(1);
  nfc.SAMConfig();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (!versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1);
  }
  Serial.println((versiondata>>24) & 0xFF, HEX);

  /* Inputs */
  pinMode(BUTTON_PIN_BLUE, INPUT);
  pinMode(BUTTON_PIN_YELLOW, INPUT);
  pinMode(DOOR_SWITCH_PIN, INPUT);

  /* Neomatrix */
  FastLED.addLeds<NEOPIXEL, NEO_PIN>(matrixleds, NUMMATRIX);
  matrix->begin();
  matrix->setBrightness(matrix_brightness);

  /* WiFi server */
  WiFi.begin(SSID, PASSWORD);
  // TODO figure out why this causes the connection to take forever
  // WiFi.config(staticIP, subnet, gateway, dns);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

#ifdef ESP8266
  if (MDNS.begin("esp8266")) {
#else
  if (MDNS.begin("esp32")) {
#endif
    Serial.println("MDNS responder started");
  }
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  const char *headerkeys[] = { "passcode", "task" };
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char *);
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");

  // In case of loss of power
  unlock();
}

void printLines(String top, String middle, String bottom = "") {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print(top);
  display.setCursor(0, 12);
  display.println(middle);
  display.setCursor(0, 24);
  display.println(bottom);
  display.display();
}

void checkDoorState() {
  Serial.print("Door state: ");
  Serial.println(digitalRead(DOOR_SWITCH_PIN) == LOW);
  if (digitalRead(DOOR_SWITCH_PIN) != LOW) {
    doorState = 1;
    Serial.println("OPEN");
    Timer = millis() - DEBOUNCE_TIMEOUT;
  } else if (millis() - Timer >= DEBOUNCE_TIMEOUT) {
    doorState = 0;
    Serial.println("CLOSED");
    Timer = millis();
  }
}

void loop() {
  int prevLocked = lockedState;
  int prevDoorstate = doorState;

  logDeviceData();

  if (lockedState) {
    matrix->fillScreen(LED_GREEN_MEDIUM);
    matrix->show();
  } else {
    matrix->fillScreen(LED_RED_MEDIUM);
    matrix->show();
  }

  server.handleClient();

  printLines(
    lockedState ? "Locked" : "Unlocked",
    doorState ? "Open" : "Closed",
    WiFi.localIP().toString());

  checkButton();
  checkDoorState();
  checkNFIC();

  if (lockedState != prevLocked || doorState != prevDoorstate) {
    String lockedStateString = lockedState ? "true" : "false";
    String doorStateString = doorState ? "true" : "false";
    String requestData = String("[{\"type\": \"EMIT_CUSTOM_STATE_UPDATE\", \"stateUpdates\": {\"locked\": " + lockedStateString +  ", \"ajar\": " + doorStateString + "}}]\r\n");
    Serial.println(requestData);
    sendRequest(requestData);
  }
}

void checkButton() {
  buttonState = digitalRead(BUTTON_PIN_BLUE);

  if (buttonState == HIGH) {
    toggle();
  }
}

void checkNFIC() {
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };
  uint8_t uidLength;

  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

  uint32_t cardidentifier = 0;

  if (success) {
    Serial.println("Card read");
    int isAuthorized = 0;

    cardidentifier = uid[3];
    cardidentifier <<= 8; cardidentifier |= uid[2];
    cardidentifier <<= 8; cardidentifier |= uid[1];
    cardidentifier <<= 8; cardidentifier |= uid[0];

    if (cardidentifier) Serial.println(cardidentifier);

    // Check if authorized
    for (int i = 0; i < AUTHORIZED_IDS_AMOUNT; i++) {
      if (AUTHORIZED_IDS[i] == cardidentifier) {
        isAuthorized = 1;
      }
    }

    if (isAuthorized) {
      printLines("ACCESS GRANTED", String(cardidentifier));
      toggle();
    } else {
      printLines("ACCESS DENIED", String(cardidentifier));
    }

    delay(500);
  } else {
    Serial.println(success);
  }
}

void logDeviceData() {
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());

  Serial.print("Device MAC: ");
  Serial.println(WiFi.macAddress());

  Serial.print("Memory heap: ");
  Serial.println(ESP.getFreeHeap());
}
