/**
 * ============================================================
 * main.cpp — IoT Secure Asset Monitoring, ESP32 entry point
 * Author : Chlliboina Yaswanth
 * Board  : ESP32 Dev Module
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
 * SYSTEM OVERVIEW (unchanged from the STM32 port)
 * ─────────────────────────────────────────────────────────────
 *  - RC522 RFID reader scans cards            → VSPI
 *  - Authorized UID list checked              → Access Granted/Denied
 *  - DS1307 RTC timestamps every event        → I2C (Wire)
 *  - DHT11 reads temp + humidity every 5 s    → GPIO 25 bit-bang
 *  - HC-SR501 PIR detects motion              → GPIO 26 interrupt
 *  - Logs uploaded to the server              → on-chip Wi-Fi
 *  - All events logged to the debug terminal  → USB serial
 *
 * HARDWARE CONNECTIONS
 * ─────────────────────────────────────────────────────────────
 * RC522 RFID (VSPI):        DS1307 RTC (Wire):
 *   SCK  → GPIO 18            SDA → GPIO 21
 *   MISO → GPIO 19            SCL → GPIO 22
 *   MOSI → GPIO 23            VCC → see the 3.3V warning in rtc.h
 *   CS   → GPIO 4
 *   RST  → GPIO 27          DHT11:
 *   VCC  → 3.3V               DATA → GPIO 25 (4.7k pull-up to 3.3V)
 *                             VCC  → 3.3V
 * HC-SR501 PIR:
 *   OUT → GPIO 26           Indicators:
 *   VCC → 5V                  Green LED (granted) → GPIO 32
 *   GND → common GND          Red LED (denied)    → GPIO 33
 *                             Yellow LED (motion) → GPIO 14
 * Wi-Fi: ON-CHIP.             Buzzer              → GPIO 13
 *   No ESP8266, no UART,
 *   no AT commands.
 *
 * WHAT DIFFERS FROM THE STM32 F446RE
 * ─────────────────────────────────────────────────────────────
 *  - Wi-Fi is on-chip; the whole AT-command layer is gone (wifi.h).
 *  - The DS1307 is out of spec at 3.3V — prefer a DS3231, or drop the
 *    external RTC and use SNTP (rtc.h explains all three options).
 *  - PIR uses attachInterrupt(), not HAL EXTI + NVIC (pir.h).
 *  - The DHT11 bit-bang needs a critical section because Wi-Fi shares
 *    the core (dht11.cpp).
 *  - Both parts are 3.3V, so — as on the F446RE — the RC522 connects
 *    directly. Never back-feed 5V into an ESP32 GPIO.
 * ============================================================
 */

#include <Arduino.h>

#include <string.h>

#include "dht11.h"
#include "pir.h"
#include "rfid.h"
#include "rtc.h"
#include "wifi.h"

/* ── Indicator pins ──────────────────────────────────────────────
 * Strapping pins (0, 2, 5, 12, 15) avoided so an attached LED cannot
 * change the boot mode. This is the ESP32 equivalent of the PB2/BOOT1
 * caveat in the STM32 port's README.
 */
constexpr uint8_t LED_GREEN_PIN = 32;   // access granted
constexpr uint8_t LED_RED_PIN = 33;     // access denied
constexpr uint8_t LED_YELLOW_PIN = 14;  // motion
constexpr uint8_t BUZZER_PIN = 13;

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
 */
constexpr uint8_t UID_MATCH_LEN = 4;
constexpr uint8_t MAX_AUTHORIZED_CARDS = 10;

static const uint8_t authorizedCards[][UID_MATCH_LEN] = {
    {0xA3, 0xF2, 0xB1, 0x09},  // Card 1 — Admin
    {0x12, 0x34, 0x56, 0x78},  // Card 2 — User 1
    {0xDE, 0xAD, 0xBE, 0xEF},  // Card 3 — User 2
};
static const uint8_t numAuthorizedCards =
    sizeof(authorizedCards) / sizeof(authorizedCards[0]);

enum AccessResult { ACCESS_GRANTED = 0, ACCESS_DENIED };

/* ── State ────────────────────────────────────────────────────── */
static RfidUid uid;
static Dht11Data env;
static char logBuf[192];
static char stamp[RTC_TIMESTAMP_LEN];
static uint32_t lastEnvRead = 0;

/* ── Logging ──────────────────────────────────────────────────── */

/** Print one timestamped line to the debug terminal. */
static void log_line(const char* msg) {
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

/** Compare the scanned UID against the authorized table. */
static AccessResult access_check_uid(const RfidUid* scanned) {
  if (scanned->size < UID_MATCH_LEN) {
    /* A short UID cannot be matched against a 4-byte table entry.
     * Fail closed — never grant on a partial read. */
    return ACCESS_DENIED;
  }

  for (uint8_t i = 0; i < numAuthorizedCards; i++) {
    if (memcmp(scanned->uid, authorizedCards[i], UID_MATCH_LEN) == 0) {
      return ACCESS_GRANTED;
    }
  }
  return ACCESS_DENIED;
}

/** The "[LOG] UID=... Result=... Time=..." audit line. */
static void access_log_event(const RfidUid* scanned, AccessResult result) {
  RtcTime now = {};
  rtc_get_time(&now);

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
  Serial.begin(115200);
  delay(100);  // let the USB CDC settle before the banner

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
  Serial.println("=========================================");
  Serial.println("  IoT Asset Management System");
  Serial.println("  ESP32 | Yaswanth Chlliboina");
  Serial.println("=========================================");

  log_line("System initialized");

  if (!wifi_connected()) {
    /* Not fatal — keep watching the door and logging locally. */
    log_line("WARNING: Wi-Fi not connected, logging locally only");
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
    log_line("PIR: MOTION DETECTED >> Yellow LED ON >> Buzzer ON");
    motion_alert();
  }

  /* 3. Poll the RFID reader. */
  if (rfid_card_present()) {
    if (rfid_read_uid(&uid)) {
      log_line("RFID scan detected...");

      snprintf(logBuf, sizeof(logBuf), "Card UID: %02X %02X %02X %02X",
               uid.uid[0], uid.uid[1], uid.uid[2], uid.uid[3]);
      log_line(logBuf);

      AccessResult result = access_check_uid(&uid);
      access_log_event(&uid, result);

      if (result == ACCESS_GRANTED) {
        access_grant();
        snprintf(logBuf, sizeof(logBuf),
                 "ACCESS GRANTED  >> Green LED ON");
      } else {
        access_deny();
        snprintf(logBuf, sizeof(logBuf),
                 "ACCESS DENIED   >> Red LED ON >> Buzzer ON");
      }

      log_line(logBuf);
      wifi_send_log(logBuf);

      /* Stop one card presentation reading as several scans. */
      delay(SCAN_DEBOUNCE_MS);
    }
  }

  delay(LOOP_TICK_MS);
}
