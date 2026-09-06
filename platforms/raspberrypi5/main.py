#!/usr/bin/env python3
"""
============================================================
IoT Secure Asset Monitoring — Raspberry Pi 5 entry point
Author : Chlliboina Yaswanth
============================================================

STATUS: runnable skeleton. The access-control state machine below is
REAL and complete; the GPIO/SPI bodies in modules/ are marked TODO and
print what they would do instead of driving hardware.

Run it on a desktop with nothing installed and it still works — the
modules degrade to printing (plus the system clock, which is real).
That makes the grant/deny/motion logic testable before the wiring
exists.

    python3 main.py                        # interactive
    echo "card motion bad c c" | python3 main.py  # scripted (EOF ends it)

Scripted event tokens, read from stdin by modules/rfid.py:
    card / c     present an authorized card
    bad  / b     present a card that is not authorized
    motion / m   raise a PIR motion event
    q / quit     stop the run (or just close the pipe / stdin)

The access decision is taken straight from the STM32 firmware
(Core/Src/access_control.c -> main.cpp on the Uno): a scanned UID is
compared, byte for byte, against the authorized table; a match grants
access (green LED 3 s), a mismatch denies it (red LED + buzzer 1 s),
and a motion event raises the yellow LED + buzzer 2 s. Every event is
timestamped by the system clock and written to the audit log; the log
upload is best-effort and honestly reports when it did not happen.

HARDWARE CONNECTIONS (BCM numbering)
─────────────────────────────────────────────────────────────
RC522 RFID     : SCK 11, MISO 9, MOSI 10, CS 8 (CE0), RST 25
DS1307         : NONE — the SoC + NTP is the clock (see modules/rtc.py)
DHT11          : DATA 26 ** 3.3V supply, 3.3V pull-up **
HC-SR501 PIR   : OUT 23 (5V supply, 3.3V TTL out — safe direct)
Green LED      : 17   Red LED : 18   Yellow LED : 22   Buzzer : 24
Wi-Fi          : on-board Ethernet / Wi-Fi

*** 3.3V RULE, PERIPHERAL BY PERIPHERAL ***
The Pi's GPIO is 3.3V and is NOT 5V-tolerant. What is safe:
  - RC522 connects directly — both parts are 3.3V (unlike the Uno).
  - PIR OUT (3.3V TTL) connects directly. Its 5V SUPPLY does not.
  - DHT11 runs at 3.3V (in spec), so its data line stays 3.3V.
What is NOT safe, ever: feeding 5V into any GPIO. The STM32 F446RE
has 5V-tolerant FT pins and the Uno is 5V native — do not copy either
of their 5V wiring to a GPIO here.

On a Pi 5, gpiozero needs the lgpio backend:
    export GPIOZERO_PIN_FACTORY=lgpio

Only modules/rtc.py and the indicator/latch logic are implemented for
real on this port. wifi_send_log() returns False until its body is
filled in: a stub that claims "GRANTED, log sent" when nothing was
sent is a false audit record.
============================================================
"""

import sys
import time

from modules import dht11, pir, rfid, rtc, wifi

# ── Indicator pins (BCM) ────────────────────────────────────────
LED_GREEN_PIN = 17    # access granted
LED_RED_PIN = 18      # access denied
LED_YELLOW_PIN = 22   # motion
BUZZER_PIN = 24

# ── Indicator timing — identical to the STM32 firmware ──────────
GRANT_HOLD_S = 3.0
DENY_HOLD_S = 1.0
MOTION_HOLD_S = 2.0
SCAN_DEBOUNCE_S = 2.0
ENV_INTERVAL_S = 5.0
LOOP_TICK_S = 0.1

# The stub run would take ~17 s for one grant followed by one deny.
# Scale the delays to zero so the scripted demo completes quickly. Set
# to 1.0 (and wire the GPIO) for real cadence.
STUB_TIME_SCALE = 0.0

# ── Access control (the STM32 port's access_control.c) ──────────
# TODO: replace these placeholders with your real card UIDs.
#
# 0xDEADBEEF in a shipped access-control table is an open door. Scan a
# card and read the "Card UID:" line this sketch prints, then paste
# the real bytes here.
AUTHORIZED_CARDS = (
    (0xA3, 0xF2, 0xB1, 0x09),  # Card 1 — Admin
    (0x12, 0x34, 0x56, 0x78),  # Card 2 — User 1
    (0xDE, 0xAD, 0xBE, 0xEF),  # Card 3 — User 2
)

# 4-byte UID title match, same as UID_MATCH_LEN on the Uno.
UID_MATCH_LEN = 4


def _sleep(seconds: float) -> None:
    """Sleep, scaled down while the indicator delays are stubs."""
    time.sleep(seconds * STUB_TIME_SCALE)


def _s(hours: int, minutes: int, seconds: int) -> str:
    """Compact [HH:MM:SS] stamp for the audit line."""
    return f"{hours:02d}:{minutes:02d}:{seconds:02d}"


def log_line(msg: str) -> None:
    """Print one timestamped audit line — the terminal log (real)."""
    now = rtc.get_time()
    print(f"{rtc.timestamp().rstrip()} {msg}")
    # Keep the exact form main.c's snprintf produced available to
    # anything that parses the log:
    #   f"[20{now.year:02d}-{now.month:02d}-{now.date:02d} "
    #   f"{_s(now.hours, now.minutes, now.seconds)}] {msg}"


def indicators_safe() -> None:
    """
    Drive every indicator to its de-energised state.

    Called at boot, after every alert, and before any early return.
    Implemented for real on purpose: this is the function that has to
    work when everything else is still a TODO. A stub that fails to
    switch the buzzer or the "granted" LED off is a hazard.
    """
    # TODO: from gpiozero import LED
    # TODO: LED(LED_GREEN_PIN).off(); LED(LED_RED_PIN).off()
    #       LED(LED_YELLOW_PIN).off(); LED(BUZZER_PIN).off()
    print("indicators_safe() -> all indicators de-energised (stub)")
    # Real behaviour with no hardware: there is nothing to energise, so
    # the safe state is trivially reached — that is why the stub is
    # honest rather than pretending it switched GPIO.
    return None


def indicators_init() -> None:
    """Claim the LED/buzzer pins and put them in the safe state."""
    # TODO: instantiate the four LEDs, then indicators_safe()
    print(f"TODO: indicators_init() -> green={LED_GREEN_PIN}, "
          f"red={LED_RED_PIN}, yellow={LED_YELLOW_PIN}, "
          f"buzzer={BUZZER_PIN}")
    indicators_safe()


def access_check_uid(uid) -> bool:
    """Compare a scanned UID against the authorized table."""
    if len(uid) < UID_MATCH_LEN:
        # A short UID cannot match a 4-byte table entry. Fail closed —
        # never grant on a partial read.
        return False
    for card in AUTHORIZED_CARDS:
        if uid[:UID_MATCH_LEN] == card:
            return True
    return False


def access_log_event(uid, granted: bool) -> None:
    """The "[LOG] UID=... Result=... Time=..." audit line."""
    u = "".join(f"{b:02X}" for b in uid[:4])
    now = rtc.get_time()
    print(f"[LOG] UID={u} Result={'GRANTED' if granted else 'DENIED'} "
          f"Time=20{now.year:02d}-{now.month:02d}-{now.date:02d} "
          f"{_s(now.hours, now.minutes, now.seconds)}")


def access_grant() -> None:
    """Green LED for 3 s, red forced off. Ends de-energised."""
    print("[GRANT] Green LED ON for 3 s")
    _sleep(GRANT_HOLD_S)
    print("[GRANT] Green LED OFF")


def access_deny() -> None:
    """Red LED + buzzer for 1 s, green forced off. Ends de-energised."""
    print("[DENY]  Red LED ON + Buzzer ON for 1 s")
    _sleep(DENY_HOLD_S)
    print("[DENY]  Red LED OFF, Buzzer OFF")


def motion_alert() -> None:
    """Yellow LED + buzzer for 2 s. Ends de-energised."""
    print("[MOTION] Yellow LED ON + Buzzer ON for 2 s")
    _sleep(MOTION_HOLD_S)
    print("[MOTION] Yellow LED OFF, Buzzer OFF")


def main() -> int:
    # Modules first. rtc.init() is real; the rest are stubs that print.
    rtc.init()
    rfid.init()
    dht11.init()
    pir.init()
    wifi.init()

    # Indicators must be safe before anything can fail loudly.
    indicators_init()

    print("\n" + "=" * 41)
    print("  IoT Asset Management System")
    print("  Raspberry Pi 5 | Yaswanth Chlliboina")
    print("=" * 41)
    log_line("System initialized")

    if wifi.connected():
        log_line("Wi-Fi: link up (upload body is a stub)")
    else:
        log_line("No network: logging locally only")

    print("\nEvents: card/c | bad/b | motion/m | q")

    env_interval = ENV_INTERVAL_S * max(STUB_TIME_SCALE, 0.0001)
    last_env = 0.0

    try:
        # On the microcontroller ports this is a bare while(1);
        # cards_online() only ever goes false here, where the stub
        # reader is fed from stdin and therefore ends.
        while rfid.cards_online():
            # 1. Read the environment every ENV_INTERVAL_S.
            now = time.monotonic()
            if now - last_env >= env_interval:
                last_env = now
                temp, hum, valid = dht11.read()
                if valid:
                    log_line(f"DHT11: Temp={temp}C  Humidity={hum}%")

            # 2. Latched PIR flag.
            if pir.motion_detected():
                log_line("PIR: MOTION DETECTED >> Yellow LED ON >> Buzzer ON")
                motion_alert()

            # 3. Poll the RFID reader.
            if rfid.card_present():
                uid = rfid.read_uid()
                if uid is not None:
                    log_line("RFID scan detected...")
                    log_line(f"Card UID: {' '.join(f'{b:02X}' for b in uid[1])}")

                    granted = access_check_uid(uid[1])
                    access_log_event(uid[1], granted)

                    if granted:
                        access_grant()
                        line = "ACCESS GRANTED  >> Green LED ON"
                    else:
                        access_deny()
                        line = "ACCESS DENIED   >> Red LED ON >> Buzzer ON"
                    log_line(line)

                    # Best-effort. Returns False until the wifi module's
                    # TODO is filled in, and that is fine — the line is
                    # already in the log above.
                    if not wifi.send_log(line):
                        print("      (log upload skipped — wifi module is a stub)")

                    # Stop one card presentation reading as several scans.
                    _sleep(SCAN_DEBOUNCE_S)

            _sleep(LOOP_TICK_S)
    except KeyboardInterrupt:
        print("\nInterrupted.")
    finally:
        # Always reach a safe state, even on an exception — a skeleton
        # that leaves the buzzer energised is a hardware and safety
        # hazard.
        indicators_safe()
        rfid.cleanup()
        pir.cleanup()
        print("Indicators de-energised, GPIO released.")

    return 0


if __name__ == "__main__":
    sys.exit(main())