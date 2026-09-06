/**
 * ============================================================
 * wifi.cpp — event log upload over on-chip Wi-Fi (ESP32)
 * ============================================================
 * STATUS: skeleton. All network bodies are TODO.
 *
 * ⚠️ THIS FILE IS NOT A PORT OF THE STM32 CODE — it is a replacement.
 *
 * The F446RE has no radio. Its WiFi_SendLog() in main.c drives an
 * external ESP8266 over USART2 with AT commands:
 *
 *     AT+CIPSEND=<len>\r\n   then the payload
 *
 * On an ESP32 there is no second module and no AT layer at all. So the
 * three functions in wifi.h keep their names and their meaning, and
 * everything below them is different. Two things improve as a result:
 *
 *   1. send_log() can tell whether the server actually received the
 *      line. The STM32 version fires into a UART after a fixed
 *      HAL_Delay(100) without reading the '>' prompt or the SEND OK,
 *      so a dropped log there is completely silent.
 *   2. There is no 3.3V/5V level-shifting question on the link.
 * ============================================================
 */

#include "wifi.h"

#include <Arduino.h>

// TODO: #include <WiFi.h>
// TODO: #include <HTTPClient.h>

bool wifi_init() {
  /* TODO: WiFi.mode(WIFI_STA);
   *       WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
   *
   * TODO: unsigned long start = millis();
   *       while (WiFi.status() != WL_CONNECTED &&
   *              (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
   *         delay(250);
   *       }
   *       return WiFi.status() == WL_CONNECTED;
   *
   * Deliberately bounded rather than a `while (!connected)` spin: the
   * monitoring loop has to start whether or not the network is up.
   */
  Serial.print("TODO: wifi_init() -> associate with SSID ");
  Serial.println(WIFI_SSID);
  return false;
}

bool wifi_connected() {
  // TODO: return WiFi.status() == WL_CONNECTED;
  return false;
}

bool wifi_send_log(const char* msg) {
  if (msg == nullptr) {
    return false;
  }

  if (!wifi_connected()) {
    /* Not an error worth stopping for. main.cpp has already written
     * the line to the debug terminal; the upload is best-effort. */
    return false;
  }

  /* TODO: HTTPClient http;
   *       http.begin(WIFI_LOG_URL);
   *       http.addHeader("Content-Type", "application/json");
   *       http.setTimeout(5000);
   *
   * TODO: char body[256];
   *       snprintf(body, sizeof(body), "{\"event\":\"%s\"}", msg);
   *       int code = http.POST(body);
   *       http.end();
   *       return code >= 200 && code < 300;
   *
   * TODO: escape msg before it goes into that JSON string. The log
   *       lines main.cpp builds contain no quotes or backslashes
   *       today, but a card UID is attacker-influenced input and this
   *       is a security log — build the body with a real serializer
   *       (ArduinoJson) rather than snprintf.
   *
   * TODO: use HTTPS (WiFiClientSecure) with a pinned CA. Plain HTTP
   *       means anyone on the LAN can read who badged in when, and
   *       forge entries. Fine on a bench, not fine deployed.
   *
   * TODO: buffer failed lines. An access-control audit trail that
   *       silently drops events during a network outage is not an
   *       audit trail. A small ring buffer in NVS, flushed on
   *       reconnect, is enough.
   */
  Serial.print("TODO: wifi_send_log() -> POST to ");
  Serial.print(WIFI_LOG_URL);
  Serial.print(" : ");
  Serial.println(msg);
  return false;
}
