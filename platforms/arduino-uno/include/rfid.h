/**
 * ============================================================
 * rfid.h — RC522 MIFARE reader over SPI (Arduino Uno)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / card_present / read_uid
 *
 * Mirrors the STM32 rc522.c driver: REQA to see whether a card is in
 * the field, then an anti-collision SELECT to pull the UID. The
 * register-level helpers are private to rfid.cpp.
 *
 * ⚠️ DIFFERENCE FROM EVERY OTHER PORT — LEVEL SHIFTING IS REQUIRED.
 *   The MFRC522 is a 3.3V part with a 3.6V absolute maximum on its
 *   pins. The Uno drives 5V logic. The ESP32, Pi 5 and F446RE are all
 *   3.3V, so they connect directly; this board does not.
 *
 *   Uno → RC522 (SCK, MOSI, CS, RST) must be shifted DOWN. Either:
 *     - a 4-channel level shifter (TXB0104 / 74AHCT125), or
 *     - resistor dividers, 1k series + 2k to GND on each line.
 *   RC522 MISO → Uno needs nothing: 3.3V clears the Uno's 3.0V
 *   logic-high threshold at 5V Vcc, with little margin but reliably.
 *
 *   RC522 VCC goes to the Uno's 3.3V pin, NOT 5V. 5V destroys it.
 *   The Uno's on-board regulator supplies ~50 mA on that pin, which
 *   is enough for the RC522's ~26 mA peak.
 * ============================================================
 */

#ifndef RFID_H
#define RFID_H

#include <stdint.h>

/* ── RC522 pins. SCK/MISO/MOSI are fixed by the ATmega328P's SPI. ── */
constexpr uint8_t RFID_SCK_PIN = 13;   // fixed — SCK
constexpr uint8_t RFID_MISO_PIN = 12;  // fixed — MISO
constexpr uint8_t RFID_MOSI_PIN = 11;  // fixed — MOSI
constexpr uint8_t RFID_CS_PIN = 10;    // SS, active low
constexpr uint8_t RFID_RST_PIN = 9;    // active low

/**
 * RC522 tops out at 10 MHz. The Uno's SPI divides its 16 MHz clock,
 * so DIV4 gives 4 MHz — the closest safe step.
 */
constexpr uint32_t RFID_SPI_HZ = 4000000;

/**
 * Longest UID this driver reports.
 *
 * SRAM NOTE: the STM32 struct reserves 10 bytes for a 7- or 10-byte
 * UID cascade. This port keeps the same 10 so the contract matches,
 * but only one RfidUid is ever instantiated (in main.cpp) — do not
 * put one on the stack in a loop.
 */
constexpr uint8_t RFID_UID_MAX = 10;

/** A scanned card identifier. Mirrors the STM32 RC522_UID struct. */
struct RfidUid {
  uint8_t size;               // valid bytes in uid[]
  uint8_t uid[RFID_UID_MAX];  // big-endian as read off the card
  uint8_t sak;                // Select Acknowledge byte
};

/**
 * Bring up SPI, reset the reader and switch the antenna on.
 * @return true if the RC522 answered with a plausible version byte.
 */
bool rfid_init();

/** True when a card is sitting in the RF field (REQA answered). */
bool rfid_card_present();

/**
 * Run anti-collision and copy the UID out.
 * @param uid destination; untouched when the read fails.
 * @return true on a complete read.
 */
bool rfid_read_uid(RfidUid* uid);

#endif  // RFID_H
