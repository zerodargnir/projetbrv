#!/usr/bin/env python3
"""
Testeur automatique des commandes GME du serveur Brave Frontier.
Rejoue chaque commande (corps capture dans les logs) en reproduisant le
chiffrement GME (AES/ECB + cle par commande + PKCS7 + base64), et affiche
la reponse du serveur (donnees / erreur / vide).

Usage: python gme_probe.py
"""
import base64
import glob
import json
import os
import re
import urllib.request

from Crypto.Cipher import AES
from Crypto.Util.Padding import pad, unpad

BASE = os.path.dirname(os.path.abspath(__file__))
LOGDIR = os.path.join(BASE, "server", "deploy", "log")
URL = "http://127.0.0.1:9960/bf/gme/action.php"

# Enveloppe GME (cles JSON)
H_HEADER, H_ID, H_CID = "F4q6i9xe", "Hhgi79M1", "aV6cLn3v"
B_BODY, B_DATA = "a3vSYuq2", "Kn51uR4Y"
B_ERR = "b5PH6mZa"

# command id -> (nom lisible, cle AES)
COMMANDS = {
    "MfZyu1q9": ("Initialize",             "EmcshnQoDr20TZz1"),
    "Zw3WIoWu": ("ChallengeArenaResetInfo","KlwYMGF1"),
    "nJ3A7qFp": ("BadgeInfo",              "bGxX67KB"),
    "uYF93Mhc": ("ControlCenterEnter",     "d0k6LGUu"),
    "m2Ve9PkJ": ("DeckEdit",               "d7UuQsq8"),
    "2o4axPIC": ("FriendGet",              "EoYuZ2nbImhCU1c0"),
    "Uo86DcRh": ("GatchaList",             "8JbxFvuSaB2CK7Ln"),
    "NiYWKdzs": ("HomeInfo",               "f6uOewOD"),
    "jE6Sp0q4": ("MissionStart",           "csiVLDKkxEwBfR70"),
    "ynB7X5P9": ("UpdateInfoLight",        "7kH9NXwC"),
    "cTZ3W2JG": ("UserInfo",               "ScJx6ywWEb0A3njT"),
}


def aes_key(k: str) -> bytes:
    """Cle AES 16 octets, zero-paddee si plus courte (comme le serveur)."""
    b = k.encode("utf-8")[:16]
    return b + b"\x00" * (16 - len(b))


def enc(body_json: str, key: str) -> str:
    c = AES.new(aes_key(key), AES.MODE_ECB)
    ct = c.encrypt(pad(body_json.encode("utf-8"), AES.block_size))
    return base64.b64encode(ct).decode("ascii")


def dec(b64: str, key: str) -> str:
    c = AES.new(aes_key(key), AES.MODE_ECB)
    pt = c.decrypt(base64.b64decode(b64))
    try:
        return unpad(pt, AES.block_size).decode("utf-8", "replace")
    except ValueError:
        return pt.decode("utf-8", "replace")


def captured_request(cmd: str):
    """Recupere le dernier corps de requete decrypte capture pour cmd."""
    files = sorted(glob.glob(os.path.join(LOGDIR, f"http_log_{cmd}_*.log")),
                   key=os.path.getmtime, reverse=True)
    for f in files:
        with open(f, "r", encoding="utf-8", errors="ignore") as fh:
            txt = fh.read().replace("\x00", "")
        m = re.search(r"REQUEST:\s*(\{.*?\})\s*\n", txt, re.DOTALL)
        if m:
            return m.group(1)
    return None


def probe(cmd: str, name: str, key: str):
    body = captured_request(cmd)
    if body is None:
        return (cmd, name, "SKIP", "pas de requete capturee")
    envelope = {
        H_HEADER: {H_ID: cmd, H_CID: "0839899613932562"},
        B_BODY: {B_DATA: enc(body, key)},
    }
    data = json.dumps(envelope).encode("utf-8")
    req = urllib.request.Request(URL, data=data, method="POST",
                                 headers={"Content-Type": "application/json"})
    try:
        with urllib.request.urlopen(req, timeout=10) as r:
            raw = r.read().decode("utf-8", "replace")
    except Exception as e:
        return (cmd, name, "HTTP-ERR", str(e))

    try:
        obj = json.loads(raw)
    except ValueError:
        return (cmd, name, "200", f"reponse non-JSON ({len(raw)}o)")

    if B_ERR in obj:
        return (cmd, name, "ERREUR", str(obj[B_ERR])[:80])
    enc_body = obj.get(B_BODY, {}).get(B_DATA)
    if not enc_body:
        return (cmd, name, "VIDE", "pas de corps")
    clear = dec(enc_body, key)
    if clear.strip() in ("{}", ""):
        return (cmd, name, "VIDE {}", "")
    return (cmd, name, "OK DONNEES", f"{len(clear)}o: {clear[:60]}")


def main():
    print(f"{'CMD':<10}{'NOM':<24}{'STATUT':<12}DETAIL")
    print("-" * 90)
    for cmd, (name, key) in COMMANDS.items():
        c, n, st, detail = probe(cmd, name, key)
        print(f"{c:<10}{n:<24}{st:<12}{detail}")


if __name__ == "__main__":
    main()
