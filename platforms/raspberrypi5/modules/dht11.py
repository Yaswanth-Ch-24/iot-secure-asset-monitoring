"""
============================================================
dht11.py — temperature and humidity, one-wire bit-bang (Raspberry Pi 5)
============================================================
STATUS: skeleton. The synthetic readings returned here exist only so
the 5-second environmental poll in main.py is testable before any
wiring; the actual GPIO read is TODO.

Replaces the STM32 port's `dht11` module. Same 40-bit frame: an 18 ms
low start pulse, the sensor's 80/80 us response, then 40 bits where
the high-phase width encodes the value, and byte 4 is a checksum.

The Pi is the trickiest of the four platforms to read a DHT11 from:

  - No MCU-style busy-wait. The F446RE and Uno disable interrupts and
    count microseconds; Linux cannot, so a real implementation uses
    pigpio's precise edge timestamps (pi.read_4_data / pi.gpio_trigger
    + a gpioCallback), which sample microsecond edges from the
    hardware. That is NOT a bit-bang loop — do not try one in Python.

  - Voltage. The DHT11 is a 3.0-5.5V part but its data line can swing
    to its supply rail. On this board, run the sensor at 3.3V and pull
    the data line up to 3.3V too, so nothing ever sees 5V. The Uno
    port (5V board) and the STM32 port (5V-tolerant pins) connect it
    directly; do not copy their 5V wiring here.
============================================================
"""

import random
import time

# ── DHT11 DATA pin, BCM numbering ──────────────────────────────
# Needs a 4.7k-10k pull-up to 3.3V. See the voltage note above: on a
# Pi, power the sensor from 3.3V (it is in spec) so the data line can
# never exceed 3.3V.
DHT11_PIN = 26

# The DHT11 cannot be read faster than once per second.
_DHT11_MIN_INTERVAL_S = 1.0

# Synthetic envelope the stub stays inside. On a real DHT11 the fields
# are whole numbers, no tenths — the DHT11 has no fractional digits
# (that is a DHT22 feature). These are placeholders to test the poll
# loop, not plausible sensor telemetry.
_MIN_TEMP, _MAX_TEMP = 24, 32
_MIN_HUM, _MAX_HUM = 45, 70

_last_read = 0.0
_last_result = (0, 0, False)


def init() -> None:
    """Park the data line as a released input (external pull-up)."""
    # TODO: set the pin to GPIO.IN with pull_up=False on a pigpio
    #       connection (the external 4.7k-10k resistor is the pull-up;
    #       an internal one fights nothing but muddies the timing).
    print(f"TODO: dht11.init() -> GPIO{DHT11_PIN} input, pull-up off")
    print("      3.3V supply + 3.3V pull-up (never 5V on this board)")
    print("      (returning synthetic readings for now)")


def read() -> tuple[int, int, bool]:
    """One 40-bit exchange. Returns (temperature, humidity, valid)."""
    global _last_read, _last_result

    # Real rate limit, matching DHT11_MIN_INTERVAL_MS on the other
    # ports. The poll loop in main.py runs at 5 s, well inside it.
    if time.monotonic() - _last_read < _DHT11_MIN_INTERVAL_S:
        return _last_result
    _last_read = time.monotonic()

    # TODO: real read with pigpio:
    #   pi.set_mode(DHT11_PIN, pigpio.OUTPUT)
    #   pi.set_pull_up_down(DHT11_PIN, pigpio.PUD_OFF)
    #   pi.gpio_trigger(DHT11_PIN, 18000, 0)      # 18 ms start pulse
    #   ...collect ~ 25 us-accurate edge times, decode the 40 bits
    #        from the high-phase widths, and verify byte 4 ==
    #        (b0 + b1 + b2 + b3) & 0xFF. And then return (temp, hum,
    #        True) with whole-degree values, exactly like DHT11_Read().
    #
    # Critical-section note for the stub reader: the DHT11 must be
    # polled alone while a read is in progress; on the Pi nothing else
    # is bit-banging the same pin, and a pigpio callback decodes the
    # frame without any noInterrupts() equivalent. That is the point
    # of using pigpio instead of spinning.

    temp = random.randint(_MIN_TEMP, _MAX_TEMP)
    hum = random.randint(_MIN_HUM, _MAX_HUM)
    _last_result = (temp, hum, True)
    return _last_result