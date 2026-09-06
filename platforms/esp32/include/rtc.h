/**
 * ============================================================
 * rtc.h — DS1307 real-time clock over I2C (ESP32)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / get_time / set_time / timestamp
 *
 * rtc_timestamp() is the one function in this module with no
 * hardware in it, so it is implemented for real on every port. Every
 * log line in main.cpp is prefixed with it, which is what keeps the
 * entry points readable across platforms.
 *
 * DIFFERENCE FROM THE STM32 PORT — read this before wiring:
 *   The DS1307 is specified for 4.5-5.5V. At 3.3V it is out of spec:
 *   it may run, but timekeeping drifts and the battery-backup
 *   threshold moves. Three options, in order of preference:
 *     1. Drop the DS1307 entirely. The ESP32 has Wi-Fi, so SNTP plus
 *        its internal RTC gives better time than a DS1307 ever will.
 *        See the SNTP note in rtc.cpp.
 *     2. Fit a DS3231 instead (2.3-5.5V, temperature-compensated,
 *        register-compatible for the seven time registers this
 *        driver touches — the driver needs no change).
 *     3. Keep the DS1307 at 5V behind a bidirectional I2C level
 *        shifter. Never pull SDA/SCL up to 5V on an ESP32 pin.
 * ============================================================
 */

#ifndef RTC_H
#define RTC_H

#include <stddef.h>
#include <stdint.h>

/* ── I2C pins (ESP32 default Wire bus) ────────────────────────── */
constexpr uint8_t RTC_SDA_PIN = 21;
constexpr uint8_t RTC_SCL_PIN = 22;

/** 7-bit address 0x68. Wire.h takes it unshifted, unlike the HAL. */
constexpr uint8_t RTC_I2C_ADDR = 0x68;

/** Minimum buffer for rtc_timestamp(): "[2026-09-04 19:50:00] " + NUL. */
constexpr size_t RTC_TIMESTAMP_LEN = 24;

/** Wall-clock reading. Mirrors the STM32 DS1307_Time struct. */
struct RtcTime {
  uint8_t seconds;
  uint8_t minutes;
  uint8_t hours;  // 24-hour
  uint8_t day;    // 1 = Monday ... 7 = Sunday
  uint8_t date;
  uint8_t month;
  uint8_t year;  // last two digits, e.g. 26 for 2026
};

/**
 * Join the I2C bus and clear the DS1307's CH (clock halt) bit.
 * @return true if the device acknowledged its address.
 */
bool rtc_init();

/** Read all seven time registers. Leaves *t untouched on failure. */
bool rtc_get_time(RtcTime* t);

/** Write all seven time registers. Call once, not every boot. */
bool rtc_set_time(const RtcTime* t);

/**
 * Format the current time as "[20YY-MM-DD HH:MM:SS] ".
 *
 * Implemented for real on every platform — it is pure string work.
 * Falls back to "[---------- --:--:--] " if the clock cannot be
 * read, so a dead RTC never silently produces a plausible-looking
 * timestamp on a security log.
 *
 * @param buf destination, at least RTC_TIMESTAMP_LEN bytes.
 * @param len sizeof(buf).
 */
void rtc_timestamp(char* buf, size_t len);

#endif  // RTC_H
