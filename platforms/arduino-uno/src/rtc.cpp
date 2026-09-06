/**
 * ============================================================
 * rtc.cpp — DS1307 real-time clock over I2C (Arduino Uno)
 * ============================================================
 * STATUS: skeleton. BCD conversion and rtc_timestamp() are real; the
 * Wire transfers are TODO.
 *
 * THIS IS THE ONE PERIPHERAL THE UNO SUITS BEST. The DS1307 is
 * specified 4.5–5.5V, so a 5V Uno drives it directly with 5V I2C
 * pull-ups and no shifting — exactly what the part wants. The ESP32
 * and Pi 5 ports both have to work around it being out of spec at
 * 3.3V.
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
  // TODO: Wire.begin();   // A4 = SDA, A5 = SCL — fixed by ATmega328P

  /* Probe the address, then clear bit 7 of register 0x00 (CH, clock
   * halt). A DS1307 straight out of the packet ships with CH set and
   * its oscillator stopped.
   *
   * TODO: Wire.beginTransmission(RTC_I2C_ADDR);
   *       if (Wire.endTransmission() != 0) return false;
   * TODO: read 0x00, write it back with bit 7 masked off.
   */
  (void)dec_to_bcd;
  (void)bcd_to_dec;

  clockValid = false;
  Serial.println(F("TODO: rtc_init() -> Wire.begin + clear the DS1307 CH bit"));
  Serial.println(F("      DS1307 runs at 5V here — in spec, no shifting needed"));
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
  /* Implemented for real on every platform — pure string formatting. */
  if (buf == nullptr || len == 0) {
    return;
  }

  RtcTime now = {};
  if (!rtc_get_time(&now)) {
    snprintf(buf, len, "[---------- --:--:--] ");
    return;
  }

  snprintf(buf, len, "[20%02d-%02d-%02d %02d:%02d:%02d] ", now.year, now.month,
           now.date, now.hours, now.minutes, now.seconds);
}
