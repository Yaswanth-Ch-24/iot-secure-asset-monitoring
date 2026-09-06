/**
 * ============================================================
 * pir.h — HC-SR501 motion detection (Arduino Uno)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / motion_detected
 *
 * Latching semantics, copied from the STM32 firmware: the interrupt
 * sets a flag, and motion_detected() returns that flag and clears it.
 * A caller never misses an event and never sees one twice.
 *
 * DIFFERENCE FROM THE STM32 PORT: the ATmega328P has exactly TWO
 * external interrupts — INT0 on D2 and INT1 on D3. The F446RE can put
 * EXTI on any pin, so this port has no choice about which pin the PIR
 * uses. D2 is it; D3 is the only spare, and the ESP-01 link in
 * wifi.h does not need an interrupt, so D3 stays free.
 *
 * The HC-SR501 needs a 5V supply and swings its OUT 0/3.3V. On a 5V
 * Uno that is the one direction that is safe without shifting: 3.3V
 * clears the Uno's 3.0V logic-high threshold.
 * ============================================================
 */

#ifndef PIR_H
#define PIR_H

#include <stdint.h>

/** HC-SR501 OUT → D2 (INT0). D2 or D3 only — see the note above. */
constexpr uint8_t PIR_PIN = 2;

/**
 * HC-SR501 holds OUT high for its own retrigger window (2–300 s, set
 * by the on-board pot). This is a software floor on top of that so one
 * wave of a hand does not queue several alerts.
 */
constexpr uint32_t PIR_DEBOUNCE_MS = 2000;

/**
 * Configure the pin as an input and attach a rising-edge interrupt.
 *
 * The HC-SR501 takes roughly 60 s after power-up to settle; expect
 * spurious triggers before then.
 */
void pir_init();

/**
 * True exactly once per detected motion event.
 * Reading clears the latch, so call it from one place only.
 */
bool pir_motion_detected();

#endif  // PIR_H
