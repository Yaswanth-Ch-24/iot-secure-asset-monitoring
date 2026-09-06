/**
 * ============================================================
 * rtc.cpp — DS1307 real-time clock over I2C (ESP32)
 * ============================================================
 * STATUS: skeleton. BCD conversion and rtc_timestamp() are real; the
 * Wire transfers are TODO.
 *
 * Port the register layout from platforms/stm32f446re/Core/Src/
 * ds1307.c. Registers 0x00-0x06 are seconds, minutes, hours, day,
 * date, month, year — all BCD.
 * ============================================================
 */

#include "rtc.h"

#include <Arduino.h>
#include <stdio.h>

// TODO: #include <Wire.h>

/** Cached copy, so rtc_timestamp() does not hit the bus per log line. */
static RtcTime cached = {0, 0, 0, 1, 1, 1, 0};
static bool clockValid = false;

/* ── BCD helpers — real, pure arithmetic ──────────────────────── */

static uint8_t dec_to_bcd(uint8_t v) {
  return static_cast<uint8_t>(((v / 10) << 4) | (v % 10));
}

static uint8_t bcd_to_dec(uint8_t v) {
  return static_cast<uint8_t>(((v >> 4) * 10) + (v & 0x0F));
}

/* ── Public contract ──────────────────────────────────────────── */

bool rtc_init() {
  // TODO: Wire.begin(RTC_SDA_PIN, RTC_SCL_PIN);
  // TODO: Wire.setClock(100000);   // DS1307 is standard-mode only

  /* Probe the address, then clear bit 7 of register 0x00 (CH, clock
   * halt). A DS1307 straight out of the packet ships with CH set and
   * its oscillator stopped, so time never advances until it is
   * cleared. The STM32 driver does not do this — it is a real bug
   * there, worth fixing on both.
   *
   * TODO: Wire.beginTransmission(RTC_I2C_ADDR);
   *       if (Wire.endTransmission() != 0) return false;
   * TODO: read 0x00, write it back with bit 7 masked off.
   */
  (void)dec_to_bcd;
  (void)bcd_to_dec;

  clockValid = false;
  Serial.println("TODO: rtc_init() -> Wire.begin + clear the DS1307 CH bit");
  Serial.println("      NOTE: DS1307 is a 4.5-5.5V part; see rtc.h");
  return false;
}

bool rtc_get_time(RtcTime* t) {
  if (t == nullptr) {
    return false;
  }

  /* TODO: Wire.beginTransmission(RTC_I2C_ADDR);
   *       Wire.write(0x00);
   *       if (Wire.endTransmission() != 0) return false;
   *       if (Wire.requestFrom(RTC_I2C_ADDR, (uint8_t)7) != 7) return false;
   *
   * TODO: cached.seconds = bcd_to_dec(Wire.read() & 0x7F);  // mask CH
   *       cached.minutes = bcd_to_dec(Wire.read());
   *       cached.hours   = bcd_to_dec(Wire.read() & 0x3F);  // 24h mask
   *       cached.day     = bcd_to_dec(Wire.read());
   *       cached.date    = bcd_to_dec(Wire.read());
   *       cached.month   = bcd_to_dec(Wire.read());
   *       cached.year    = bcd_to_dec(Wire.read());
   *       clockValid = true;
   */

  /* Until then, hand back the cache and report failure honestly, so
   * rtc_timestamp() prints its placeholder rather than 2000-01-01. */
  *t = cached;
  return clockValid;
}

bool rtc_set_time(const RtcTime* t) {
  if (t == nullptr) {
    return false;
  }

  /* TODO: Wire.beginTransmission(RTC_I2C_ADDR);
   *       Wire.write(0x00);
   *       Wire.write(dec_to_bcd(t->seconds) & 0x7F);  // clears CH
   *       Wire.write(dec_to_bcd(t->minutes));
   *       Wire.write(dec_to_bcd(t->hours) & 0x3F);    // force 24h
   *       Wire.write(dec_to_bcd(t->day));
   *       Wire.write(dec_to_bcd(t->date));
   *       Wire.write(dec_to_bcd(t->month));
   *       Wire.write(dec_to_bcd(t->year));
   *       return Wire.endTransmission() == 0;
   */
  cached = *t;
  return false;
}

void rtc_timestamp(char* buf, size_t len) {
  /* Implemented for real on every platform — pure string formatting,
   * and every log line in main.cpp goes through it. */
  if (buf == nullptr || len == 0) {
    return;
  }

  RtcTime now = {};
  if (!rtc_get_time(&now)) {
    /* Deliberately not a plausible-looking date. A security log with
     * a fabricated timestamp is worse than one that admits the clock
     * is unavailable. */
    snprintf(buf, len, "[---------- --:--:--] ");
    return;
  }

  snprintf(buf, len, "[20%02d-%02d-%02d %02d:%02d:%02d] ", now.year, now.month,
           now.date, now.hours, now.minutes, now.seconds);
}

/* ── Alternative worth considering on this platform ───────────────
 * This board has Wi-Fi, so an external RTC is optional:
 *
 *   configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org");
 *   struct tm tmNow;
 *   if (getLocalTime(&tmNow)) { ... }
 *
 * SNTP plus the ESP32's internal RTC is more accurate than a DS1307
 * and removes the 3.3V/5V problem entirely. The trade-off is that
 * time is wrong until the network is up, and lost across a cold boot
 * without network — which matters for an access-control audit log.
 * A DS3231 alongside SNTP covers both cases.
 * ============================================================
 */
