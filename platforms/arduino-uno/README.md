# IoT Secure Asset Monitoring — Arduino Uno Port

**Status:** 🚧 skeleton. The application logic in `src/main.cpp` is complete; the hardware bodies in the
module `.cpp` files are marked `TODO`.

It builds and runs today — it prints its banner, logs through USB serial, and drives the indicator LEDs and
buzzer for real — but no sensor returns live data until the TODOs (or a matching breadboard build) are done.
The one thing it genuinely *cannot* do is networking; see below.

```bash
pio run                  # build
pio run --target upload  # flash
pio device monitor       # 9600 baud
```

## Module contract

Identical to the ESP32, Raspberry Pi 5 and STM32 F446RE ports, so `src/main.cpp` reads almost the same as
`main.py` and `Core/Src/main.c`:

| Module | Functions |
|---|---|
| `rfid` | `init` `card_present` `read_uid` |
| `pir` | `init` `motion_detected` |
| `dht11` | `init` `read` |
| `rtc` | `init` `get_time` `set_time` `timestamp` |
| `wifi` | `init` `connected` `send_log` |

Access control — the authorized-UID table, the grant/deny decision, the indicator sequences — is application
logic and lives in `src/main.cpp`, matching the STM32 port's `access_control.c`.

## ⚠️ Two hard limits this board has that the others do not

**1. 2 KB of SRAM, total.** The STM32 firmware keeps a 256-byte log buffer plus a 300-byte AT-command
buffer — 556 bytes, ~27% of this board's entire RAM, before the SPI, Wire and SoftwareSerial libraries take
their share. This port's SRAM budget:

| What | Bytes |
|---|---|
| `main.cpp` state block (`uid` `env` `logBuf[72]` `stamp` `lastEnvRead`) | ~115 |
| `wifi.cpp` ESP-01 scratch buffer (once wired) | up to ~200 |
| SPI / Wire / SoftwareSerial library globals | ~120+ |
| Authorized-UID table | **0** — held in flash (PROGMEM), not RAM |
| String literals | **0** — wrapped in `F()`, kept in flash |

That is roughly 500-700 bytes of the 2048 before the runtime heap and stack. Every buffer here is
deliberately small and every literal is in flash because there is no room to be careless.

**2. No networking at all.** The ATmega328P has no radio and no Ethernet MAC. The `wifi` module **cannot
work without external hardware** — an ESP-01 on SoftwareSerial (A0/A1) or a W5500 on the SPI bus. With
neither fitted, `wifi_send_log()` returns `false` and the system logs locally only. That is stated honestly
in `src/wifi.cpp` rather than papered over — **this is the only port where "log upload" requires adding a
second module**. If you have a choice, use the ESP32 port: on-chip Wi-Fi, 520 KB of SRAM, and neither limit
applies.

## Wiring

| Peripheral | Signal | Pin | Notes |
|---|---|---|---|
| RC522 RFID (SPI) | SCK | D13 | **5V→3.3V level shift required** |
| | MISO | D12 | safe direct — 3.3V clears the Uno's 3.0V threshold |
| | MOSI | D11 | **level shift required** |
| | CS | D10 | **level shift required** |
| | RST | D9 | **level shift required** |
| | VCC | 3.3V pin | **never 5V** — the MFRC522 is a 3.3V part |
| DS1307 RTC (Wire) | SDA | A4 | 4.7k pull-up to 5V |
| | SCL | A5 | **in spec here at 5V** — no workaround needed |
| DHT11 | DATA | D8 | 4.7k–10k pull-up to 5V (DHT11 is 3.0–5.5V) |
| HC-SR501 PIR | OUT | D2 (INT0) | 5V supply, 3.3V TTL out — safe direct |
| Green LED (granted) | anode | D4 | series resistor |
| Red LED (denied) | anode | D5 | series resistor |
| Yellow LED (motion) | anode | D6 | series resistor |
| Buzzer | + | D7 | drive via transistor above ~20 mA |
| ESP-01 (optional) | TX | A0 | — |
| | RX | A1 | **level shift required** |

> ⚠️ **The level-shift warning is unique to this board.** The RC522 is a 3.3V part; the Uno drives 5V. The
> ESP32, Pi 5 and F446RE are all 3.3V parts and connect to the RC522 directly. Every Uno **output** into the
> RC522 (SCK, MOSI, CS, RST — and the ESP-01's RX) needs a 4-channel level shifter (TXB0104 / 74AHCT125) or
> a 1k+2k resistor divider. See `include/rfid.h`.

## What differs from the STM32 F446RE

| | STM32 F446RE | Arduino Uno |
|---|---|---|
| Networking | external ESP8266, AT commands over USART2 | **none without external hardware** — the whole upload is optional here |
| Logic level | 3.3V | **5V** — only port that must level-shift the RC522 |
| RTC | DS1307 at 5V | DS1307 at 5V — **genuinely in spec**, the one peripheral this board suits best |
| PIR interrupt | CubeMX EXTI + NVIC, any pin | **D2 or D3 only** — the two external interrupts |
| µs delays | TIM1 counter | `delayMicroseconds()`; the DHT11 bit loop needs `noInterrupts()` because `millis()` steals ~4 µs |
| UID table | RAM | PROGMEM + `memcmp_P` — flash, not RAM |
| Upload result | fire-and-forget into a UART | same, into SoftwareSerial (or no-op with no module) |

## Safety

`indicators_safe()` in `src/main.cpp` and every turn-off path in `access_grant()`, `access_deny()` and
`motion_alert()` are **implemented for real, not stubbed**. A stub that fails to de-energise the buzzer or
the "access granted" LED is a hazard, not a cosmetic bug — a jammed buzzer is a nuisance and a stuck green
LED is a false authorisation signal. `pir_motion_detected()`'s read-and-clear latch and the DHT11 checksum
check are real for the same reason: they are the logic main.cpp trusts.

## Filling in the TODOs

| File | What is stubbed |
|---|---|
| `src/rfid.cpp` | `SPI.begin`, register read/write, REQA, anti-collision. Port from `platforms/stm32f446re/Core/Src/rc522.c` — the protocol is identical, only `SPI.transfer` replaces `HAL_SPI_Transmit` |
| `src/pir.cpp` | `attachInterrupt()` only; the latch is already real. **Must be D2 or D3.** |
| `src/dht11.cpp` | the 40-bit exchange, with `noInterrupts()`/`interrupts()` around the bit loop |
| `src/rtc.cpp` | `Wire` transfers; BCD conversion and `rtc_timestamp()` are already real |
| `src/wifi.cpp` | the whole ESP-01 AT layer, **and** fit an ESP-01/W5500 — without the hardware it stays a serial log |

Also outstanding, and flagged in the code:

- [ ] Replace the placeholder UIDs in `src/main.cpp`. `0xDEADBEEF` in a shipped access-control table is an
      open door.
- [ ] Level-shift the RC522 and ESP-01 lines — that is the failure people hit first on this board.
- [ ] Rethink networking seriously before choosing this board: an ESP-01 on SoftwareSerial caps around
      9600 baud and shares a CPU with the DHT11 critical section.

## Top-level layout

```
platforms/arduino-uno/
├── include/   dht11.h  pir.h  rfid.h  rtc.h  wifi.h   — shared contract, hardware notes
├── src/       main.cpp                                                        — application logic
│              dht11.cpp  pir.cpp  rfid.cpp  rtc.cpp  wifi.cpp                 — hardware skeletons
└── platformio.ini                                                            — Uno env, 9600 monitor
```