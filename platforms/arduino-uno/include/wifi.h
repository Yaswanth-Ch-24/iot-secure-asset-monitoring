/**
 * ============================================================
 * wifi.h — event log upload (Arduino Uno)
 * ============================================================
 *
 * Shared module contract (identical on all four platforms):
 *   init / connected / send_log
 *
 * ⚠️⚠️ HONEST ASSESSMENT: THE UNO CANNOT DO THIS ON ITS OWN. ⚠️⚠️
 *
 * The ATmega328P has no radio, no Wi-Fi MAC, no Ethernet MAC and no
 * TCP/IP stack. There is no software fix for that. This module is
 * therefore NOT "a driver with some TODOs" — it is a driver for
 * hardware you must add. Three outcomes, and only three:
 *
 * ── Option A: external ESP-01 (ESP8266) on SoftwareSerial ────────
 *   The route this header is written for. Feasible, but constrained:
 *
 *   - BAUD RATE. The ESP-01 ships at 115200. SoftwareSerial on a
 *     16 MHz AVR is a bit-banged UART that blocks with interrupts off
 *     for a whole character time; it is unreliable above ~38400 and
 *     genuinely reliable only at 9600. You MUST reconfigure the module
 *     once over a hardware UART:  AT+UART_DEF=9600,8,1,0,0
 *     At 9600 a 70-byte log line takes ~73 ms to clock out.
 *
 *   - INTERRUPT COLLISION. SoftwareSerial disables interrupts while it
 *     shifts bits. So does the DHT11 bit loop (dht11.cpp). If a log
 *     upload overlaps a sensor read, one of them corrupts. main.cpp
 *     only uploads on an RFID event and only reads the DHT11 every
 *     5 s, so they rarely collide — but "rarely" is not "never", and
 *     the PIR interrupt on D2 can be delayed by either.
 *
 *   - LEVEL SHIFTING. The ESP-01 is 3.3V with a 3.6V maximum. The
 *     Uno's TX at 5V must be divided down (1k + 2k) or shifted. The
 *     ESP-01's 3.3V TX into the Uno's RX is fine as-is.
 *
 *   - POWER. The ESP-01 draws up to ~320 mA in transmit bursts. The
 *     Uno's 3.3V pin supplies about 50 mA. It WILL brown out if you
 *     power it from the board. Use a separate 3.3V regulator (AMS1117
 *     or better) with a 100 µF bulk capacitor at the module.
 *
 *   - SRAM. SoftwareSerial's receive buffer is 64 bytes, plus the
 *     library's own state, plus whatever you need to parse the AT
 *     replies. Budget ~120 bytes of the 2048 available. See the SRAM
 *     table in this folder's README.
 *
 * ── Option B: W5500 Ethernet module on the SPI bus ──────────────
 *   More reliable — the TCP/IP stack lives in the W5500's own
 *   silicon, so there is no AT parser and no bit-banged UART. It
 *   shares SPI with the RC522 on a different CS pin (D6 is free if
 *   you move the yellow LED). But it is wired Ethernet, not Wi-Fi,
 *   and the Ethernet library costs roughly 1.5 KB of SRAM — three
 *   quarters of this board's total. On an Uno that is very tight.
 *
 * ── Option C: don't network from this board ─────────────────────
 *   Perfectly reasonable. The Uno logs every event to its USB serial
 *   port at 9600 baud; a host PC or a Raspberry Pi tails that port
 *   and does the uploading. main.cpp already works this way: it
 *   prints every line locally FIRST and treats the upload as
 *   best-effort, so with no module fitted the system still functions
 *   as a complete local access-control logger.
 *
 * WHAT THIS CODE DOES WITH NO MODULE FITTED: wifi_init() returns
 * false, wifi_connected() returns false, wifi_send_log() returns
 * false immediately without touching the UART. Nothing pretends to
 * have worked. main.cpp checks the return and logs a warning once.
 *
 * Compare the ESP32 port, where all of this collapses into
 * `WiFi.begin()` on the chip that is already there.
 * ============================================================
 */

#ifndef WIFI_H
#define WIFI_H

#include <stdint.h>

/**
 * Set to 1 once an ESP-01 is actually wired up as described above.
 *
 * Left at 0 by default and checked at the top of every function in
 * wifi.cpp — a board with no module must not sit blocking on a
 * SoftwareSerial read that can never answer. This flag is the honest
 * default: no hardware, no pretending.
 */
#define WIFI_MODULE_FITTED 0

/* ── ESP-01 link, SoftwareSerial ──────────────────────────────────
 * A0/A1 used as digital pins. D0/D1 are the hardware UART (the debug
 * terminal), D2 is the PIR interrupt, D8–D13 are DHT11 + SPI, and
 * D4/D5/D7 are indicators — A0/A1 are what is left.
 *
 * On an ATmega328P every pin supports a pin-change interrupt, so
 * SoftwareSerial can receive on A0.
 */
constexpr uint8_t WIFI_RX_PIN = 14;  // A0 — Uno receives, ESP-01 TX
constexpr uint8_t WIFI_TX_PIN = 15;  // A1 — Uno transmits, ESP-01 RX

/** Must match AT+UART_DEF. Do not raise this — see the note above. */
constexpr uint32_t WIFI_BAUD = 9600;

/* ── Credentials ──────────────────────────────────────────────────
 * TODO: these are compiled into flash as plain text. On an AVR there
 * is no secure element and no encrypted NVS — anyone with the .hex
 * can read them. Treat this network as untrusted.
 */
constexpr const char* WIFI_SSID = "YOUR_SSID";
constexpr const char* WIFI_PASSWORD = "YOUR_PASSWORD";

/** Log server. Split into host/port because AT+CIPSTART wants them apart. */
constexpr const char* WIFI_SERVER_HOST = "192.168.1.100";
constexpr uint16_t WIFI_SERVER_PORT = 8080;

/** Bounded so a missing module can never hang the monitoring loop. */
constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 15000;
constexpr uint32_t WIFI_SEND_TIMEOUT_MS = 3000;

/**
 * Bring up the ESP-01 and join the network.
 *
 * Non-fatal by design, exactly as on the STM32: the monitoring loop
 * must start whether or not the network is up. A node that stops
 * watching the door because Wi-Fi is down is worse than one that
 * logs locally.
 *
 * @return true only if a module is fitted AND it associated.
 *         Always false when WIFI_MODULE_FITTED is 0.
 */
bool wifi_init();

/** True while the ESP-01 reports an IP address. */
bool wifi_connected();

/**
 * Upload one log line, best-effort.
 * @return true if the server accepted it. False costs the caller
 *         nothing — main.cpp has already logged locally.
 */
bool wifi_send_log(const char* msg);

#endif  // WIFI_H
