#include <Arduino.h>
#include <HardwareSerial.h>
#include <ArduinoJson.h>

#include "button.h"
#include "resistor_ladder_buttons.h"

#define RXD1 18
#define TXD1 17
#define CBAUD 9600

HardwareSerial comm(1);

Button buttonPrev(48, "BTN_PREV");
Button buttonNext(36, "BTN_NEXT");

int auxDetectState = 0;
int illuminationState = 0;

const uint8_t BTN_VOL_UP = 1;
const uint8_t BTN_VOL_DOWN = 2;
const uint8_t BTN_SEEK_NEXT = 3;
const uint8_t BTN_SEEK_PREV = 4;
const uint8_t BTN_MODE = 5;
const uint8_t BTN_MUTE = 6;
const uint8_t BTN_SPEAK = 7;
const uint8_t BTN_ANSWER = 8;
const uint8_t BTN_DENY = 9;

ResistorLadderButtons swc(1, 50, 600, 0, 10, 50);

void send_json(JsonDocument& doc) {
  String s;
  serializeJson(doc, comm);
  comm.println();
}

void send_button_up(String button_id) {
  JsonDocument doc;
  doc["command"] = "comm_button";
  doc["value"] = button_id.c_str();
  send_json(doc);
}

void send_aux_detect_state() {
  JsonDocument doc;
  doc["command"] = "comm_aux_detect";
  doc["value"] = auxDetectState;
  send_json(doc);
}

void send_illumination_state() {
  JsonDocument doc;
  doc["command"] = "comm_illumination";
  doc["value"] = illuminationState;
  send_json(doc);
}

void send_swc_button_up(int button_id) {
  Serial.print("SWC button pressed: ");
  Serial.println(button_id);
  JsonDocument doc;
  doc["command"] = "comm_swc";
  doc["value"] = button_id;
  send_json(doc);
}

void process_message(JsonDocument& doc) {
  if (doc.containsKey("command")) {
    String command = doc["command"].as<String>();
    if (command == "request_aux_detect_state") {
      send_aux_detect_state();
    } else {
      Serial.println("Unknown command: " + command);
    }
  }
}

void receive_json() {
  static String buffer;
  static bool in_frame = false;

  while (comm.available()) {
    char c = comm.read();

    if (!in_frame) {
      if (c == '{') {
        buffer = "{";
        in_frame = true;
      }
      continue;
    }

    if (c == '\n') {
      buffer.trim();
      //Serial.println(buffer);

      StaticJsonDocument<256> doc;
      DeserializationError err = deserializeJson(doc, buffer);

      if (!err) {
        Serial.print("Received reply: ");
        serializeJson(doc, Serial);
        Serial.println();
        process_message(doc);
      } else {
        Serial.println("JSON parse failed");
        Serial.println(err.c_str());
      }

      buffer = "";
      in_frame = false;
    } else {
      buffer += c;

      if (buffer.length() > 256) {
        buffer = "";
        in_frame = false;
      }
    }
  }
}

void buttonHandler(Button* btn, ButtonAction action) {
  Serial.print("Button on pin ");
  Serial.print(btn->pin());
  Serial.print(": ");

  uint8_t buttonPin = btn->pin();
  char* buttonId = btn->button_id();

  switch (action) {
    case ButtonAction::Down:
      Serial.println("Down");
      //send_button_down(buttonId);
      break;
    case ButtonAction::Up:
      Serial.println("Up");
      send_button_up(buttonId);
      break;
    case ButtonAction::Held:
      Serial.println("Held");
      //send_button_held(buttonId);
      break;
    case ButtonAction::HeldLong:
      Serial.println("Held Long");
      //send_button_held_long(buttonId);
      break;
  }
}

// vol up - 1920 - 2048
// vol down - 2250 - 2300
// seek next - 400 - 650
// seek prev - 850 - 750
// mode - 1150
// mute - 1490 - 1500
// speak - 2650 - 2660
// answer - 3650 - 3675
// deny - 3100 - 3135

void setup() {
  Serial.begin(115200);

  comm.begin(CBAUD, SERIAL_8N1, RXD1, TXD1);

  swc.begin();
  swc.addButton(BTN_VOL_UP, 1920, 2048, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_VOL_DOWN, 2250, 2300, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_SEEK_NEXT, 400, 600, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_SEEK_PREV, 775, 850, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_MODE, 1125, 1175, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_MUTE, 1490, 1500, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_SPEAK, 2650, 2660, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_ANSWER, 3650, 3675, nullptr, nullptr, send_swc_button_up);
  swc.addButton(BTN_DENY, 3100, 3135, nullptr, nullptr, send_swc_button_up);

  buttonPrev.onAction(buttonHandler);
  buttonNext.onAction(buttonHandler);
  //buttonInp.onAction(buttonHandler);

  pinMode(2, INPUT_PULLUP);
  //pinMode(LED_BUILTIN, OUTPUT);

  pinMode(10, INPUT_PULLDOWN);

  delay(1000);
  Serial.println("Ready");
}

unsigned long lastUpdate = 0;
float loopFrequency = 1000.0f/60.0f;

void loop() {
  receive_json();
  swc.update();

  if (lastUpdate < millis()) {
    buttonPrev.update();
    buttonNext.update();
    //buttonInp.update();

    static int prevAuxDetectState = 0;
    auxDetectState = digitalRead(2);

    if (auxDetectState != prevAuxDetectState) {
      send_aux_detect_state();
      Serial.print("AUX state: ");
      Serial.println(auxDetectState);
      prevAuxDetectState = auxDetectState;
    }

    static int prevIlluminationState = 0;
    illuminationState = digitalRead(10);
    if (illuminationState != prevIlluminationState) {
      send_illumination_state();
      Serial.print("Illumination state: ");
      Serial.println(illuminationState);
      prevIlluminationState = illuminationState;
    }
    
    lastUpdate = millis() + loopFrequency;
  }
}
