# Quick Start Guide

## No hardware? Run the PC simulator

```bash
# Clone the repo
git clone https://github.com/Yaswanth-Ch-24/iot-secure-asset-monitoring.git
cd iot-secure-asset-monitoring

# Run simulator (Python 3.8+ required, no extra packages needed)
cd simulator
python simulate.py
```

The simulator produces the exact same UART output you would see
on a serial terminal connected to the STM32 F446RE Nucleo board at 115200 baud.

## With hardware (STM32 F446RE Nucleo)

1. Install [STM32CubeIDE](https://www.st.com/en/development-tools/stm32cubeide.html) — free
2. File → Import → General → Existing Projects into Workspace → select this folder
3. Connect Nucleo board via USB (ST-Link)
4. Click the Run button (F11) — flashes automatically
5. Open any serial terminal at **115200 baud** to see live output

## Pin connections

See the full wiring table in [README.md](../README.md#-pin-connections-stm32-f446re-nucleo)
