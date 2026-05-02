#include "resistor_ladder_buttons.h"

ResistorLadderButtons::ResistorLadderButtons(uint8_t adcPin,
                                             uint16_t debounceMs,
                                             uint16_t holdDelayMs,
                                             uint16_t holdIntervalMs,
                                             uint8_t numSamples,
                                             uint16_t valueTolerance)
  : adcPin(adcPin),
    debounceDelay(debounceMs),
    holdDelay(holdDelayMs),
    holdInterval(holdIntervalMs),
    numSamples(numSamples),
    valueTolerance(valueTolerance),
    pendingButton(NO_BUTTON),
    lastDebounceTime(0),
    stableButton(NO_BUTTON),
    previousStableButton(NO_BUTTON),
    holdStartTime(0),
    heldFired(false),
    lastHoldFireTime(0)
{}

void ResistorLadderButtons::addButton(uint8_t id,
                                      uint16_t minVal,
                                      uint16_t maxVal,
                                      std::function<void(uint8_t)> downCallback,
                                      std::function<void(uint8_t)> heldCallback,
                                      std::function<void(uint8_t)> upCallback)
{
  buttons.push_back({id, minVal, maxVal, downCallback, heldCallback, upCallback});
}

void ResistorLadderButtons::begin() {
  pinMode(adcPin, INPUT);
}

void ResistorLadderButtons::update() {
  // 1. Average ADC samples
  uint32_t sum = 0;
  for (uint8_t i = 0; i < numSamples; i++) {
    sum += analogRead(adcPin);
    delay(1);
  }
  uint16_t adcValue = sum / numSamples;

  // 2. Map value to a button
  uint8_t currentButton = getButtonForAdc(adcValue);

  // 3. Debounce
  if (currentButton != pendingButton) {
    pendingButton = currentButton;
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) >= debounceDelay) {
    if (stableButton != pendingButton) {
      uint8_t prev = stableButton;
      stableButton = pendingButton;

      if (prev != NO_BUTTON) {
        callOnUp(prev);
        heldFired = false;
      }

      if (stableButton != NO_BUTTON) {
        callOnDown(stableButton);
        holdStartTime = millis();
        heldFired = false;
      }

      previousStableButton = prev;
    }
  }

  // 4. Held logic
  if (stableButton != NO_BUTTON) {
    unsigned long now = millis();
    if (!heldFired && (now - holdStartTime) >= holdDelay) {
      heldFired = true;
      callOnHeld(stableButton);
      lastHoldFireTime = now;
    } else if (heldFired && holdInterval > 0 && (now - lastHoldFireTime) >= holdInterval) {
      callOnHeld(stableButton);
      lastHoldFireTime = now;
    }
  }
}

uint8_t ResistorLadderButtons::getButtonForAdc(uint16_t adcValue) {
  for (const auto& btn : buttons) {
    if (adcValue >= btn.minVal - valueTolerance && adcValue <= btn.maxVal + valueTolerance) {
      return btn.id;
    }
  }
  return NO_BUTTON;
}

void ResistorLadderButtons::callOnDown(uint8_t id) {
  for (const auto& btn : buttons) {
    if (btn.id == id && btn.onDown) {
      btn.onDown(id);
      break;
    }
  }
}

void ResistorLadderButtons::callOnHeld(uint8_t id) {
  for (const auto& btn : buttons) {
    if (btn.id == id && btn.onHeld) {
      btn.onHeld(id);
      break;
    }
  }
}

void ResistorLadderButtons::callOnUp(uint8_t id) {
  for (const auto& btn : buttons) {
    if (btn.id == id && btn.onUp) {
      btn.onUp(id);
      break;
    }
  }
}