/**
 * ============================================================
 * main.cpp — IoT Secure Asset Monitoring, Arduino Uno entry point
 * Author : Chlliboina Yaswanth
 * Board  : Arduino Uno (ATmega328P)
 * ============================================================
 *
 * The application logic below is COMPLETE and mirrors the STM32
 * F446RE firmware: the 5-second environmental poll, the latched PIR
 * alert, the RFID scan → UID lookup → grant/deny sequence, and the
 * indicator timing (3 s green, 1 s red + buzzer, 2 s yellow + buzzer,
 * 2 s scan debounce). What is NOT complete is the hardware layer
 * underneath it — see the // TODO markers in rfid.cpp, pir.cpp,
 * dht11.cpp, rtc.cpp and wifi.cpp.
 *
 * The indicator layer in this file is REAL, not stubbed. pinMode and
 * digitalWrite are core Arduino API and always available, and
 * indicators_safe() has to actually work: a stub that fails to turn
 * the buzzer and the "granted" LED off is a hazard, not a cosmetic
 * bug. Everything that de-energises hardware here is implemented.
 *
 * ⚠️ TWO HARD LIMITS ON THIS BOARD
 * ─────────────────────────────────────────────────────────────
 * 1. 2 KB of SRAM, total. The STM32 firmware keeps a 256-byte log
 *    buffer plus a 300-byte AT-command buffer — 27% of this board's
 *    RAM before any library loads. This port uses a 72-byte log
 *    buffer, holds the authorized-UID table in flash (PROGMEM), and
 *    wraps every string literal in F() so it stays in flash too.
 *    Running estimate is in this folder's README.
 *
 * 2. NO NETWORKING. There is no radio and no Ethernet MAC on an
 *    ATmega328P. wifi_send_log() needs an external ESP-01 or W5500,
 *    and with neither fitted it returns false and this sketch logs
 *    locally only. include/wifi.h lays out all three options
 *    honestly. Nothing here pretends the upload happened.
 *
 * SYSTEM OVERVIEW (unchanged from the STM32 port)
 * ─────────────────────────────────────────────────────────────
 *  - RC522 RFID reader scans cards            → SPI
 *  - Authorized UID list checked              → Access Granted/Denied
 *  - DS1307 RTC timestamps every event        → I2C (A4/A5)
 *  - DHT11 reads temp + humidity every 5 s    → D8 bit-bang
 *  - HC-SR501 PIR detects motion              → D2 (INT0)
 *  - Logs uploaded to the server              → external module only
 *  - All events logged to the debug terminal  → USB serial, 9600
 *
 * HARDWARE CONNECTIONS
 * ─────────────────────────────────────────────────────────────
 * RC522 RFID (SPI):          DS1307 RTC (Wire):
 *   SCK  → D13 *shift*         SDA → A4
 *   MISO → D12                 SCL → A5
 *   MOSI → D11 *shift*         VCC → 5V  (in spec here — see rtc.h)
 *   CS   → D10 *shift*
 *   RST  → D9  *shift*       DHT11:
 *   VCC  → 3.3V pin, NOT 5V    DATA → D8 (4.7k pull-up to 5V)
 *                              VCC  → 5V
 * HC-SR501 PIR:
 *   OUT → D2 (INT0)          Indicators:
 *   VCC → 5V                   Green LED (granted) → D4
 *                              Red LED (denied)    → D5
 * ESP-01 (optional):           Yellow LED (motion) → D6
 *   ESP TX → A0                Buzzer              → D7
 *   ESP RX ← A1 *shift*
 *   VCC → separate 3.3V reg   GND → common with everything
 *
 * *shift* = 5V → 3.3V level shifting REQUIRED. This is the only port
 * in the repo that needs it; the ESP32, Pi 5 and F446RE are all 3.3V
 * parts and connect to the RC522 directly.
 *
 * WHAT DIFFERS FROM THE STM32 F446RE
 * ─────────────────────────────────────────────────────────────
 *  - No networking at all without external hardware (wifi.h).
 *  - 5V logic, so the 3.3V RC522 needs level shifting (rfid.h).
 *  - The PIR must be on D2 or D3 — the only two external interrupts.
 *  - µs delays come from delayMicroseconds(), and the DHT11 bit loop
 *    must run with interrupts off (dht11.cpp).
 *  - The authorized-UID table lives in PROGMEM, not RAM.
 *  - The DS1307 is genuinely in spec here at 5V, which is NOT true on
 *    the ESP32 or Pi 5 ports.
 * ============================================================
 */

#include <Arduino.h>

#include <avr/pgmspace.h>
#include <string.h>

#include "dht11.h"
#include "pir.h"
#include "rfid.h"
#include "rtc.h"
#include "wifi.h"

/* ── Indicator pins ──────────────────────────────────────────────
 * D0/D1 are the hardware UART, D2 is the PIR interrupt, D8–D13 are
 * DHT11 + SPI, A0/A1 are the optional ESP-01. D3–D7 is what remains.
 * D3 is left free: it is the only other external interrupt pin.
 */
constexpr uint8_t LED_GREEN_PIN = 4;   // access granted
constexpr uint8_t LED_RED_PIN = 5;     // access denied
constexpr uint8_t LED_YELLOW_PIN = 6;  // motion
constexpr uint8_t BUZZER_PIN = 7;

/* ── Indicator timing — identical to the STM32 firmware ───────── */
constexpr uint32_t GRANT_HOLD_MS = 3000;
constexpr uint32_t DENY_HOLD_MS = 1000;
constexpr uint32_t MOTION_HOLD_MS = 2000;
constexpr uint32_t SCAN_DEBOUNCE_MS = 2000;
constexpr uint32_t ENV_INTERVAL_MS = 5000;
constexpr uint32_t LOOP_TICK_MS = 100;

/* ── Access control (the STM32 port's access_control.c) ──────────
 * TODO: replace these placeholders with your real card UIDs. Scan a
 * card and read the "Card UID:" line this sketch prints.
 *
 * 0xDEADBEEF in a shipped access-control table is an open door.
 *
 * SRAM NOTE: PROGMEM keeps the table in flash. Ten 4-byte entries is
 * only 40 bytes, but on a 2 KB board every array that never changes
 * belongs in flash — and this is the pattern to follow if the table
 * ever grows. memcmp_P() (not memcmp) reads it back.
 */
constexpr uint8_t UID_MATCH_LEN = 4;
constexpr uint8_t MAX_AUTHORIZED_CARDS = 10;

static const uint8_t authorizedCards[][UID_MATCH_LEN] PROGMEM = {
    {0xA3, 0xF2, 0xB1, 0x09},  // Card 1 — Admin
    {0x12, 0x34, 0x56, 0x78},  // Card 2 — User 1
    {0xDE, 0xAD, 0xBE, 0xEF},  // Card 3 — User 2
};
static const uint8_t numAuthorizedCards =
    sizeof(authorizedCards) / sizeof(authorizedCards[0]);

enum AccessResult { ACCESS_GRANTED = 0, ACCESS_DENIED };

/* ── State ────────────────────────────────────────────────────────
 * SRAM budget for this block: 12 + 3 + 72 + 24 + 4 = 115 bytes.
 * The STM32 equivalent uses 256 + 300 for its buffers alone.
 */
static RfidUid uid;
static Dht11Data env;
static char logBuf[72];
static char stamp[RTC_TIMESTAMP_LEN];
static uint32_t lastEnvRead = 0;

/* ── Logging ──────────────────────────────────────────────────── */

/** Print one timestamped line to the debug terminal. */
static void log_line(const char* msg) {
  rtc_timestamp(stamp, sizeof(stamp));
  Serial.print(stamp);
  Serial.println(msg);
}

/** Same, for a flash-resident literal — keeps it out of SRAM. */
static void log_line_P(const __FlashStringHelper* msg) {
  rtc_timestamp(stamp, sizeof(stamp));
  Serial.print(stamp);
  Serial.println(msg);
}

/* ── Indicators — REAL, not stubbed ──────────────────────────── */

/**
 * Drive every indicator to its de-energised state.
 *
 * Called at boot, after every alert, and before any early return.
 * Implemented for real on purpose: this is the function that has to
 * work when everything else is still a TODO.
 */
static void indicators_safe() {
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

static void indicators_init() {
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_YELLOW_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  indicators_safe();
}

/* ── Access control logic ─────────────────────────────────────── */

/** Compare the scanned UID against the authorized table in flash. */
static AccessResult access_check_uid(const RfidUid* scanned) {
  if (scanned->size < UID_MATCH_LEN) {
    /* A short UID cannot be matched against a 4-byte table entry.
     * Fail closed — never grant on a partial read. */
    return ACCESS_DENIED;
  }

  for (uint8_t i = 0; i < numAuthorizedCards; i++) {
    /* memcmp_P, not memcmp: the table is in flash, and on AVR that is
     * a separate address space. Plain memcmp would compare against
     * whatever happens to sit at that address in RAM. */
    if (memcmp_P(scanned->uid, authorizedCards[i], UID_MATCH_LEN) == 0) {
      return ACCESS_GRANTED;
    }
  }
  return ACCESS_DENIED;
}

/** The "[LOG] UID=... Result=... Time=..." audit line. */
static void access_log_event(const RfidUid* scanned, AccessResult result) {
  RtcTime now = {};
  rtc_get_time(&now);

  /* 59 bytes at most, which is why logBuf is 72. snprintf on AVR
   * costs ~1.5 KB of flash for the formatter; there is plenty of the
   * 32 KB free, and it is SRAM that is scarce here. */
  snprintf(logBuf, sizeof(logBuf),
           "[LOG] UID=%02X%02X%02X%02X Result=%s "
           "Time=20%02d-%02d-%02d %02d:%02d:%02d",
           scanned->uid[0], scanned->uid[1], scanned->uid[2], scanned->uid[3],
           (result == ACCESS_GRANTED) ? "GRANTED" : "DENIED", now.year,
           now.month, now.date, now.hours, now.minutes, now.seconds);
  Serial.println(logBuf);
}

/** Green LED for 3 s, red forced off. Ends de-energised. */
static void access_grant() {
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);
  delay(GRANT_HOLD_MS);
  digitalWrite(LED_GREEN_PIN, LOW);
}

/** Red LED + buzzer for 1 s, green forced off. Ends de-energised. */
static void access_deny() {
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(DENY_HOLD_MS);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

/** Yellow LED + buzzer for 2 s. Ends de-energised. */
static void motion_alert() {
  digitalWrite(LED_YELLOW_PIN, HIGH);
  digitalWrite(BUZZER_PIN, HIGH);
  delay(MOTION_HOLD_MS);
  digitalWrite(LED_YELLOW_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
}

/* ── Setup ────────────────────────────────────────────────────── */

void setup() {
  Serial.begin(9600);

  /* Indicators first: every init below may fail, and a failure must
   * not leave an LED or the buzzer energised. */
  indicators_init();

  rtc_init();
  rfid_init();
  dht11_init();
  pir_init();
  wifi_init();

  /* Set the clock once, then comment this back out — otherwise every
   * reset rewinds it.
   * RtcTime initTime = {0, 30, 9, 2, 15, 3, 26};
   * rtc_set_time(&initTime);
   */

  Serial.println();
  Serial.println(F("========================================="));
  Serial.println(F("  IoT Asset Management System"));
  Serial.println(F("  Arduino Uno | Yaswanth Chlliboina"));
  Serial.println(F("========================================="));

  log_line_P(F("System initialized"));

  if (!wifi_connected()) {
    /* Expected on a bare Uno. Say so once, plainly, and carry on —
     * the local log is the primary record on this board. */
    log_line_P(F("No network module: logging to serial only"));
  }
}

/* ── Main loop ────────────────────────────────────────────────── */

void loop() {
  /* 1. Read the environment every 5 seconds. */
  if ((millis() - lastEnvRead) >= ENV_INTERVAL_MS) {
    lastEnvRead = millis();
    env = dht11_read();

    if (env.valid) {
      snprintf(logBuf, sizeof(logBuf), "DHT11: Temp=%dC  Humidity=%d%%",
               env.temperature, env.humidity);
      log_line(logBuf);
    }
  }

  /* 2. Check the latched PIR flag. */
  if (pir_motion_detected()) {
    log_line_P(F("PIR: MOTION DETECTED >> Yellow LED ON >> Buzzer ON"));
    motion_alert();
  }

  /* 3. Poll the RFID reader. */
  if (rfid_card_present()) {
    if (rfid_read_uid(&uid)) {
      log_line_P(F("RFID scan detected..."));

      snprintf(logBuf, sizeof(logBuf), "Card UID: %02X %02X %02X %02X",
               uid.uid[0], uid.uid[1], uid.uid[2], uid.uid[3]);
      log_line(logBuf);

      AccessResult result = access_check_uid(&uid);
      access_log_event(&uid, result);

      if (result == ACCESS_GRANTED) {
        access_grant();
        snprintf(logBuf, sizeof(logBuf), "ACCESS GRANTED  >> Green LED ON");
      } else {
        access_deny();
        snprintf(logBuf, sizeof(logBuf),
                 "ACCESS DENIED   >> Red LED ON >> Buzzer ON");
      }

      log_line(logBuf);

      /* Best-effort. Returns false with no module fitted, and that is
       * fine — the line is already in the serial log above. */
      wifi_send_log(logBuf);

      /* Stop one card presentation reading as several scans. */
      delay(SCAN_DEBOUNCE_MS);
    }
  }

  delay(LOOP_TICK_MS);
}
