/**
 * ============================================================
 * wifi.h — event log upload (ESP32)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / connected / send_log
 *
 * DIFFERENCE FROM THE STM32 PORT — this is the biggest divergence in
 * the whole repository:
 *   The F446RE has no radio, so it drives an external ESP8266 with
 *   AT commands over USART2 ("AT+CIPSEND=<len>", then the payload).
 *   On an ESP32 that entire layer disappears. There is no second
 *   module, no UART, no AT parser — WiFi.h and HTTPClient talk to the
 *   server directly. So this file has the same three functions and a
 *   completely different implementation underneath.
 *
 * Consequences worth knowing:
 *   - send_log() can report real success or failure (an HTTP status
 *     code), where the STM32 version fires blind into a UART.
 *   - Wi-Fi and the DHT11 bit-bang compete for the same core; see the
 *     critical-section note in dht11.cpp.
 * ============================================================
 */

#ifndef WIFI_H
#define WIFI_H

/* ── Credentials ──────────────────────────────────────────────────
 * TODO: move these out of source before publishing anything. Options:
 *   - a build_flag: -DWIFI_SSID=\"...\" in platformio.ini (gitignored)
 *   - WiFiManager, so the device serves a config portal on first boot
 *   - NVS / Preferences, provisioned once over serial
 */
constexpr const char* WIFI_SSID = "YOUR_SSID";
constexpr const char* WIFI_PASSWORD = "YOUR_PASSWORD";

/** Where event logs are POSTed. Plain HTTP — see the TODO in wifi.cpp. */
constexpr const char* WIFI_LOG_URL = "http://192.168.1.100:8080/api/logs";

/** How long wifi_init() waits for an association before giving up. */
constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000;

/**
 * Start the station interface and associate.
 *
 * Non-fatal by design: the STM32 firmware logs to its debug UART
 * whether or not the ESP8266 answers, and this port keeps that
 * behaviour. A monitoring node that stops watching the door because
 * the network is down is worse than one that logs locally.
 *
 * @return true if associated before the timeout.
 */
bool wifi_init();

/** True while associated and holding an IP address. */
bool wifi_connected();

/**
 * Upload one log line.
 * @return true if the server accepted it (2xx).
 */
bool wifi_send_log(const char* msg);

#endif  // WIFI_H
