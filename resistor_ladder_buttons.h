#ifndef RESISTOR_LADDER_BUTTONS_H
#define RESISTOR_LADDER_BUTTONS_H

#include <Arduino.h>
#include <vector>
#include <functional>

class ResistorLadderButtons {
public:
  static const uint8_t NO_BUTTON = 0;

  /*
  * Constructor
  * adcPin          - analog input pin
  * debounceMs      - time (ms) a new state must be stable before being accepted
  * holdDelayMs     - time (ms) a button must be continuously pressed before the first "held" event
  * holdIntervalMs  - repeat interval (ms) for "held" events; 0 = fire only once after holdDelay
  * numSamples      - how many ADC readings to average (reduces noise)
  */
  ResistorLadderButtons(uint8_t adcPin,
                        uint16_t debounceMs = 50,
                        uint16_t holdDelayMs = 500,
                        uint16_t holdIntervalMs = 100,
                        uint8_t numSamples = 1,
                        uint16_t valueTolerance = 50);

  // Add a button with its ADC range and callbacks (any callback can be nullptr)
  void addButton(uint8_t id,
                  uint16_t minVal,
                  uint16_t maxVal,
                  std::function<void(uint8_t)> downCallback = nullptr,
                  std::function<void(uint8_t)> heldCallback = nullptr,
                  std::function<void(uint8_t)> upCallback = nullptr);

  // Initialise the ADC pin (call in setup())
  void begin();

  // Must be called repeatedly (e.g. in loop())
  void update();

private:
    struct Button {
      uint8_t id;
      uint16_t minVal;
      uint16_t maxVal;
      std::function<void(uint8_t)> onDown;
      std::function<void(uint8_t)> onHeld;
      std::function<void(uint8_t)> onUp;
    };

    uint8_t adcPin;
    uint16_t debounceDelay;
    uint16_t holdDelay;
    uint16_t holdInterval;
    uint8_t numSamples;
    uint16_t valueTolerance;

    std::vector<Button> buttons;
    uint8_t pendingButton;
    unsigned long lastDebounceTime;
    uint8_t stableButton;
    uint8_t previousStableButton;
    unsigned long holdStartTime;
    bool heldFired;
    unsigned long lastHoldFireTime;

    uint8_t getButtonForAdc(uint16_t adcValue);
    void callOnDown(uint8_t id);
    void callOnHeld(uint8_t id);
    void callOnUp(uint8_t id);
};

#endif