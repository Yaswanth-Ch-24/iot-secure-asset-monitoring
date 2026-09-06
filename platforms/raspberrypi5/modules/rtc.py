"""
============================================================
rtc.py — real-time clock (Raspberry Pi 5)
============================================================
STATUS: implemented for real. The Pi has no DS1307 and needs none —
the SoC and the OS keep time, normally synchronised by NTP. The other
three ports carry a DS1307 or need one; here the module reads the
system clock and get_time()/timestamp() work as-is on any machine.

The DS1307 exists on the F446RE/Uno because those MCUs have no clock
that survives a reset. On the Pi the equivalent is the HWRTC + NTP:
a DS1307 would be a strict downgrade, and soldering one on at 3.3V
(the DS1307 is specified 4.5-5.5V) would be an out-of-spec hack. So
unlike the STM32/ESP32 ports, this module's contract is honest about
what "real time" means here.

set_time() is a guarded no-op for a stub run: system time is owned by
the OS (timedatectl) and changing it needs privileges, so this driver
does not pretend to own the clock. The one-shot set-and-comment-out
pattern from the other ports has no analogue on this platform.
============================================================
"""

import datetime


class RtcTime:
    """Wall-clock reading, mirroring the STM32 DS1307_Time struct."""

    __slots__ = ("seconds", "minutes", "hours", "day", "date", "month", "year")

    def __init__(self, seconds, minutes, hours, day, date, month, year):
        self.seconds = seconds
        self.minutes = minutes
        self.hours = hours        # 24-hour
        self.day = day            # 1 = Monday ... 7 = Sunday -> dt.weekday() + 1
        self.date = date
        self.month = month
        self.year = year          # last two digits, e.g. 26 for 2026


def init() -> bool:
    """
    "Join the bus" is a no-op — there is no bus. The SoC's clock is
    already running; this returns True so main.py's init order is the
    same on every platform.
    """
    # TODO (optional): verify the system clock is actually sane, e.g.
    #   timedatectl → "System clock synchronized: yes". A board that
    #   boots before NTP has a year-2026-epoch clock and will stamp
    #   audit lines with wrong time until it syncs.
    print("rtc.init() -> using system clock (HWRTC + NTP), no DS1307")
    return True


def get_time() -> RtcTime:
    """Read the current wall clock. Always succeeds."""
    now = datetime.datetime.now()
    return RtcTime(
        seconds=now.second,
        minutes=now.minute,
        hours=now.hour,
        day=now.isoweekday(),  # Mon=1 ... Sun=7, same as DS1307 day field
        date=now.day,
        month=now.month,
        year=now.year % 100,
    )


def set_time(t) -> bool:
    """
    Set the clock. Deliberately a no-op on this platform.

    The Pi gets time from NTP by design; a DS1307-style one-shot write
    would either need root and fight the OS, or silently do nothing.
    Set the system clock out-of-band (`sudo timedatectl set-time`).
    """
    # TODO: if you really must write the system clock from here:
    #   import subprocess; subprocess.run(["sudo", "timedatectl",
    #       "set-time", f"{2000+t.year}-{t.month:02d}-{t.date:02d} "
    #                    f"{t.hours:02d}:{t.minutes:02d}:{t.seconds:02d}"])
    print("rtc.set_time() -> no-op (system clock / NTP owns the time)")
    return False


def timestamp() -> str:
    """
    Format the current time as "[20YY-MM-DD HH:MM:SS] ".

    Implemented for real on every platform — it is pure string work.
    Unlike the MCU ports there is no dead-RTC fallback string here:
    the fallback exists to stop a silent clock loss being reported as
    a plausible timestamp on a security log, and a Linux kernel clock
    that "cannot be read" is not a recoverable condition anyway.
    """
    return f"[{datetime.datetime.now():%Y-%m-%d %H:%M:%S}] "