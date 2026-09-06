/**
 * ============================================================
 * wifi.cpp — event log upload (Arduino Uno)
 * ============================================================
 * STATUS: skeleton. All network bodies are TODO.
 *
 * See include/wifi.h for the full hardware assessment. Short version:
 * the ATmega328P has no radio. You need an ESP-01 on SoftwareSerial,
 * a W5500 on SPI, or just log locally and let a Pi/PC tail the serial
 * port. This file is written for Option A (ESP-01), but returns false
 * for everything until WIFI_MODULE_FITTED is set to 1 in wifi.h.
 * ============================================================
 */

#include "wifi.h"

#include <Arduino.h>

// TODO: #include <SoftwareSerial.h>
// TODO: static SoftwareSerial espSerial(WIFI_RX_PIN, WIFI_TX_PIN);

/**
 * Send a raw AT command and wait for an expected response.
 * Returns true when the response appears within the timeout.
 *
 * TODO: implement with espSerial.print() + polling espSerial.available()
 */
static bool at_cmd(const char* cmd, const char* expect, uint32_t timeout_ms) {
  (void)cmd;
  (void)expect;
  (void)timeout_ms;
  return false;
}

bool wifi_init() {
#if WIFI_MODULE_FITTED == 0
  /* No module wired — say so once and return without touching the UART. */
  Serial.println(F("wifi_init(): no module fitted (WIFI_MODULE_FITTED=0)"));
  Serial.println(F("  logging to serial only - see wifi.h for options"));
  return false;
#endif

  /* TODO: espSerial.begin(WIFI_BAUD);
   *       delay(100);
   *
   * TODO: at_cmd("AT\r\n",         "OK",   2000) &&
   *       at_cmd("AT+CWMODE=1\r\n","OK",   2000) &&  // Station mode
   *       at_cmd(("AT+CWJAP=\"" + String(WIFI_SSID) + "\",\""
   *               + String(WIFI_PASSWORD) + "\"\r\n").c_str(),
   *              "WIFI GOT IP", WIFI_CONNECT_TIMEOUT_MS);
   *
   * NOTE: build the CWJAP string with F() to keep it in flash, or
   * snprintf into a small char[] — do not use Arduino String on a 2 KB
   * board unless you are very careful about heap fragmentation.
   */
  return at_cmd(nullptr, nullptr, 0);  // placeholder; always false
}

bool wifi_connected() {
#if WIFI_MODULE_FITTED == 0
  return false;
#endif
  // TODO: at_cmd("AT+CIPSTATUS\r\n", "STATUS:3", 1000);
  return false;
}

bool wifi_send_log(const char* msg) {
  if (msg == nullptr) {
    return false;
  }

  if (!wifi_connected()) {
    /* Not an error — main.cpp already wrote to serial. */
    return false;
  }

  /* TODO: char cmd[64];
   *       snprintf(cmd, sizeof(cmd),
   *                "AT+CIPSTART=\"TCP\",\"%s\",%u\r\n",
   *                WIFI_SERVER_HOST, WIFI_SERVER_PORT);
   *       if (!at_cmd(cmd, "CONNECT", 5000)) return false;
   *
   * TODO: uint16_t len = strlen(msg) + 2;  // +\r\n
   *       snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u\r\n", len);
   *       if (!at_cmd(cmd, ">", WIFI_SEND_TIMEOUT_MS)) {
   *         at_cmd("AT+CIPCLOSE\r\n", "OK", 1000);
   *         return false;
   *       }
   *
   * TODO: espSerial.print(msg);
   *       espSerial.print("\r\n");
   *       bool ok = at_cmd("", "SEND OK", WIFI_SEND_TIMEOUT_MS);
   *       at_cmd("AT+CIPCLOSE\r\n", "OK", 1000);
   *       return ok;
   */
  return false;
}
