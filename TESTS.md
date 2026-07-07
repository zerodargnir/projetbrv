# Suite de tests — Brave Frontier Offline (piste A)

Tests qui valident toute la chaîne du mod offline, **sans dépendance à installer**
(stdlib `unittest` + `urllib` uniquement).

## Lancer

```powershell
.\run_tests.ps1
```
ou directement :
```powershell
python -m unittest discover -p "test_*.py" -v
```

## Ce qui est couvert (35 tests)

| Fichier | Rôle | Dépend de |
|---------|------|-----------|
| `test_asset_diff.py` | Logique de l'outil de diff d'assets (parsing des chemins concaténés de `dlc_404.log`, calcul des manquants) | rien (pur) |
| `test_build_artifacts.py` | Les binaires produits existent et sont valides : `gimuserverw.exe` + DLLs runtime, `all.hpp` généré, `libcurl.dll` **patché** (taille > original), `packet-generator.exe`, APPX retail = ZIP valide, APPX patché valide | les builds |
| `test_server.py` | Intégration : **lance le serveur au besoin** puis frappe les endpoints réels (`getServerTime`, `featureCheck`, `accounts/guest/login`) et vérifie les réponses | serveur compilé |
| `test_client.py` | Le client patché est installé (`Get-AppxPackage`), Publisher = `CN=MyBraveFrontier`, exemption loopback présente | Windows + client installé |

## Comportement

- `test_server.py` démarre `gimuserverw.exe` depuis `server/deploy` s'il n'écoute pas
  déjà sur `127.0.0.1:9960`, et l'arrête à la fin (s'il l'a lui-même lancé).
- Les tests qui dépendent d'un état non encore présent se **skippent proprement**
  (ex. APPX patché pas encore généré, client non installé, hors Windows) plutôt
  que d'échouer.

## Fichiers support (non-tests)

- `bf_paths.py` — chemins du projet + helpers (HTTP, port, lancement serveur).
- `asset_diff.py` — outil de diff assets demandés (`dlc_404.log`) vs fournis (`game_content`).
