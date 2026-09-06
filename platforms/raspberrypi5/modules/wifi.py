"""
============================================================
wifi.py — event log upload (Raspberry Pi 5)
============================================================
STATUS: skeleton. The network ALREADY EXISTS (unlike the Uno, and
unlike the STM32's external ESP8266), so `connected()` is true as a
fact of Linux; the actual POST body is TODO and currently prints what
it would send instead of sending it.

This is where the Pi and the ESP32 agree and differ from the other
two: the network is on-board and a log upload is a plain HTTP request,
not an AT-command dance. `send_log()` can report real success or
failure (an HTTP status code), where the STM32 version fires blind
into a UART.

Honest no-upload note: until the TODO is filled in this function
prints the line it would POST to stderr and returns False. The STM32
and Uno "fire and forget" — a skeleton on the Pi must not *pretend*
an audit line left the machine. main.py logs the failure plainly.
============================================================
"""

import sys

# Where event logs are POSTed. Plain HTTP — see the TODO in send_log().
WIFI_LOG_URL = "http://192.168.1.100:8080/api/logs"

# How long a connection attempt is allowed before giving up.
WIFI_CONNECT_TIMEOUT_S = 15

_online = True  # Linux network stack is up; True unless the wire is gone


def init() -> bool:
    """Raises the interface (normally already up)."""
    # TODO: use urllib to probe WIFI_LOG_URL and cache reachability.
    #       A monitoring node that refuses to watch the door because
    #       the network is down is worse than one that logs locally —
    #       keep the STM32 port's non-fatal behaviour.
    print(f"TODO: wifi.init() -> probe {WIFI_LOG_URL}")
    print("      (the Pi's network stack is present; upload body is a stub)")
    return True


def connected() -> bool:
    """
    True while the network is reachable.

    Real by construction on Linux — the kernel maintains the link —
    with one honest caveat: it reports "interface up", not "server
    reachable". A real implementation should resolve WIFI_LOG_URL's
    host or open a socket, and return the pipe state rather than the
    link state. (The STM32's AT+CIPSTATUS equivalent.)
    """
    return _online


def send_log(msg) -> bool:
    """
    Upload one audit line and return True if the server accepted it.

    TODO: real body:
      from urllib.request import Request, urlopen
      body = json.dumps({"ts": <ISO8601>, "line": msg}).encode()
      req = Request(WIFI_LOG_URL, data=body,
                    headers={"Content-Type": "application/json"})
      try:
          with urlopen(req, timeout=WIFI_CONNECT_TIMEOUT_S) as r:
              return 200 <= r.status < 300
      except OSError:
          return False

    Remember: msg's UID field is attacker-influenced input (a card UID
    read off a radio link). It should be JSON-escaped, not concatenated
    raw, and HTTPS should be used before this goes anywhere public.
    """
    if not connected():
        return False

    if msg is None:
        return False

    # Print what the POST would carry, then honestly report no upload.
    print(f"[WIFI] would POST {len(msg.encode())} bytes to {WIFI_LOG_URL}",
          file=sys.stderr)
    print(f"[WIFI]   {msg}", file=sys.stderr)
    return False