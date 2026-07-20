"""
IoT Secure Asset Management & Environmental Monitoring
PC Terminal Simulator — No hardware needed

Simulates the exact UART output of the STM32 F446RE system:
  - RFID card scans (authorized & unauthorized)
  - PIR motion detection
  - DHT11 temperature & humidity readings
  - Timestamped event log

Run: python simulate.py
"""

import time
import random
import sys
from datetime import datetime, timedelta

# ── ANSI Colors ───────────────────────────────────
GREEN  = "\033[92m"
RED    = "\033[91m"
YELLOW = "\033[93m"
CYAN   = "\033[96m"
WHITE  = "\033[97m"
DIM    = "\033[2m"
RESET  = "\033[0m"
BOLD   = "\033[1m"

# ── Authorized Card UIDs ──────────────────────────
AUTHORIZED_CARDS = {
    "A3 F2 B1 09": "Admin Card",
    "12 34 56 78": "User Card 1",
    "DE AD BE EF": "User Card 2",
}

# ── All test cards (authorized + unauthorized) ────
ALL_CARDS = [
    "A3 F2 B1 09",
    "12 34 56 78",
    "DE AD BE EF",
    "FF FF FF FF",   # Unauthorized
    "BA D0 CA FE",   # Unauthorized
]

def ts(dt=None):
    d = dt or datetime.now()
    return f"[{d.strftime('%Y-%m-%d %H:%M:%S')}]"

def print_slow(text, delay=0.008):
    for ch in text:
        sys.stdout.write(ch)
        sys.stdout.flush()
        time.sleep(delay)
    print()

def banner():
    print(f"\n{CYAN}{'='*45}{RESET}")
    print(f"{BOLD}{WHITE}  IoT Asset Management System{RESET}")
    print(f"{BOLD}{WHITE}  STM32 F446RE | Yaswanth Chlliboina{RESET}")
    print(f"{CYAN}{'='*45}{RESET}\n")

def env_reading(dt, temp=None, hum=None):
    temp = temp or random.randint(26, 32)
    hum  = hum  or random.randint(55, 75)
    print(f"{DIM}{ts(dt)} DHT11: Temp={temp}C  Humidity={hum}%{RESET}")
    return temp, hum

def rfid_scan(dt, uid):
    print(f"\n{WHITE}{ts(dt)} RFID scan detected...{RESET}")
    time.sleep(0.4)
    print(f"{WHITE}{ts(dt)} Card UID: {uid}{RESET}")
    time.sleep(0.3)

    if uid in AUTHORIZED_CARDS:
        label = AUTHORIZED_CARDS[uid]
        print(f"{GREEN}{BOLD}{ts(dt)} ACCESS GRANTED  >> Green LED ON  [{label}]{RESET}")
        print(f"{DIM}{ts(dt)} Log sent to server via ESP8266{RESET}")
        print(f"{GREEN}  ┌─────────────────────────┐{RESET}")
        print(f"{GREEN}  │  ✅  ACCESS GRANTED      │{RESET}")
        print(f"{GREEN}  │  {label:<23} │{RESET}")
        print(f"{GREEN}  └─────────────────────────┘{RESET}")
    else:
        print(f"{RED}{BOLD}{ts(dt)} ACCESS DENIED   >> Red LED ON >> Buzzer ON{RESET}")
        print(f"{DIM}{ts(dt)} Alert sent via ESP8266{RESET}")
        print(f"{RED}  ┌─────────────────────────┐{RESET}")
        print(f"{RED}  │  ❌  ACCESS DENIED       │{RESET}")
        print(f"{RED}  │  Unknown Card            │{RESET}")
        print(f"{RED}  └─────────────────────────┘{RESET}")

def motion_alert(dt):
    print(f"\n{YELLOW}{BOLD}{ts(dt)} PIR: MOTION DETECTED >> Yellow LED ON >> Buzzer ON{RESET}")
    print(f"{YELLOW}  ┌─────────────────────────┐{RESET}")
    print(f"{YELLOW}  │  ⚠️   MOTION DETECTED    │{RESET}")
    print(f"{YELLOW}  └─────────────────────────┘{RESET}")
    time.sleep(1)
    print(f"{DIM}{ts(dt)} Yellow LED OFF | Buzzer OFF{RESET}")

def run_simulation():
    banner()
    now = datetime.now()

    print(f"{ts(now)} System initialized\n")
    time.sleep(0.5)

    # ── Sequence of events ───────────────────────
    events = [
        ("env",    0,  None,         None),
        ("rfid",   3,  "A3 F2 B1 09", None),
        ("env",    8,  None,         None),
        ("rfid",   12, "FF FF FF FF", None),
        ("motion", 15, None,         None),
        ("rfid",   20, "12 34 56 78", None),
        ("env",    25, None,         None),
        ("rfid",   28, "BA D0 CA FE", None),
        ("rfid",   32, "DE AD BE EF", None),
        ("motion", 38, None,         None),
        ("env",    42, None,         None),
        ("rfid",   46, "A3 F2 B1 09", None),
    ]

    for ev_type, offset, uid, _ in events:
        dt = now + timedelta(minutes=offset)
        time.sleep(0.6)

        if ev_type == "env":
            env_reading(dt)
        elif ev_type == "rfid":
            rfid_scan(dt, uid)
        elif ev_type == "motion":
            motion_alert(dt)

    print(f"\n{CYAN}{'='*45}{RESET}")
    print(f"{BOLD}  Simulation complete.{RESET}")
    print(f"{DIM}  In real hardware, this runs continuously on STM32 F446RE.{RESET}")
    print(f"{CYAN}{'='*45}{RESET}\n")

if __name__ == "__main__":
    try:
        run_simulation()
    except KeyboardInterrupt:
        print(f"\n{DIM}Simulation stopped.{RESET}\n")
