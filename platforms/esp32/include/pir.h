/**
 * ============================================================
 * pir.h — HC-SR501 motion detection (ESP32)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / motion_detected
 *
 * Latching semantics, copied from the STM32 firmware: the interrupt
 * sets a flag, and motion_detected() returns that flag and clears it.
 * So a caller never misses an event, and never sees one twice.
 *
 * DIFFERENCE FROM THE STM32 PORT: the interrupt is registered with
 * attachInterrupt() rather than CubeMX EXTI + NVIC, and the handler
 * must live in IRAM (IRAM_ATTR) because flash may be busy when it
 * fires. The flag is volatile for the same reason it is on the
 * F446RE — it is written from interrupt context.
 * ============================================================
 */

#ifndef PIR_H
#define PIR_H

#include <stdint.h>

/** HC-SR501 OUT. Input-only pins 34-39 also work; 26 is a plain GPIO. */
constexpr uint8_t PIR_PIN = 26;

/**
 * HC-SR501 holds OUT high for its own retrigger window (2-300 s,
 * set by the on-board pot). This is a software floor on top of that
 * so one wave of a hand does not queue several alerts.
 */
constexpr uint32_t PIR_DEBOUNCE_MS = 2000;

/**
 * Configure the pin as an input and attach a rising-edge interrupt.
 *
 * The HC-SR501 needs a 5V supply but its OUT swings 0/3.3V, so it
 * drives an ESP32 pin directly. Do NOT wire its VCC to a GPIO.
 */
void pir_init();

/**
 * True exactly once per detected motion event.
 * Reading clears the latch, so call it from one place only.
 */
bool pir_motion_detected();

#endif  // PIR_H
