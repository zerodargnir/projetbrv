#!/usr/bin/env python3
"""
Helper partage pour la suite de tests Brave Frontier Offline.
Centralise les chemins du projet + utilitaires (port, HTTP, lancement serveur).
N'est PAS un module de test (pas de prefixe test_).
"""
import json
import os
import socket
import subprocess
import time
import urllib.request

ROOT = os.path.dirname(os.path.abspath(__file__))

# --- Chemins des artefacts de build ---
SERVER_EXE = os.path.join(ROOT, "server", "build", "standalone_frontend", "Debug", "gimuserverw.exe")
SERVER_DEPLOY = os.path.join(ROOT, "server", "deploy")
SERVER_DLLS = ["drogon.dll", "trantor.dll", "sqlite3.dll", "libssl-3-x64.dll", "libcrypto-3-x64.dll"]
GENERATED_PACKETS = os.path.join(ROOT, "server", "gimuserver", "packets", "all.hpp")

PROXY_DLL = os.path.join(ROOT, "offline-proxy", "build", "bin", "libcurl.dll")
PROXY_DLL_MIN_SIZE = 1_000_000  # le libcurl patche fait ~3.5 Mo (l'original retail ~263 Ko)

PKTGEN_EXE = os.path.join(ROOT, "packet-generator", "target", "release", "packet-generator.exe")

APPX_RETAIL = os.path.join(ROOT, "client-appx", "BraveFrontier_2.19.6.0_x86.appx")
APPX_PATCHED = os.path.join(ROOT, "client-appx", "BraveFrontierPatched.appx")

# --- Reseau ---
SERVER_HOST = "127.0.0.1"
SERVER_PORT = 9960
BASE_URL = f"http://{SERVER_HOST}:{SERVER_PORT}"

ZIP_MAGIC = b"PK\x03\x04"  # signature d'une archive ZIP/APPX


def is_port_open(host=SERVER_HOST, port=SERVER_PORT, timeout=0.5):
    """True si quelque chose ecoute sur host:port."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.settimeout(timeout)
        return s.connect_ex((host, port)) == 0


def http_get(path, timeout=8):
    """GET sur le serveur. Retourne (status_code, body_texte)."""
    url = BASE_URL + path
    req = urllib.request.Request(url, method="GET")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
            return resp.status, resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError as e:
        body = e.read().decode("utf-8", errors="replace")
        e.close()
        return e.code, body


def http_get_json(path, timeout=8):
    """GET + parse JSON. Retourne (status_code, objet_python | None)."""
    status, body = http_get(path, timeout=timeout)
    try:
        return status, json.loads(body)
    except (json.JSONDecodeError, ValueError):
        return status, None


def ensure_server(wait=20):
    """S'assure que le serveur ecoute. S'il n'est pas la, le lance depuis deploy/.

    Retourne le Popen si on l'a lance nous-memes (a terminer par l'appelant),
    sinon None (deja en marche, on ne le touche pas).
    """
    if is_port_open():
        return None
    if not os.path.isfile(SERVER_EXE):
        raise FileNotFoundError(f"serveur non compile: {SERVER_EXE}")
    proc = subprocess.Popen(
        [SERVER_EXE],
        cwd=SERVER_DEPLOY,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    deadline = time.time() + wait
    while time.time() < deadline:
        if is_port_open():
            return proc
        if proc.poll() is not None:
            raise RuntimeError("le serveur s'est arrete au demarrage")
        time.sleep(0.3)
    proc.terminate()
    raise TimeoutError(f"le serveur n'ecoute pas apres {wait}s")


def stop_server(proc):
    """Arrete le serveur si on l'a lance (proc non None)."""
    if proc is None:
        return
    proc.terminate()
    try:
        proc.wait(timeout=5)
    except subprocess.TimeoutExpired:
        proc.kill()
