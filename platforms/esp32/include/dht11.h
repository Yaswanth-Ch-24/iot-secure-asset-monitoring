/**
 * ============================================================
 * dht11.h — temperature and humidity, one-wire bit-bang (ESP32)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / read
 *
 * Same 40-bit frame as the STM32 driver: an 18 ms low start pulse,
 * the sensor's 80/80 µs response, then 40 bits where the width of
 * the high phase encodes the value. Byte 4 is a checksum.
 *
 * DIFFERENCE FROM THE STM32 PORT: timing comes from
 * delayMicroseconds() instead of a TIM1 counter. That is accurate
 * enough on its own, but the ESP32 runs Wi-Fi and its own tasks on
 * the same core, and a pre-emption mid-frame corrupts the read. The
 * fix is a critical section around the bit loop — see dht11.cpp.
 * A failed read is not fatal: Dht11Data::valid comes back 0 and the
 * caller skips that cycle, exactly as on the F446RE.
 * ============================================================
 */

#ifndef DHT11_H
#define DHT11_H

#include <stdint.h>

/** DHT11 DATA. Needs a 4.7k-10k pull-up to 3.3V. */
constexpr uint8_t DHT11_PIN = 25;

/** The DHT11 cannot be read faster than once per second. */
constexpr uint32_t DHT11_MIN_INTERVAL_MS = 1000;

/** One reading. Mirrors the STM32 DHT11_Data struct exactly. */
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
