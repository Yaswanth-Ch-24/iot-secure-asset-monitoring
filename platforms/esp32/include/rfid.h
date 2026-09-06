/**
 * ============================================================
 * rfid.h — RC522 MIFARE reader over SPI (ESP32)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / card_present / read_uid
 *
 * Mirrors the STM32 rc522.c driver: REQA to see whether a card is
 * in the field, then an anti-collision SELECT to pull the UID.
 * The register-level helpers are intentionally NOT part of the
 * contract — they are private to rfid.cpp.
 *
 * DIFFERENCE FROM THE STM32 PORT: none functionally. The RC522 is a
 * 3.3V part and the ESP32 is a 3.3V part, so — as on the F446RE —
 * the bus connects directly with no level shifting.
 * ============================================================
 */

#ifndef RFID_H
#define RFID_H

#include <stdint.h>

/* ── RC522 pins (VSPI). Strapping pins 0/2/5/12/15 avoided. ───── */
constexpr uint8_t RFID_SCK_PIN = 18;  // VSPI CLK
constexpr uint8_t RFID_MISO_PIN = 19; // VSPI MISO
constexpr uint8_t RFID_MOSI_PIN = 23; // VSPI MOSI
constexpr uint8_t RFID_CS_PIN = 4;    // active low
constexpr uint8_t RFID_RST_PIN = 27;  // active low

/* RC522 tops out at 10 MHz; 4 MHz is the usual safe choice. */
constexpr uint32_t RFID_SPI_HZ = 4000000;

/** Longest UID this driver reports. MIFARE Classic 1K uses 4. */
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
