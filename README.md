# Brave Frontier Offline — save "missions + home ok" (2026-07-08)

État **fonctionnel confirmé** : les 204 missions se jouent (vrais ennemis/vagues/loot/drops,
positionnement + spawn sur place), la jauge BB marche, l'équipe se gère, et le **home est propre**
(plus de cinématique summoner, plus de crash).

## Contenu (fichiers essentiels, sans les gros assets)
- `server/` — serveur émulé C++ (source `gimuserver/` + runtime `deploy/` avec exe+dll+configs).
  **Pas inclus** : `server/build/` (régénérable CMake), `server/deploy/game_content/` (assets, cf. 21900.zip),
  `packet-generator/target/` (build Rust).
- `server/deploy/system/missions_data.json` — 204 missions (généré par `build_missions_data.py`).
- `server/deploy/system/player_inventory.json` — LA SAVE (unités droppées).
- `client-appx/BraveFrontierPatched.appx` + `MyKey.cer/pfx` — client patché ré-installable.
- `offline-proxy/build/bin/libcurl.dll` — libcurl patché (redirect 127.0.0.1:9960 + fix position ennemi).
- Scripts : `build_missions_data.py`, `fix_assets.py` (répare assets client corrompus +i²),
  `recover_client.ps1` (récup client après reset), `replace_libcurl.ps1` (déploie le libcurl patché).

## Restaurer / relancer (résumé)
1. **Serveur** : rebuild via MSBuild `server/build/standalone_frontend/gimuserverw.vcxproj`
   (reconfigurer CMake d'abord car `build/` est exclu), copier l'exe + les DLLs dans `server/deploy/`,
   lancer avec working dir `deploy`.
2. **Client** : app UWP dans WindowsApps. Si absent → `Add-AppxPackage client-appx/BraveFrontierPatched.appx`
   (sideloading GPO doit être à 1 : `HKLM\SOFTWARE\Policies\Microsoft\Windows\Appx\AllowAllTrustedApps=1`),
   puis déployer le libcurl patché (`replace_libcurl.ps1`, élevé).
3. **Assets client corrompus** (home cassé) → `fix_assets.py` (répare byte[i]-=i²).
4. **Home cassé par état local** → reset client + `recover_client.ps1`.

Détails techniques complets : voir les notes du projet (mémoire Claude / CLAUDE.md).
Assets complets (14.5 Go) = `21900.zip` dans le save précédent `brv v5`.
