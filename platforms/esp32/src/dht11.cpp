/**
 * ============================================================
 * dht11.cpp — temperature and humidity bit-bang (ESP32)
 * ============================================================
 * STATUS: skeleton. The idle-line setup and the checksum path are
 * real; the 40-bit exchange is TODO.
 *
 * Port the frame timing from platforms/stm32f446re/Core/Src/dht11.c.
 * The protocol is identical; only the delay source changes.
 * ============================================================
 */

#include "dht11.h"

#include <Arduino.h>

static uint32_t lastReadMs = 0;
static Dht11Data lastResult = {0, 0, 0};

void dht11_init() {
  /* Real: park the line released (high via the external pull-up).
   * INPUT_PULLUP also engages the internal ~45k, which is too weak on
   * its own — fit the 4.7k-10k external resistor as well. */
  pinMode(DHT11_PIN, INPUT_PULLUP);

  lastReadMs = 0;
  lastResult = {0, 0, 0};
}

Dht11Data dht11_read() {
  Dht11Data result = {0, 0, 0};
  uint8_t data[5] = {0};

  /* Real rate limit. The DHT11 cannot be read faster than 1 Hz; a
   * quicker request returns garbage, so hand back the previous
   * reading instead. main.cpp polls at 5 s so this rarely fires, but
   * it protects anyone who calls this from somewhere else. */
  if (lastReadMs != 0 && (millis() - lastReadMs) < DHT11_MIN_INTERVAL_MS) {
    return lastResult;
  }
  lastReadMs = millis();

  /* ── Start signal: hold low 18 ms, release, wait 30 µs ──────────
   *
   * TODO: pinMode(DHT11_PIN, OUTPUT);
   *       digitalWrite(DHT11_PIN, LOW);
   *       delay(18);
   *       digitalWrite(DHT11_PIN, HIGH);
   *       delayMicroseconds(30);
   *       pinMode(DHT11_PIN, INPUT_PULLUP);
   */

  /* ── Response: sensor pulls low ~80 µs, then high ~80 µs ────────
   *
   * TODO: spin with a bounded counter for high→low, low→high,
   *       high→low, bailing out to `valid = 0` on timeout. Use the
   *       same 10000-iteration ceiling the STM32 driver uses.
   */

  /* ── 40 data bits ───────────────────────────────────────────────
   * Each bit is a ~50 µs low, then a high whose width is the value:
   * ~26-28 µs = 0, ~70 µs = 1. The STM32 driver samples blind 40 µs
   * into the high phase, which is simple and works.
   *
   * TODO: for (int i = 0; i < 40; i++) {
   *         wait for the line to go high (bounded);
   *         delayMicroseconds(40);
   *         data[i / 8] <<= 1;
   *         if (digitalRead(DHT11_PIN) == HIGH) data[i / 8] |= 1;
   *         wait for the line to go low (bounded);
   *       }
   *
   * ⚠️ DIFFERENCE FROM THE STM32 PORT — the reason this is not a
   * copy-paste. On the F446RE this loop owns the CPU. On an ESP32,
   * Wi-Fi and the IDF task scheduler share core 0, and a pre-emption
   * of even 50 µs mid-frame shifts every following bit. Wrap the loop
   * in a critical section:
   *
   *   portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
   *   portENTER_CRITICAL(&mux);
   *   ... the 40-bit loop ...
   *   portEXIT_CRITICAL(&mux);
   *
   * Keep it to the bit loop only — the 18 ms start pulse must NOT be
   * inside it or the Wi-Fi stack starves and the watchdog fires.
   *
   * If reads still come back intermittently invalid, the robust fix
   * on this platform is the RMT peripheral, which captures the pulse
   * train in hardware and is immune to scheduling entirely.
   */

  /* ── Checksum — real, so a corrupt frame can never be reported as
   * a valid temperature. Byte 4 is the low 8 bits of the sum of the
   * first four. */
  if (data[4] == static_cast<uint8_t>(data[0] + data[1] + data[2] + data[3]) &&
      data[4] != 0) {
    result.humidity = data[0];
    result.temperature = data[2];
    result.valid = 1;
  }

  /* An all-zero frame passes the checksum arithmetically (0 == 0),
   * which is why the `data[4] != 0` guard above is there: with the
   * TODOs unfilled every byte is zero, and reporting "0C / 0% valid"
   * would be worse than reporting nothing. */

  lastResult = result;
  return result;
}
