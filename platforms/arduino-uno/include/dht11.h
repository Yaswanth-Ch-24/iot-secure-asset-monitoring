/**
 * ============================================================
 * dht11.h — temperature and humidity, one-wire bit-bang (Arduino Uno)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / read
 *
 * Same 40-bit frame as the STM32 driver: an 18 ms low start pulse,
 * the sensor's 80/80 µs response, then 40 bits where the width of the
 * high phase encodes the value. Byte 4 is a checksum.
 *
 * DIFFERENCE FROM THE STM32 PORT: timing comes from
 * delayMicroseconds() rather than a TIM1 counter, and the bit loop
 * must run with interrupts disabled — the Arduino core's millis()
 * timer fires every 1.024 ms and steals ~4 µs, which is enough to
 * misread a bit whose 0/1 boundary is ~40 µs wide. See dht11.cpp.
 *
 * The DHT11 is a 3.0–5.5V part, so on this 5V board it runs at 5V and
 * its output is a clean 5V logic level. No shifting needed — unlike
 * the RC522.
 * ============================================================
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/** DHT11 DATA → D8. Needs a 4.7k–10k pull-up to 5V. */
constexpr uint8_t DHT11_PIN = 8;

/** The DHT11 cannot be read faster than once per second. */
constexpr uint32_t DHT11_MIN_INTERVAL_MS = 1000;

/**
 * One reading. Mirrors the STM32 DHT11_Data struct exactly.
 *
 * Whole degrees and whole percent, as the DHT11 reports them. The
 * root README's sample output shows "Temp=28.5C", which this sensor
 * cannot produce — the fractional bytes (data[1] and data[3]) are
 * always zero on a DHT11. A DHT22 gives tenths.
 */
struct Dht11Data {
  uint8_t temperature;  // whole degrees Celsius
  uint8_t humidity;     // whole percent RH
  uint8_t valid;        // 1 = checksum matched, 0 = discard
};

/** Park the data line in its idle (released, pulled-up) state. */
void dht11_init();

/**
 * Perform one full 40-bit exchange.
 * @return the reading; check .valid before using the fields.
 */
Dht11Data dht11_read();

#endif  // DHT11_H
