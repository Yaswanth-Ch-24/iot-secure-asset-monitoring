# IoT Secure Asset Monitoring — ESP32 Port

**Status:** 🚧 skeleton. The application logic in `src/main.cpp` is complete; the hardware bodies in the
module `.cpp` files are marked `TODO`.

It builds and runs today — it prints its banner, logs through USB serial, and the indicator LEDs and buzzer
are driven for real — but no sensor returns live data until the TODOs are filled in.

```bash
pio run                  # build
pio run --target upload  # flash
pio device monitor        # 115200 baud
```

## Module contract

Identical to the Arduino Uno, Raspberry Pi 5 and STM32 F446RE ports, so `src/main.cpp` reads almost the same
as `main.py` and `Core/Src/main.c`:

| Module | Functions |
|---|---|
| `rfid` | `init` `card_present` `read_uid` |
| `pir` | `init` `motion_detected` |
| `dht11` | `init` `read` |
| `rtc` | `init` `get_time` `set_time` `timestamp` |
| `wifi` | `init` `connected` `send_log` |

Access control — the authorized-UID table, the grant/deny decision, the indicator sequences — is application
logic and lives in `src/main.cpp`, matching the STM32 port's `access_control.c`.

## Wiring

| Peripheral | Signal | GPIO | Notes |
|---|---|---|---|
| RC522 RFID (VSPI) | SCK | 18 | |
| | MISO | 19 | |
| | MOSI | 23 | |
| | CS | 4 | active low |
| | RST | 27 | active low |
| | VCC | 3.3V | **3.3V only** — both parts are 3.3V, no shifting needed |
| DS1307 RTC (Wire) | SDA | 21 | see the voltage warning below |
| | SCL | 22 | |
| DHT11 | DATA | 25 | 4.7k–10k pull-up to 3.3V |
| HC-SR501 PIR | OUT | 26 | 5V supply, 3.3V TTL output — safe direct |
| Green LED (granted) | anode | 32 | series resistor |
| Red LED (denied) | anode | 33 | series resistor |
| Yellow LED (motion) | anode | 14 | series resistor |
| Buzzer | + | 13 | drive via transistor above ~20 mA |
| Wi-Fi | — | — | **on-chip** — no ESP8266, no UART |
| — | GND | GND | common with every peripheral |

Strapping pins (0, 2, 5, 12, 15) are deliberately unused so an attached LED or pull-up cannot change the
boot mode. This is the ESP32 counterpart to the `PB2`/`BOOT1` caveat in the STM32 port.

> ⚠️ **Never back-feed 5V into an ESP32 GPIO.** Unlike the Uno, this part is 3.3V and its pins are not
> 5V-tolerant.

## What differs from the STM32 F446RE

| | STM32 F446RE | ESP32 |
|---|---|---|
| Networking | external ESP8266, AT commands over USART2 | **on-chip Wi-Fi**, `WiFi.h` + `HTTPClient` — the entire AT layer is gone |
| Upload result | fire-and-forget into a UART | a real HTTP status code |
| RTC | DS1307 at 5V | DS1307 is **out of spec at 3.3V** — see below |
| PIR interrupt | CubeMX EXTI + NVIC | `attachInterrupt()`, handler must be `IRAM_ATTR` |
| µs delays | TIM1 counter | `delayMicroseconds()`, **not** interrupt-safe |
| DHT11 timing | loop owns the CPU | Wi-Fi shares the core — needs a critical section |
| Logic level | 3.3V | 3.3V (same) |

> ⚠️ **DS1307 voltage.** The DS1307 is specified 4.5–5.5V. At 3.3V it may run but drifts, and its
> battery-backup threshold shifts. Three options, best first: drop it and use SNTP (this board has Wi-Fi);
> fit a **DS3231** (2.3–5.5V, register-compatible for the seven registers this driver touches, so no code
> change); or keep it at 5V behind a bidirectional I2C level shifter. `include/rtc.h` spells this out.

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
| `src/pir.cpp` | `attachInterrupt()` only; the latch is already real |
| `src/dht11.cpp` | the 40-bit exchange, plus the `portENTER_CRITICAL` wrapper the ESP32 needs |
| `src/rtc.cpp` | `Wire` transfers; BCD conversion and `rtc_timestamp()` are already real |
| `src/wifi.cpp` | `WiFi.begin`, `HTTPClient` POST — a replacement for the AT-command layer, not a port of it |

Also outstanding, and flagged in the code:

- [ ] Replace the placeholder UIDs in `src/main.cpp`. `0xDEADBEEF` in a shipped access-control table is an
      open door.
- [ ] Move the Wi-Fi credentials out of `include/wifi.h` (build flag, NVS, or WiFiManager).
- [ ] Switch the log upload to HTTPS and escape the payload with a real JSON serializer — a card UID is
      attacker-influenced input.
- [ ] Buffer failed uploads. An audit trail that silently drops events during an outage is not an audit
      trail.
- [ ] Verify the RC522 BCC byte in `rfid_read_uid()`. The STM32 driver skips it; on an access-control path a
      corrupted read that happens to match a real card is a genuine risk.
- [ ] Replace the blocking `delay()` holds (3 s grant, 1 s deny, 2 s motion, 2 s debounce) with non-blocking
      timers. Same limitation as the STM32 port: scans and PIR events are missed during them.
