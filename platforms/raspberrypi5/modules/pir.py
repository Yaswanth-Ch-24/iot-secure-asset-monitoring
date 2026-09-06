"""
============================================================
pir.py — HC-SR501 motion detection (Raspberry Pi 5)
============================================================
STATUS: skeleton. The latched read-and-clear semantics are real; the
GPIO edge is TODO and currently replaced by a scripted "motion" token
on stdin so the alert path in main.py is testable.

Replaces the STM32 port's EXTI interrupt on PC0. On the Pi a real
implementation differs in one way and one way only:

  - Attachment. The F446RE and the Uno map the PIR to an interrupt
    line; Linux has no per-pin interrupt API, so gpiozero's Button
    with when_activated (debounced by bounce_time) is the equivalent.
    See the TODO in init().

  - Latching stays identical. The edge sets a flag and
    motion_detected() returns it and clears it, exactly as
    HAL_GPIO_EXTI_Callback did — a caller never misses an event and
    never sees one twice, and that behaviour is the same code logging
    depends on, so it is implemented here rather than stubbed.

Scripted stub input (whitespace-separated tokens on stdin, owned by
modules/rfid.py):
    motion / m    raise one PIR motion event
============================================================
"""

# ── HC-SR501 OUT pin, BCM numbering ────────────────────────────
# The module needs a 5V supply and swings its OUT 0/3.3V. On a Pi
# that is safe direct — 3.3V is the Pi's own logic-high level, not a
# hazard. The 5V SUPPLY line must never touch a GPIO; only GND and
# OUT connect to the header.
PIR_PIN = 23

# Software floor on top of the module's own retrigger window (2-300 s,
# set by the on-board pot), matching PIR_DEBOUNCE_MS on the other
# ports. gpiozero's bounce_time (seconds) is the direct equivalent.
PIR_DEBOUNCE_MS = 2000

# Latch — mirrors `static volatile uint8_t motionFlag` in main.c.
# Set from an edge callback, read-and-cleared from the main loop.
_flag = False


def _on_edge() -> None:
    """Rising-edge callback — the STM32 HAL_GPIO_EXTI_Callback."""
    global _flag
    _flag = True


def init() -> None:
    """Set the pin as an input and arm the edge callback."""
    # TODO: from gpiozero import Button
    # TODO: _pir = Button(PIR_PIN, pull_up=False, bounce_time=2.0)
    #       _pir.when_activated = _on_edge
    #
    # Pull-up is NOT used: the HC-SR501 drives OUT push-pull, so the
    # pin must float to whatever level the module asserts — a pull-up
    # fights the module's pull-down while it is idle. The STM32 port
    # configured a plain floating input for exactly this reason.
    #
    # gpiozero's when_activated runs on a background thread; the flag
    # set there is plain Python (no volatile needed) because the main
    # loop's read-and-clear is atomic under the GIL.
    #
    # The module takes roughly 60 s after power-up to settle; expect
    # spurious triggers before then.
    print(f"TODO: pir.init() -> arm GPIO{PIR_PIN} edge (bounce 2 s)")
    print("      (falling back to scripted stdin events for now)")


def motion_detected() -> bool:
    """
    True exactly once per motion event. Read-and-clear.

    The stub first drains any pending scripted "motion" tokens (queued
    by modules/rfid.py's stdin parser) into the latch before clearing
    it, so the one-shot semantics hold for scripted runs too. In a
    real run those tokens never exist and the flag comes only from
    _on_edge().
    """
    global _flag

    from . import rfid  # scripted-event bridge, stub runs only

    while rfid.queued_motion() > 0:
        rfid._clear_motion_tokens()  # drain one scripted edge each
        _flag = True

    # Read-and-clear. Atomic: no await point between the read and the
    # clear, and _on_edge() cannot interleave under the GIL.
    triggered = _flag
    _flag = False
    return triggered


def cleanup() -> None:
    """Release the GPIO claim. Safe to call when init() never ran."""
    # TODO: _pir.close()
    print("TODO: pir.cleanup() -> release GPIO")