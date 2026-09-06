# IoT Secure Asset Monitoring — Raspberry Pi 5 Port

**Status:** 🚧 skeleton. The access-control state machine in `main.py` is complete; the GPIO/SPI bodies in
the modules are marked `TODO` and print what they would do instead of driving hardware.

It runs today on any machine with Python 3 — even a desktop with nothing installed — because the onboard
system clock (`modules/rtc.py`) and the whole authorization flow are real; only the sensor/indicator GPIO is
stubbed.

```bash
python3 main.py                         # interactive
echo "card motion bad c c" | python3 main.py     # scripted, ends at EOF
```

Scripted event tokens (read from stdin by `modules/rfid.py`):

| Token | What it fires |
|---|---|
| `card` / `c` | present an authorized card → **ACCESS GRANTED**, green LED 3 s |
| `bad` / `b` | present a non-authorized card → **ACCESS DENIED**, red LED + buzzer 1 s |
| `motion` / `m` | raise a PIR event → yellow LED + buzzer 2 s |
| `q` / `quit` | end the run (stands in for the MCU ports' `while(1)`) |

## Module contract

Identical to the Arduino Uno, ESP32 and STM32 F446RE ports, so `main.py` reads almost the same as
`src/main.cpp` and `Core/Src/main.c`:

| Module | Functions | Status |
|---|---|---|
| `rfid` | `init` `card_present` `read_uid` | stub (SPI via `spidev`) |
| `pir` | `init` `motion_detected` | latch real, edge TODO (gpiozero `Button`) |
| `dht11` | `init` `read` | stub (pigpio edge decode TODO) |
| `rtc` | `init` `get_time` `set_time` `timestamp` | **real** — system clock + NTP |
| `wifi` | `init` `connected` `send_log` | link real, upload body stub |

`rtc` being real is the point of this port: the Pi's SoC and OS already keep time, so the DS1307 every
other board carries is unnecessary — a strict downgrade, in fact.

## Wiring (BCM numbering)

| Peripheral | Signal | GPIO | Notes |
|---|---|---|---|
| RC522 RFID (SPI0) | SCK | 11 | **direct — both parts 3.3V**, no shifting (the Uno is the only port that needs it) |
| | MISO | 9 | |
| | MOSI | 10 | |
| | CS | 8 (CE0) | active low |
| | RST | 25 | active low |
| DHT11 | DATA | 26 | **3.3V supply + 4.7k–10k pull-up to 3.3V** — never 5V |
| HC-SR501 PIR | OUT | 23 | 5V supply, 3.3V TTL out — safe direct |
| Green LED (granted) | anode | 17 | series resistor |
| Red LED (denied) | anode | 18 | series resistor |
| Yellow LED (motion) | anode | 22 | series resistor |
| Buzzer | + | 24 | drive via transistor above ~20 mA |
| Wi-Fi | — | — | **on-board** — no ESP8266, no UART |
| Clock | — | — | **no DS1307** — SoC HWRTC + NTP |
| — | GND | GND | common with every peripheral |

> ⚠️ **The 3.3V rule.** The Pi's GPIO is 3.3V and is **not 5V-tolerant**. The STM32 F446RE has 5V-tolerant
> FT pins and the Uno is 5V native — neither of those wirings is safe to copy here. In particular the RC522
> is 3.3V on this board (direct), the PIR's **supply** line never touches a GPIO, and the DHT11 runs at
> 3.3V so its data line stays 3.3V. See the per-peripheral notes in the code.

## What differs from the STM32 F446RE

| | STM32 F446RE | Raspberry Pi 5 |
|---|---|---|
| Networking | external ESP8266, AT commands over USART2 | **on-board** Ethernet/Wi-Fi — the upload is a plain HTTP POST, no AT layer |
| Clock | DS1307 at 5V over I2C1 | **system clock + NTP** — no DS1307, and none is wanted |
| PIR interrupt | CubeMX EXTI + NVIC | gpiozero `Button.when_activated` — no per-pin interrupt API in Linux |
| DHT11 timing | TIM1 counter, interrupts disabled | pigpio edge timestamps — **no busy-wait is possible in userspace** |
| Logic level | 3.3V (5V-tolerant pins) | 3.3V (**not** 5V-tolerant — the RC522 connects direct here) |
| Main loop | `while(1)` forever | stdin-driven, so a scripted run can reach a clean teardown |
| UID table | RAM | Python tuple constant |

## Safety

`indicators_safe()` in `main.py` and every turn-off path in `access_grant()`, `access_deny()` and
`motion_alert()` are **implemented for real, not stubbed**, and `main.py`'s `finally:` block reaches that
safe state even on an exception. A skeleton that leaves the buzzer or a "granted" LED stuck energised is a
hazard, not a cosmetic bug. `wifi.send_log()` returns `False` until its upload body exists — a stub that
claimed "log sent" when nothing left the machine would be a false audit record, which is worse than no
record.

## Filling in the TODOs

| File | What is stubbed |
|---|---|
| `modules/rfid.py` | `spidev` open + reset, register read/write, REQA, anti-collision. Port from `platforms/stm32f446re/Core/Src/rc522.c` — the protocol is identical, only `xfer2()` replaces `HAL_SPI_Transmit` |
| `modules/pir.py` | gpiozero `Button` + `when_activated`; the read-and-clear latch is already real |
| `modules/dht11.py` | the 40-bit decode from pigpio edge timestamps; synthetic readings today |
| `modules/rtc.py` | `set_time()` no-op by design — the OS owns the clock; verify NTP sync instead |
| `modules/wifi.py` | the `urllib`/`requests` POST + JSON escaping + HTTPS |
| `main.py` | instantiate the gpiozero LEDs in `indicators_init()` |

Also outstanding, and flagged in the code:

- [ ] Replace the placeholder UIDs in `main.py`. `0xDEADBEEF` in a shipped access-control table is an open
      door.
- [ ] Wrap `wifi.send_log()`'s payload in a real JSON serializer — a card UID is attacker-influenced input.
- [ ] Buffer failed uploads. An audit trail that silently drops events during an outage is not an audit
      trail.
- [ ] Replace the blocking `time.sleep()` holds (3 s grant, 1 s deny, 2 s motion, 2 s debounce — scaled to
      zero in `STUB_TIME_SCALE`) with non-blocking deadlines. Same limitation as the MCU ports: scans and
      PIR events are missed during them.

## Top-level layout

```
platforms/raspberrypi5/
├── main.py         — access-control state machine (mirrors main.c / main.cpp)
├── modules/
│   ├── __init__.py  rfid.py  pir.py  dht11.py  rtc.py  wifi.py
├── requirements.txt  — gpiozero, lgpio, spidev, pigpio (none needed to run the stub)
└── README.md
```