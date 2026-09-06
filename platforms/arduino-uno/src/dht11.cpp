/**
 * ============================================================
 * dht11.cpp — temperature and humidity bit-bang (Arduino Uno)
 * ============================================================
 * STATUS: skeleton. The idle-line setup and the checksum path are
 * real; the 40-bit exchange is TODO.
 *
 * DIFFERENCE FROM THE STM32 PORT: timing comes from
 * delayMicroseconds() rather than a TIM1 counter, and the bit loop
 * must run with interrupts disabled — the Arduino core's millis()
 * timer fires every 1.024 ms and steals ~4 µs, which is enough to
 * misread a bit whose 0/1 boundary is ~40 µs wide.
 * ============================================================
 */

#include "dht11.h"

#include <Arduino.h>

static uint32_t lastReadMs = 0;
static Dht11Data lastResult = {0, 0, 0};

void dht11_init() {
  /* Real: park the line released (high via the external pull-up).
   * INPUT_PULLUP also engages the internal ~45k — fit the 4.7k–10k
   * external resistor as well for signal integrity. */
  pinMode(DHT11_PIN, INPUT_PULLUP);

  lastReadMs = 0;
  lastResult = {0, 0, 0};
}

Dht11Data dht11_read() {
  Dht11Data result = {0, 0, 0};
  uint8_t data[5] = {0};

  /* Real rate limit. The DHT11 cannot be read faster than 1 Hz. */
  if (lastReadMs != 0 && (millis() - lastReadMs) < DHT11_MIN_INTERVAL_MS) {
    return lastResult;
  }
  lastReadMs = millis();

  /* ── Start signal: hold low 18 ms, release, wait 30 µs ────────────
   *
   * TODO: pinMode(DHT11_PIN, OUTPUT);
   *       digitalWrite(DHT11_PIN, LOW);
   *       delay(18);
   *       digitalWrite(DHT11_PIN, HIGH);
   *       delayMicroseconds(30);
   *       pinMode(DHT11_PIN, INPUT_PULLUP);
   */

  /* ── Response: sensor pulls low ~80 µs, then high ~80 µs ──────────
   *
   * TODO: spin with a bounded counter for high->low, low->high,
   *       high->low, bailing out to `valid = 0` on timeout.
   */

  /* ── 40 data bits ────────────────────────────────────────────────
   *
   * TODO: noInterrupts();   // disable millis() ISR during bit loop
   *       for (int i = 0; i < 40; i++) {
   *         wait for the line to go high (bounded counter);
   *         delayMicroseconds(40);
   *         data[i / 8] <<= 1;
   *         if (digitalRead(DHT11_PIN) == HIGH) data[i / 8] |= 1;
   *         wait for the line to go low (bounded counter);
   *       }
   *       interrupts();
   */

  /* ── Checksum — real ─────────────────────────────────────────────
   * Byte 4 is the low 8 bits of the sum of the first four.
   * The `data[4] != 0` guard prevents an all-zero stub frame from
   * being reported as valid {0°C, 0%, valid=1}. */
  if (data[4] == static_cast<uint8_t>(data[0] + data[1] + data[2] + data[3]) &&
      data[4] != 0) {
    result.humidity = data[0];
    result.temperature = data[2];
    result.valid = 1;
  }

  lastResult = result;
  return result;
}
