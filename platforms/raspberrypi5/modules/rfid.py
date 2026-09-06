"""
============================================================
rfid.py — RC522 MIFARE card reader over SPI (Raspberry Pi 5)
============================================================
STATUS: skeleton. The card-present / UID logic is TODO; the module
currently consumes a scripted event stream from stdin so the access
state machine in main.py is testable before any wiring exists.

Replaces the STM32 port's `rc522` module. The chip and the protocol
are identical — REQA to see a card, anti-collision SELECT to pull the
UID — and on the Pi the transport is the Linux SPI device
(/dev/spidev0.0) instead of HAL_SPI_Transmit / HAL_SPI_Receive. The
rc522.c register reads and writes transfer over almost unchanged.

The Pi is one of only two boards here that can drive the RC522
directly: both are 3.3V parts, so there is no level shifting (unlike
the 5V Uno port). The other direct-connect board is the ESP32.

Scripted stub input (whitespace-separated tokens on stdin):
    card / c        present an authorized card (UID set to main.py[0])
    bad  / b        present a card that is not authorized
    motion / m      raise one PIR motion event (drained by pir.py)
    q / quit        stop the run
============================================================
"""

# ── RC522 pins on the Pi GPIO header, BCM numbering ────────────
# SCLK/MOSI/MISO are fixed by the Pi's SPI0 controller. RST is a
# free GPIO; CS here is CE0, which the kernel gives spidev0.0.
RFID_SCLK_PIN = 11    # SPI0 SCLK
RFID_MISO_PIN = 9     # SPI0 MISO
RFID_MOSI_PIN = 10    # SPI0 MOSI
RFID_CS_PIN = 8       # SPI0 CE0, active low
RFID_RST_PIN = 25     # active low

# RC522 tops out at 10 MHz; plenty of margin at 4 MHz.
RFID_SPI_HZ = 4000000

# Longest UID this driver reports — matches the STM32 RC522_UID size.
RFID_UID_MAX = 10

# Presented by the stubs, in STM32 RC522_UID.order (big-endian).
_GRANTED_UID = (0xA3, 0xF2, 0xB1, 0x09)  # main.py's first card
_BAD_UID = (0x11, 0x22, 0x33, 0x44)

# Cards queued by the scripted input, consumed one per poll.
_pending_cards = []   # [bool]  True = authorized card is next
_motion_tokens = 0    # [int]   pending "motion" tokens for pir.py
_online = True        # False once stdin runs out or a quit token


def init() -> None:
    """Open /dev/spidev0.0, reset the reader, switch the antenna on."""
    # TODO: import spidev
    # TODO: _spi = spidev.SpiDev(); _spi.open(0, 0)
    #       _spi.max_speed_hz = RFID_SPI_HZ
    #       _spi.mode = 0b00
    #       and then the RC522 init sequence from rc522.c, with
    #       read_register/write_register as SPI xfer2() calls —
    #       address byte (reg << 1) & 0x7E, read adds the 0x80 bit.
    print(
        f"TODO: rfid.init() -> SPI0 on CS=GPIO{RFID_CS_PIN}, "
        f"RST=GPIO{RFID_RST_PIN}, {RFID_SPI_HZ/1_000_000:.0f} MHz"
    )
    print("      3.3V part on a 3.3V board — direct, no shifting")
    print("      (falling back to scripted stdin events for now)")


def card_present() -> bool:
    """
    True when a card is sitting in the RF field. Non-blocking.

    A real implementation does a REQA (send 0x26, read the ATQA) and
    returns True when the FIFO comes back non-empty. The stub drains
    its stdin queue, refilling the queue with a fresh script line only
    when it is truly empty, which is also what lets cards_online() go
    False at EOF.
    """
    global _online, _motion_tokens

    if _online and not _pending_cards:
        try:
            line = input()
        except EOFError:
            _online = False
            return False

        for token in line.split():
            key = token.lower()
            if key in ("card", "c"):
                _pending_cards.append(True)
            elif key in ("bad", "b"):
                _pending_cards.append(False)
            elif key in ("motion", "m"):
                _motion_tokens += 1
            elif key in ("q", "quit"):
                # "No more input" — same as EOF. Stop parsing this
                # line, but any cards already queued above it still
                # fire; the loop's cards_online() check ends the run.
                _online = False
                break
            else:
                print(f"  (ignoring unknown event token: {token!r})")

    # Pull the head of the queue for this shot, but only consume it if
    # the next card is actually read. A presentation with no read still
    # counts as a presentation — matching REQA, which does not clear
    # the FIFO's data.
    if _pending_cards:
        return True  # a card token is queued
    return False


def read_uid() -> tuple | None:
    """
    Anti-collision SELECT, returning (size, uid, sak).

    The stub reports whichever card the scripted token named. Port the
    anti-collision cascade level 1 from rc522.c to make it real.
    """
    if not _pending_cards:
        return None

    authorized = _pending_cards.pop(0)
    uid = _GRANTED_UID if authorized else _BAD_UID
    return (len(uid), uid, 0x08)  # sak 0x08 = 4-byte UID (MIFARE Classic)


def queued_motion() -> int:
    """
    Number of scripted PIR motion events still pending.

    The stdin stream belongs to this module, and the "motion" tokens
    in it describe a different peripheral, so they are queued here and
    drained by pir.motion_detected() — see that module. A real (GPIO)
    run never has them; this exists only so the scripted demo can
    raise PIR events without giving stdin to two modules.
    """
    return _motion_tokens


def _clear_motion_tokens() -> None:
    """pir.motion_detected() calls this after draining each event."""
    global _motion_tokens
    _motion_tokens -= 1


def cards_online() -> bool:
    """
    True while the reader can be polled.

    The Pi genuinely differs from the three microcontroller ports
    here. There the RC522 is wired in and this always returns True;
    here the stub is driven by stdin, which ends, so main.py's loop
    has a real termination condition and can reach its safe-state
    teardown.

    A real spidev implementation should return True unconditionally,
    or check that the SPI device is still open.
    """
    return _online


def cleanup() -> None:
    """Release the SPI device. Safe to call when init() never ran."""
    # TODO: _spi.close()
    print("TODO: rfid.cleanup() -> close /dev/spidev0.0")