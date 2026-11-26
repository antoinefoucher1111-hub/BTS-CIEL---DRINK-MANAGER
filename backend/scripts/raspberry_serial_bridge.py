#!/usr/bin/env python3
"""
Pont serie Arduino (AS608 + LCD) <-> Backend C++ Drink Manager sur Raspberry Pi.

Flux complet :
1. L'utilisateur pose son doigt sur le capteur AS608 branche a l'Arduino.
2. L'Arduino envoie l'ID d'empreinte sur le port serie :
   - Format LCD actuel : "1\n" (entier seul) -> le script renvoie "XXXXXX\n" (6 chiffres)
   - Format protocole  : "USER:1\n"         -> le script renvoie "CODE:XXXXXX\n"
3. Ce script appelle le backend C++ local (POST /api/auth/biometric).
4. Le backend C++ trouve le compte SQLite associe a cette empreinte, genere un code
   a 6 chiffres valable 2 minutes (usage unique) et le renvoie a ce script.
5. Ce script renvoie les 6 chiffres a l'Arduino, qui les affiche sur l'ecran LCD.
6. L'utilisateur saisit ces 6 chiffres sur la page web -> connecte a son compte !
"""

import argparse
import json
import sys
import time
from typing import Optional
import urllib.error
import urllib.request

try:
    import serial
except ImportError:
    serial = None


def check_backend_health(api_url: str) -> bool:
    try:
        with urllib.request.urlopen(f"{api_url}/health", timeout=3) as resp:
            data = json.loads(resp.read().decode("utf-8"))
            print(f"[API] Backend C++ joignable : {data.get('status')} (BDD: {data.get('database')})")
            return True
    except Exception as exc:
        print(f"[WARN] Backend C++ non joignable sur {api_url} ({exc})")
        print("[WARN] Assurez-vous de lancer ./drink_manager avant de scanner une empreinte.")
        return False


def call_biometric_api(api_url: str, *, fingerprint_id: Optional[int] = None, serial_line: Optional[str] = None) -> Optional[dict]:
    if fingerprint_id is not None:
        body = {"fingerprint_id": fingerprint_id}
    elif serial_line is not None:
        body = {"serial_line": serial_line}
    else:
        return None

    payload = json.dumps(body).encode("utf-8")
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
        detail = err.read().decode("utf-8", errors="ignore")
        print(f"[API] Rejet ({err.code}): {detail}")
        return None
    except Exception as exc:
        print(f"[API] Erreur connexion au backend C++: {exc}")
        return None


def handle_serial_line(raw: str, api_url: str) -> Optional[bytes]:
    """Traite une ligne recue de l'Arduino et retourne les octets a renvoyer, ou None."""
    if raw.isdigit():
        finger_id = int(raw)
        print(f"\n[+] Empreinte detectee ! ID recu de l'Arduino : {finger_id}")
        res = call_biometric_api(api_url, fingerprint_id=finger_id)
        if res and "code" in res:
            code = str(res["code"]).strip()
            user_name = res.get("user", {}).get("name", "?")
            user_id = res.get("user", {}).get("id", "?")
            print(f"[+] Compte #{user_id} ({user_name}) -> Code 6 chiffres envoye a l'Arduino : {code}")
            return (code + "\n").encode("utf-8")
        print("[!] Aucun code genere (empreinte non associee en base ou backend arrete).")
        return None

    if raw.startswith("USER:"):
        res = call_biometric_api(api_url, serial_line=raw)
        if res and "serial_command" in res:
            cmd = str(res["serial_command"])
            user_name = res.get("user", {}).get("name", "?")
            print(f"[PI -> ARDUINO] {cmd.strip()} (utilisateur: {user_name})")
            return cmd.encode("utf-8")
        return None

    if raw == "AUTH_FAIL":
        print("[INFO] Empreinte non reconnue par le capteur.")
        return b"CLEAR\n"

    return None


def main() -> None:
    parser = argparse.ArgumentParser(description="Pont serie Drink Manager (Arduino <-> Backend C++)")
    parser.add_argument("--port", default="/dev/ttyACM0", help="Port serie Arduino (ex: /dev/ttyACM0 ou COM3)")
    parser.add_argument("--baud", type=int, default=9600, help="Baudrate (defaut: 9600)")
    parser.add_argument("--api", default="http://127.0.0.1:8080/api", help="URL de l'API C++")
    parser.add_argument("--test-id", type=int, default=None, help="Simuler un ID d'empreinte sans Arduino et quitter")
    args = parser.parse_args()

    api_url = args.api.rstrip("/")
    check_backend_health(api_url)

    if args.test_id is not None:
        reply = handle_serial_line(str(args.test_id), api_url)
        sys.exit(0 if reply is not None else 1)

    if serial is None:
        print("[ERREUR] Module 'pyserial' manquant. Installez-le avec : pip install pyserial")
        sys.exit(1)

    print(f"[INFO] Connexion a l'Arduino sur {args.port} ({args.baud} bauds)...")
    while True:
        try:
            with serial.Serial(args.port, args.baud, timeout=1) as ser:
                time.sleep(2)  # Laisse l'Arduino redemarrer apres ouverture serie
                ser.reset_input_buffer()
                print("[INFO] Connecte ! En ecoute des empreintes...")
                while True:
                    raw = ser.readline().decode("utf-8", errors="ignore").strip()
                    if not raw:
                        continue
                    print(f"[ARDUINO -> PI] {raw}")
                    reply = handle_serial_line(raw, api_url)
                    if reply:
                        ser.write(reply)
                        ser.flush()
        except serial.SerialException as exc:
            print(f"[WARN] Port serie indisponible ({exc}). Nouvelle tentative dans 3s...")
            time.sleep(3)
        except KeyboardInterrupt:
            print("\n[INFO] Arret du pont serie.")
            break


if __name__ == "__main__":
    main()
