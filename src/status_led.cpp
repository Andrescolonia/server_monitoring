#include "status_led.h"

#include <Arduino.h>

#include "config.h"

namespace {

RackStatus currentBlinkStatus = RackStatus::Connecting;
uint32_t lastBlinkMs = 0;
bool blinkOn = false;

void setStatusColor(RackStatus status, bool enabled) {
  if (!enabled) {
    setRgb(false, false, false);
    return;
  }

  switch (status) {
    case RackStatus::Connecting:
      setRgb(false, false, true);
      break;
    case RackStatus::Offline:
      setRgb(false, true, true);
      break;
    case RackStatus::Optimal:
      setRgb(false, true, false);
      break;
    case RackStatus::Warning:
      setRgb(true, true, false);
      break;
    case RackStatus::Critical:
      setRgb(true, false, false);
      break;
    case RackStatus::SensorError:
      setRgb(true, false, true);
      break;
  }
}

}  // namespace

void initStatusLed() {
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_LED_B, OUTPUT);
  setRgb(false, false, false);
}

void setRgb(bool red, bool green, bool blue) {
  const uint8_t onLevel = LED_COMMON_ANODE ? LOW : HIGH;
  const uint8_t offLevel = LED_COMMON_ANODE ? HIGH : LOW;

  digitalWrite(PIN_LED_R, red ? onLevel : offLevel);
  digitalWrite(PIN_LED_G, green ? onLevel : offLevel);
  digitalWrite(PIN_LED_B, blue ? onLevel : offLevel);
}

void showStatusColor(RackStatus status) {
  setStatusColor(status, true);
}

void updateStatusLed(RackStatus status) {
  const uint32_t now = millis();

  if (status != currentBlinkStatus) {
    currentBlinkStatus = status;
    blinkOn = false;
    lastBlinkMs = now;
    setStatusColor(status, blinkOn);
    return;
  }

  if (now - lastBlinkMs < LED_BLINK_INTERVAL_MS) {
    return;
  }

  lastBlinkMs = now;
  blinkOn = !blinkOn;
  setStatusColor(status, blinkOn);
}

void runLedSelfTest() {
  Serial.println(F("Prueba LED RGB: rojo, verde, azul, amarillo, blanco, apagado"));

  setRgb(true, false, false);
  delay(LED_TEST_STEP_MS);
  setRgb(false, true, false);
  delay(LED_TEST_STEP_MS);
  setRgb(false, false, true);
  delay(LED_TEST_STEP_MS);
  setRgb(true, true, false);
  delay(LED_TEST_STEP_MS);
  setRgb(true, true, true);
  delay(LED_TEST_STEP_MS);
  setRgb(false, false, false);
}
