/**
 * ============================================================
 * rfid.cpp — RC522 MIFARE reader over SPI (Arduino Uno)
 * ============================================================
 * STATUS: skeleton. Pin setup and the safe-state reset are real; the
 * SPI register traffic is TODO.
 *
 * Port the register map and the REQA / anti-collision sequence from
 * platforms/stm32f446re/Core/Src/rc522.c — the protocol is identical,
 * only the transport call changes (SPI.transfer instead of
 * HAL_SPI_Transmit / HAL_SPI_Receive).
 *
 * ⚠️ LEVEL SHIFTING: The MFRC522 is a 3.3V part. Level shift Uno's
 * 5V outputs (SCK, MOSI, CS, RST) down to 3.3V before the RC522.
 * ============================================================
 */

#include "rfid.h"

#include <Arduino.h>

// TODO: #include <SPI.h>

/* ── RC522 register map (from the STM32 driver) ────────────────── */
constexpr uint8_t REG_COMMAND = 0x01;
constexpr uint8_t REG_COM_IEN = 0x02;
constexpr uint8_t REG_COM_IRQ = 0x04;
constexpr uint8_t REG_ERROR = 0x06;
constexpr uint8_t REG_FIFO_DATA = 0x09;
constexpr uint8_t REG_FIFO_LEVEL = 0x0A;
constexpr uint8_t REG_BIT_FRAMING = 0x0D;
constexpr uint8_t REG_MODE = 0x11;
constexpr uint8_t REG_TX_CONTROL = 0x14;
constexpr uint8_t REG_TX_ASK = 0x15;
constexpr uint8_t REG_T_MODE = 0x2A;
constexpr uint8_t REG_T_PRESCALER = 0x2B;
constexpr uint8_t REG_T_RELOAD_H = 0x2C;
constexpr uint8_t REG_T_RELOAD_L = 0x2D;
constexpr uint8_t REG_VERSION = 0x37;

/* ── RC522 commands ───────────────────────────────────────────── */
constexpr uint8_t CMD_IDLE = 0x00;
constexpr uint8_t CMD_TRANSCEIVE = 0x0C;
constexpr uint8_t CMD_SOFT_RESET = 0x0F;

/* ── MIFARE PICC commands ─────────────────────────────────────── */
constexpr uint8_t PICC_REQA = 0x26;
constexpr uint8_t PICC_SEL_CL1 = 0x93;

/* ── Private register helpers ─────────────────────────────────── */

static uint8_t read_register(uint8_t reg) {
  // TODO: address byte for a read is ((reg << 1) & 0x7E) | 0x80
  // TODO: digitalWrite(RFID_CS_PIN, LOW);
  //       SPI.transfer(addr); uint8_t v = SPI.transfer(0x00);
  //       digitalWrite(RFID_CS_PIN, HIGH); return v;
  (void)reg;
  return 0;
}

static void write_register(uint8_t reg, uint8_t value) {
  // TODO: address byte for a write is (reg << 1) & 0x7E
  // TODO: digitalWrite(RFID_CS_PIN, LOW);
  //       SPI.transfer(addr); SPI.transfer(value);
  //       digitalWrite(RFID_CS_PIN, HIGH);
  (void)reg;
  (void)value;
}

static void set_bit_mask(uint8_t reg, uint8_t mask) {
  write_register(reg, read_register(reg) | mask);
}

static void clear_bit_mask(uint8_t reg, uint8_t mask) {
  write_register(reg, read_register(reg) & static_cast<uint8_t>(~mask));
}

static void antenna_on() {
  // TODO: if ((read_register(REG_TX_CONTROL) & 0x03) != 0x03)
  //         set_bit_mask(REG_TX_CONTROL, 0x03);
}

/* ── Public contract ──────────────────────────────────────────── */

bool rfid_init() {
  /* Pin setup is real. CS and RST idle high (both are active low), so
   * the reader is left deselected and out of reset even if every TODO
   * below is still empty. */
  pinMode(RFID_CS_PIN, OUTPUT);
  pinMode(RFID_RST_PIN, OUTPUT);
  digitalWrite(RFID_CS_PIN, HIGH);
  digitalWrite(RFID_RST_PIN, HIGH);

  // TODO: SPI.begin();
  // TODO: SPI.beginTransaction(SPISettings(RFID_SPI_HZ, MSBFIRST, SPI_MODE0));

  /* Hard reset, then soft reset — same sequence as RC522_Reset(). */
  // TODO: digitalWrite(RFID_RST_PIN, LOW);  delay(10);
  //       digitalWrite(RFID_RST_PIN, HIGH); delay(50);
  //       write_register(REG_COMMAND, CMD_SOFT_RESET); delay(50);

  /* Timer + modulation setup, byte for byte from RC522_Init(). */
  // TODO: write_register(REG_T_MODE,      0x80);
  //       write_register(REG_T_PRESCALER, 0xA9);
  //       write_register(REG_T_RELOAD_H,  0x03);
  //       write_register(REG_T_RELOAD_L,  0xE8);
  //       write_register(REG_TX_ASK,      0x40);
  //       write_register(REG_MODE,        0x3D);
  antenna_on();

  /* A healthy MFRC522 reports 0x91 or 0x92 in REG_VERSION. 0x00 and
   * 0xFF both mean "nothing is talking on this bus" — usually a CS or
   * a MISO wiring fault.
   *
   * TODO: uint8_t v = read_register(REG_VERSION);
   *       return (v != 0x00 && v != 0xFF);
   */
  Serial.println(F("TODO: rfid_init() -> bring up SPI + RC522"));
  Serial.println(F("      NOTE: 5V->3.3V level shifting required on SCK/MOSI/CS/RST"));
  return false;
}

bool rfid_card_present() {
  /* REQA at 7 valid bits. See RC522_IsCardPresent() for the full
   * sequence — the ordering of these writes matters:
   *
   * TODO: write_register(REG_COM_IEN, 0x77 | 0x80);
   *       clear_bit_mask(REG_COM_IRQ, 0x80);
   *       set_bit_mask(REG_FIFO_LEVEL, 0x80);        // flush FIFO
   *       write_register(REG_COMMAND, CMD_IDLE);
   *       write_register(REG_FIFO_DATA, PICC_REQA);
   *       write_register(REG_BIT_FRAMING, 7);        // 7 valid bits
   *       write_register(REG_COMMAND, CMD_TRANSCEIVE);
   *       set_bit_mask(REG_BIT_FRAMING, 0x80);       // StartSend
   *
   * TODO: spin on REG_COM_IRQ for up to ~2000 reads waiting for bit
   *       0x30 (RxIRq | IdleIRq) or 0x01 (timer), then
   *       clear_bit_mask(REG_BIT_FRAMING, 0x80) and return false on a
   *       timeout or when REG_ERROR & 0x1B is set.
   */
  (void)PICC_REQA;
  return false;
}

bool rfid_read_uid(RfidUid* uid) {
  if (uid == nullptr) {
    return false;
  }

  /* Anti-collision cascade level 1. See RC522_ReadCardUID().
   *
   * TODO: write_register(REG_COMMAND, CMD_IDLE);
   *       set_bit_mask(REG_FIFO_LEVEL, 0x80);
   *       write_register(REG_FIFO_DATA, PICC_SEL_CL1);
   *       write_register(REG_FIFO_DATA, 0x20);
   *       write_register(REG_BIT_FRAMING, 0x00);
   *       write_register(REG_COMMAND, CMD_TRANSCEIVE);
   *       set_bit_mask(REG_BIT_FRAMING, 0x80);
   *       delay(10);
   *       clear_bit_mask(REG_BIT_FRAMING, 0x80);
   *
   * TODO: n = read_register(REG_FIFO_LEVEL); if (n < 4) return false;
   *       read min(n, 4) bytes out of REG_FIFO_DATA into uid->uid,
   *       set uid->size, then verify the BCC (byte 5 == XOR of the
   *       first four).
   */
  (void)PICC_SEL_CL1;
  return false;
}
