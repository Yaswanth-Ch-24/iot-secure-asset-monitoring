"""
IoT Secure Asset Monitoring hardware modules for the Raspberry Pi 5.

Every module exposes the same contract used by the Arduino Uno, ESP32
and STM32 F446RE ports:

    rfid  init / card_present / read_uid
    pir   init / motion_detected
    dht11 init / read
    rtc   init / get_time / set_time / timestamp
    wifi  init / connected / send_log

`rtc` is the one module implemented for real on this port: the Pi's
SoC and OS already keep time (the DS1307 is unnecessary), so
`timestamp()` reads the system clock. The rest are stubs marked TODO
that print instead of touching hardware, which is what lets main.py
run on a desktop with nothing installed.

The safe-state functions are NOT stubs: `wifi.send_log()` returns
False when the upload body is a stub, and main.py never pretends an
audit line left the machine. A skeleton that reports "GRANTED, log
sent" when it did nothing is a false audit record — worse than no
record at all.

Scripted stub input is owned by `rfid` and read from stdin:

    card / c    present an authorized card (the first UID in main.py)
    bad  / b    present a card that is not authorized
    motion / m  raise a PIR motion event
    q / quit    end the run
"""

__all__ = ["rfid", "pir", "dht11", "rtc", "wifi"]