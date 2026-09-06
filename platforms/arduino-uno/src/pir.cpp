/**
 * ============================================================
 * pir.cpp — HC-SR501 motion detection (Arduino Uno)
 * ============================================================
 * STATUS: skeleton. The latch and the debounce window are real; the
 * interrupt attachment is TODO.
 *
 * The latch logic is deliberately implemented rather than stubbed —
 * it is pure software, it is what main.cpp's motion branch depends
 * on, and getting it wrong means either missed intrusions or a
 * buzzer that re-triggers forever.
 * ============================================================
 */

#include "pir.h"

#include <Arduino.h>

/* Written from interrupt context, read from loop() — must be
 * volatile, exactly as motionFlag is on the F446RE. */
static volatile bool motionFlag = false;
static volatile uint32_t lastTriggerMs = 0;

/**
 * Rising-edge ISR.
 *
 * On ATmega328P this runs in SRAM and is triggered by INT0 (D2).
 */
static void pir_isr() {
  uint32_t now = millis();

  /* Software debounce on top of the module's own retrigger window. */
  if ((now - lastTriggerMs) >= PIR_DEBOUNCE_MS) {
    lastTriggerMs = now;
    motionFlag = true;
  }
}

void pir_init() {
  /* Real: a defined input state before the interrupt is armed.
   * INPUT because the HC-SR501 drives the line push-pull (0V / 3.3V).
   * 3.3V clears the Uno's 3.0V logic-high threshold directly. */
  pinMode(PIR_PIN, INPUT);

  // TODO: attachInterrupt(digitalPinToInterrupt(PIR_PIN), pir_isr, RISING);
  (void)pir_isr;

  Serial.print(F("TODO: pir_init() -> attach RISING interrupt on D"));
  Serial.println(PIR_PIN);
  Serial.println(F("      HC-SR501 needs ~60s to stabilise after power-up"));
}

bool pir_motion_detected() {
  /* Read-and-clear. Real, not stubbed — main.cpp relies on this
   * returning true exactly once per event.
   *
   * noInterrupts()/interrupts() makes the read-modify-write atomic
   * against pir_isr().
   */
  noInterrupts();
  bool triggered = motionFlag;
  motionFlag = false;
  interrupts();

  return triggered;
}
