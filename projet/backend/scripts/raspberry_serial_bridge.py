#!/usr/bin/env python3
"""
Pont série Arduino (AS608) <-> Backend C++ Drink Manager sur Raspberry Pi.
Lit les trames série (ex: USER:1, AUTH_FAIL), interroge l'API C++ locale
(/api/auth/biometric) et renvoie la commande CODE:XXXXXX\n vers l'Arduino.
"""

import argparse
import json
import time
import urllib.error
import urllib.request
import serial


def call_biometric_api(api_url: str, line: str) -> dict | None:
    payload = json.dumps({"serial_line": line}).encode("utf-8")
    req = urllib.request.Request(
        f"{api_url}/auth/biometric",
        data=payload,
        headers={"Content-Type": "application/json"},
        method="POST",
    )
    try:
        with urllib.request.urlopen(req, timeout=3) as resp:
            return json.loads(resp.read().decode("utf-8"))
    except urllib.error.HTTPError as err:
        print(f"[API] Rejet ({err.code}): {err.read().decode('utf-8', errors='ignore')}")
        return None
    except Exception as exc:
        print(f"[API] Erreur connexion: {exc}")
        return None


def main() -> None:
    parser = argparse.ArgumentParser(description="Pont série Drink Manager (Raspberry Pi)")
    parser.add_argument("--port", default="/dev/ttyACM0", help="Port série Arduino (ex: /dev/ttyACM0 ou COM3)")
    parser.add_argument("--baud", type=int, default=9600, help="Baudrate (défaut: 9600)")
    parser.add_argument("--api", default="http://127.0.0.1:8080/api", help="URL de l'API C++")
    args = parser.parse_args()

    print(f"[INFO] Ouverture du port {args.port} à {args.baud} bauds...")
    while True:
        try:
            with serial.Serial(args.port, args.baud, timeout=1) as ser:
                time.sleep(2)  # Laisse l'Arduino redémarrer après ouverture série
                print("[INFO] Liaison série prête. En attente d'empreinte...")
                while True:
                    raw = ser.readline().decode("utf-8", errors="ignore").strip()
                    if not raw:
                        continue
                    print(f"[ARDUINO -> PI] {raw}")
                    if raw.startswith("USER:"):
                        res = call_biometric_api(args.api, raw)
                        if res and "serial_command" in res:
                            cmd = res["serial_command"]
                            ser.write(cmd.encode("utf-8"))
                            print(f"[PI -> ARDUINO] {cmd.strip()} (utilisateur: {res['user']['name']})")
                    elif raw == "AUTH_FAIL":
                        ser.write(b"CLEAR\n")
                        print("[INFO] Empreinte non reconnue.")
        except serial.SerialException as exc:
            print(f"[WARN] Port série indisponible ({exc}). Nouvelle tentative dans 3s...")
            time.sleep(3)


if __name__ == "__main__":
    main()
