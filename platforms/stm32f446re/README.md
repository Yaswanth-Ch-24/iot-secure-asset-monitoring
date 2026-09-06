# IoT Secure Asset Monitoring — STM32 F446RE Port

**Status:** ✅ native platform — this is the original, complete firmware. Nothing here is a stub.

This is the reference implementation every other port mirrors. The F446RE is 3.3V native, which matters:
the RC522 (3.3V part) connects directly with no level shifting, while the DS1307 and HC-SR501 want a 5V
supply. The other three ports diverge from this one wherever the peripheral API or the logic level forces
them to — each divergence is called out in that port's own comments and in the root README.

## Layout

```text
Core/
├── Inc/{main.h, rc522.h, dht11.h, ds1307.h, access_control.h}
└── Src/{main.c, rc522.c, dht11.c, ds1307.c, access_control.c}
```

## Module contract

The ESP32, Arduino Uno and Raspberry Pi 5 ports expose a deliberately uniform contract so their entry
points read near-identically. This native port predates that naming, so the mapping is:

| Portable module | Native equivalent | Functions on the ports |
|---|---|---|
| `rfid` | `rc522.c` | `init` `card_present` `read_uid` |
| `pir` | (inline EXTI callback in `main.c`) | `init` `motion_detected` |
| `dht11` | `dht11.c` | `init` `read` |
| `rtc` | `ds1307.c` | `init` `get_time` `set_time` `timestamp` |
| `wifi` | `WiFi_SendLog()` in `main.c` | `init` `connected` `send_log` |

`access_control.c` is **application logic**, not a driver — the authorized-UID table, the grant/deny
decision and the LED/buzzer indicator sequences. On the three new ports that logic lives directly in the
entry point, which is why they have five modules and no `access_control` file.

## Wiring (NUCLEO-F446RE)

| Peripheral | Signal | MCU pin | Nucleo label | Notes |
|---|---|---|---|---|
| RC522 RFID (SPI1) | SDA / CS | PA4 | CN7-17 | GPIO output, active low |
| | SCK | PA5 | CN10-11 (D13) | SPI1_SCK, AF5 |
| | MISO | PA6 | CN10-13 (D12) | SPI1_MISO, AF5 |
| | MOSI | PA7 | CN10-15 (D11) | SPI1_MOSI, AF5 |
| | RST | PC7 | CN10-19 (D9) | GPIO output |
| | VCC | 3.3V | CN6-4 | **3.3V only** — 5V destroys the MFRC522 |
| DS1307 RTC (I2C1) | SDA | PB7 | CN7-21 | AF4, open-drain |
| | SCL | PB6 | CN10-17 | AF4, open-drain |
| | VCC | 5V | CN6-5 | DS1307 is specified 4.5–5.5V |
| DHT11 | DATA | PA9 | CN10-21 (D8) | bit-bang, pull-up on release |
| | VCC | 3.3V | CN6-4 | 3.0–5.5V part |
| HC-SR501 PIR | OUT | PC0 | CN7-35 | EXTI rising edge |
| | VCC | 5V | CN6-5 | 5V supply, 3.3V TTL output |
| ESP8266 (USART2) | ESP TX → MCU RX | PA3 | CN10-37 | AF7 |
| | MCU TX → ESP RX | PA2 | CN10-35 | AF7 |
| | VCC | 3.3V | CN6-4 | needs ~300 mA burst headroom |
| Green LED (granted) | anode | PB0 | CN10-31 | series resistor |
| Red LED (denied) | anode | PB1 | CN10-24 | series resistor |
| Yellow LED (motion) | anode | PB2 | CN10-22 | series resistor |
| Buzzer | + | PC1 | CN7-36 | drive via transistor if > 20 mA |
| Debug UART (USART1) | MCU TX | PA9 | CN10-21 | see the pin-conflict note below |
| | MCU RX | PA10 | CN10-33 | |
| — | GND | GND | CN6-6 | all grounds common |

> ⚠️ **Pre-existing pin conflict, carried over unchanged:** `main.h` maps `DHT11_PIN` to **PA9**, which is
> also **USART1_TX** — the debug UART the firmware logs through. Both cannot own PA9. This is present in
> the original code and has been left exactly as-is rather than silently "fixed"; on real hardware move the
> DHT11 to a free pin (PA8 / PC8 are both unused here) or move the debug UART to USART6.

> ⚠️ **`PB2` is `BOOT1`** on this part. It works as a GPIO output once the chip is running, but a load
> pulling it high at reset changes the boot mode. The yellow LED is on PB2 in the original design; keep the
> series resistor high and do not add a pull-up.

## CubeMX setup

The committed `Core/` tree contains only the application code — the HAL itself is generated per-project and
is deliberately not committed. To get a build:

1. **Generate the project** in STM32CubeIDE / STM32CubeMX for **NUCLEO-F446RE**.
2. **Configure the peripherals:**
   - `SPI1` → Full-Duplex Master, 8-bit, CPOL low / CPHA 1 Edge, prescaler for ≤ 4 MHz (RC522 max 10 MHz)
   - `I2C1` → I2C, Standard Mode 100 kHz (PB6 = SCL, PB7 = SDA)
   - `USART1` → Asynchronous, **115200** baud, 8N1 — the debug terminal
   - `USART2` → Asynchronous, 115200 baud, 8N1 — the ESP8266 link
   - `TIM1` → Internal Clock, prescaler `89`, counter period `65535` — gives the 1 µs tick `delay_us()` needs
     at 90 MHz APB2; recheck the prescaler if you change the clock tree
   - `PC0` → `GPIO_EXTI0`, rising edge, pull-down, with **EXTI line0 interrupt enabled** in NVIC
   - `PA4`, `PC7`, `PB0`, `PB1`, `PB2`, `PC1` → `GPIO_Output`, push-pull, no pull, low speed
   - `PA9` → whichever function you resolved the conflict above in favour of
3. **Copy `Core/Inc` and `Core/Src`** over the generated ones, keeping CubeMX's `stm32f4xx_hal_conf.h`,
   `stm32f4xx_it.c` and the startup file.
4. **Merge `main.h`** — the committed one declares the handles and pin macros; append CubeMX's generated
   content rather than replacing it.
5. Build and flash with the on-board ST-Link, then open a terminal at **115200 baud**.

### Static analysis

The same invocation CI runs, from the repository root:

```bash
cppcheck --enable=warning,style,performance \
  --suppress=missingIncludeSystem --suppress=unusedFunction \
  --inline-suppr --error-exitcode=1 \
  -I platforms/stm32f446re/Core/Inc \
  platforms/stm32f446re/Core/Src/
```

## Safety

Two behaviours are load-bearing and should stay real on every port:

- **Indicators must reach a de-energised state.** `AccessControl_GrantAccess()` and
  `AccessControl_DenyAccess()` both drive their LED and the buzzer **low** on the way out, and the motion
  branch in `main.c` clears the yellow LED and buzzer after its 2-second alert. A stub that skips the
  turn-off leaves a buzzer screaming and a "granted" LED lit — that is a hazard, not a cosmetic bug.
- **`Error_Handler()` must not leave outputs driven.** See the TODO checklist below.

## TODO checklist

The application logic is complete. What is outstanding is integration and the two robustness gaps found
while porting:

- [ ] **Resolve the PA9 conflict** between `DHT11_PIN` and `USART1_TX` (see the warning above).
- [ ] **Generate the CubeMX project** and supply `SystemClock_Config()`, `MX_GPIO_Init()`, `MX_SPI1_Init()`,
      `MX_I2C1_Init()`, `MX_USART1_UART_Init()`, `MX_USART2_UART_Init()`, `MX_TIM1_Init()`. These are
      declared in `Core/Inc/main.h` and called from `main()`, but no definition is committed — the build
      will not link without them.
- [ ] **Make `Error_Handler()` safe.** It currently does `__disable_irq()` and spins, which freezes the
      buzzer and LEDs in whatever state they were in. Drive the buzzer and all three LEDs low *before*
      disabling interrupts.
- [ ] **Set the RTC once.** Uncomment the `DS1307_SetTime()` line in `main.c`, flash, then comment it back
      out — otherwise every reset rewinds the clock.
- [ ] **Populate `authorizedCards[]`** in `Core/Src/access_control.c` with real UIDs. The three entries
      there are placeholders, and `0xDEADBEEF` in a shipped access-control table is an open door.
- [ ] **Check the ESP8266 `AT+CIPSEND` reply.** `WiFi_SendLog()` fires the command and a fixed
      `HAL_Delay(100)` without reading the `>` prompt or the `SEND OK`, so a dropped log is silent.
- [ ] **Move the blocking delays out of the hot loop.** The grant (3 s), deny (1 s), motion (2 s) and
      debounce (2 s) paths all use `HAL_Delay()`, during which RFID scans and PIR events are missed.
      A non-blocking timer per indicator fixes it without changing behaviour.
