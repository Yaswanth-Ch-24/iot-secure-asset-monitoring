/**
 * ============================================================
 * pir.cpp — HC-SR501 motion detection (ESP32)
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
 * Rising-edge handler.
 *
 * IRAM_ATTR keeps it out of flash: on an ESP32 the interrupt can fire
 * while flash is being written or a cache line is being filled, and a
 * handler that lives in flash crashes the chip when that happens.
 * This is the practical difference from the F446RE's plain
 * HAL_GPIO_EXTI_Callback, which has no such constraint.
 */
static void IRAM_ATTR pir_isr() {
  uint32_t now = millis();

  /* Software debounce on top of the module's own retrigger window. */
  if ((now - lastTriggerMs) >= PIR_DEBOUNCE_MS) {
    lastTriggerMs = now;
    motionFlag = true;
  }
}

void pir_init() {
  /* Real: a defined input state before the interrupt is armed.
   * INPUT (not INPUT_PULLDOWN) because the HC-SR501 drives the line
   * both ways — it is a push-pull output, not open-collector. */
  pinMode(PIR_PIN, INPUT);

  // TODO: attachInterrupt(digitalPinToInterrupt(PIR_PIN), pir_isr, RISING);
  (void)pir_isr;

  Serial.print("TODO: pir_init() -> attach RISING interrupt on GPIO ");
  Serial.println(PIR_PIN);
  Serial.println("      HC-SR501 needs ~60s to stabilise after power-up");
}

bool pir_motion_detected() {
  /* Read-and-clear. Real, not stubbed — main.cpp relies on this
   * returning true exactly once per event.
   *
   * noInterrupts()/interrupts() makes the read-modify-write atomic
   * against pir_isr(); without it an event landing between the read
   * and the clear is lost. The F446RE version has the same hazard.
   */
  noInterrupts();
  bool triggered = motionFlag;
  motionFlag = false;
  interrupts();

  return triggered;
}
