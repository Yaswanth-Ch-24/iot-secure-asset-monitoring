# IoT-Enabled Secure Asset Management & Environmental Monitoring System

> **Multi-platform: STM32 F446RE (reference) | Arduino Uno | ESP32 | Raspberry Pi 5 | 2026**
> RFID-based access control + PIR motion detection + DHT11 environmental sensing + DS1307 RTC timestamped logging over Wi-Fi — with a **common module contract** ported across four boards.

---

## 📌 Reference + Supported Ports

This repo contains a **reference** implementation on **STM32 F446RE (Nucleo-F446RE)** and three *ports* that keep a uniform module contract:

- **STM32 F446RE** (reference): bare-metal C + HAL + DS1307
- **Arduino Uno**: AVR C++ + PlatformIO + PROGMEM UID table + level-shifting notes
- **ESP32**: Arduino C++ + on-chip Wi‑Fi + DS1307 voltage caveat
- **Raspberry Pi 5**: Python 3 + real-time clock (system clock/NTP) + scripted SPI/PIR stubs

| MCU / Platform | What it represents |
|---|---|
| STM32 F446RE | original firmware + application logic reference |
| Arduino Uno | smallest-memory port + hardest wiring constraints |
| ESP32 | Wi‑Fi port with native networking |
| Raspberry Pi 5 | Linux-hosted simulator-friendly port |

---

## 🔍 Project Overview

A complete **IoT security and environmental monitoring gateway** — a reference implementation on STM32 F446RE with identical module contracts ported to Arduino Uno, ESP32, and Raspberry Pi 5. Every port exposes the same five modules (`rfid` / `pir` / `dht11` / `rtc` / `wifi`) with identical function names, so the application state machine reads almost identically across C++, Python, and bare-metal firmware. The STM32 F446RE port is the original; the three new ports diverge wherever the logic level, the peripheral API, or the networking stack forces them to.

---

## ⚡ Features

| Feature | Hardware | STM32 Interface |
|---|---|---|
| RFID Access Control | RC522 | SPI1 |
| Motion Detection | HC-SR501 PIR | GPIO (EXTI interrupt) |
| Temp + Humidity | DHT11 | GPIO (bit-bang) |
| Real-Time Clock | DS1307 | I2C1 |
| Wi-Fi / Serial Log | ESP8266 | USART2 |
| Status LEDs | 3x LEDs | GPIO Output |
| Buzzer Alert | Passive buzzer | GPIO Output |
| Debug Terminal | USB-UART | USART1 (ST-Link) |

---

## 🔌 Pin Connections (STM32 F446RE Nucleo)

### RC522 RFID — SPI1

| RC522 Pin | STM32 Pin | Nucleo Label |
|---|---|---|
| SDA (CS) | PA4 | CN7-17 |
| SCK | PA5 | CN10-11 (Arduino D13) |
| MOSI | PA7 | CN10-15 (Arduino D11) |
| MISO | PA6 | CN10-13 (Arduino D12) |
| RST | PC7 | CN10-19 (Arduino D9) |
| VCC | 3.3V | CN6-4 |
| GND | GND | CN6-6 |

### DS1307 RTC — I2C1

| DS1307 Pin | STM32 Pin | Nucleo Label |
|---|---|---|
| SDA | PB7 | CN7-21 |
| SCL | PB6 | CN10-17 |
| VCC | 5V | CN6-5 |
| GND | GND | CN6-6 |

### DHT11 Temperature/Humidity — GPIO

| DHT11 Pin | STM32 Pin | Nucleo Label |
|---|---|---|
| DATA | PA9 | CN10-21 (Arduino D8) |
| VCC | 3.3V | CN6-4 |
| GND | GND | CN6-6 |

### HC-SR501 PIR Motion — GPIO EXTI

| PIR Pin | STM32 Pin | Nucleo Label |
|---|---|---|
| OUT | PC0 | CN7-35 |
| VCC | 5V | CN6-5 |
| GND | GND | CN6-6 |

### ESP8266 Wi-Fi — USART2

| ESP8266 Pin | STM32 Pin | Nucleo Label |
|---|---|---|
| TX | PA3 (USART2_RX) | CN10-37 |
| RX | PA2 (USART2_TX) | CN10-35 |
| VCC | 3.3V | CN6-4 |
| GND | GND | CN6-6 |

### LEDs and Buzzer — GPIO Output

| Component | STM32 Pin | Nucleo Label |
|---|---|---|
| Green LED (Access OK) | PB0 | CN10-31 |
| Red LED (Access Denied) | PB1 | CN10-24 |
| Yellow LED (Motion) | PB2 | CN10-22 |
| Buzzer | PC1 | CN7-36 |

---

## 🔌 Wiring Guides

> Per-board wiring for **all four platforms** is documented in each port's README:
> [`platforms/stm32f446re/README.md`](platforms/stm32f446re/README.md) (reference), [`platforms/arduino-uno/README.md`](platforms/arduino-uno/README.md),
> [`platforms/esp32/README.md`](platforms/esp32/README.md), [`platforms/raspberrypi5/README.md`](platforms/raspberrypi5/README.md).
> See [docs/quick_start.md](docs/quick_start.md) for the no-hardware simulator.

## 🔗 Module Contract

Every port exposes the same five modules with the same function names, so the application
state machine reads almost identically across C, C++ and Python:

| Module | Functions | STM32 native | Purpose |
|---|---|---|---|
| `rfid` | `init` `card_present` `read_uid` | `rc522.c` | RC522 card reader over SPI |
| `pir` | `init` `motion_detected` | EXTI in `main.c` | HC-SR501 motion latch |
| `dht11` | `init` `read` | `dht11.c` | temperature + humidity |
| `rtc` | `init` `get_time` `set_time` `timestamp` | `ds1307.c` | timestamped audit lines |
| `wifi` | `init` `connected` `send_log` | `WiFi_SendLog()` | event log upload |

Access control (UID table, grant/deny decision, LED/buzzer sequences) is **application logic**
and lives in each port's entry point, matching the STM32 port's `access_control.c`.

---

## 📁 Repository Structure

```text
iot-secure-asset-monitoring/
├── platforms/
│   ├── stm32f446re/              # reference firmware (original, complete)
│   │   └── Core/{Inc,Src}        # main, rc522, dht11, ds1307, access_control
│   ├── arduino-uno/              # AVR C++ · PlatformIO · PROGMEM UID table
│   │   └── {include,src}/        # 5 headers + main.cpp + 5 drivers
│   ├── esp32/                    # Arduino C++ · on-chip Wi-Fi
│   │   └── {include,src}/        # 5 headers + main.cpp + 5 drivers
│   └── raspberrypi5/             # Python 3 · system clock/NTP · scripted stubs
│       ├── main.py               # access-control state machine
│       ├── modules/              # rfid, pir, dht11, rtc, wifi
│       └── requirements.txt
├── simulator/
│   └── simulate.py               # terminal simulator, no hardware needed
├── docs/
│   └── quick_start.md
└── README.md
```

---

## 🚀 How to Run

### Option A — Simulate on PC (No hardware needed)

```bash
cd simulator
python simulate.py
```

This runs a full terminal simulation — RFID scan, access grant/deny, motion alerts, and environmental readings.

### Option B — Flash to STM32 F446RE Nucleo

1. Install **STM32CubeIDE** from [st.com](https://www.st.com/en/development-tools/stm32cubeide.html)
2. Open STM32CubeIDE → File → Import → Existing Project → select this folder
3. Connect Nucleo board via USB
4. Click **Run** (F11) — ST-Link flashes automatically
5. Open Serial Monitor at **115200 baud** to see logs

### Option C — Run the other ports

| Port | Build / run |
|---|---|
| **Raspberry Pi 5** | `cd platforms/raspberrypi5 && python3 main.py` — scripted demo: `echo "card motion bad c c" \| python3 main.py` |
| **Arduino Uno** | `cd platforms/arduino-uno && pio run`, monitor at **9600** baud |
| **ESP32** | `cd platforms/esp32 && pio run`, monitor at **115200** baud |

Each port's README has its full wiring table, hardware notes and TODO list.

---

## 📊 Sample Output (UART Terminal @ 115200 baud)

```text
=========================================
  IoT Asset Management System
  STM32 F446RE | Yaswanth Chlliboina
=========================================
[2026-03-15 09:30:00] System initialized
[2026-03-15 09:30:01] DHT11: Temp=28C  Humidity=65%
[2026-03-15 09:32:14] RFID scan detected...
[2026-03-15 09:32:14] Card UID: A3 F2 B1 09
[2026-03-15 09:32:14] ACCESS GRANTED  >> Green LED ON
[2026-03-15 09:32:14] Log sent to server via ESP8266
[2026-03-15 09:45:02] PIR: MOTION DETECTED >> Yellow LED ON >> Buzzer ON
[2026-03-15 10:01:55] ACCESS DENIED   >> Red LED ON >> Buzzer ON
```

---

## 👤 Author

Chlliboina Yaswanth

B.Tech Electrical and Electronics Engineering | CGPA: 8.56

Dr. Lankapalli Bullayya College of Engineering, Visakhapatnam

- Email: [yaswanth2452005@gmail.com](mailto:yaswanth2452005@gmail.com)
- LinkedIn: [yaswanth-chlliboina](https://www.linkedin.com/in/yaswanth-chlliboina/)
- GitHub: [Yaswanth-Ch-24](https://github.com/Yaswanth-Ch-24)
