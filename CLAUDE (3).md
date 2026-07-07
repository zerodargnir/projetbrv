# CLAUDE.md — Projet « Brave Frontier Offline » (decompfrontier)

> Fichier de contexte pour Claude Code. Objectif : **faire revivre Brave Frontier**
> via l'émulation serveur + mod offline, et à plus long terme via la décompilation
> du client. Lis ce fichier en entier avant toute action sur le code.

---

## 1. Mission & vue d'ensemble

Brave Frontier (jeu mobile gacha de gumi/Alim) a vu ses serveurs officiels fermés.
Ce projet collectif (org **`decompfrontier`**) vise à le rendre **rejouable hors-ligne**.

Il y a **deux pistes parallèles** qu'il ne faut pas confondre :

| Piste | But | État | Repos concernés |
| ----- | --- | ---- | --------------- |
| **A — Offline Mod** (prioritaire, jouable) | Patcher un client *retail* + lancer un serveur local émulé | Expérimental mais fonctionnel | `server`, `offline-proxy`, `offline-diff`, `packet-generator`, `scripts` |
| **B — Décompilation** (préservation, long terme) | Recompiler le client depuis les sources reconstruites | WIP, pas forcément buildable | `client`, `client-thirdparty-libs`, `idaidc` |

👉 **Pour obtenir un jeu jouable rapidement, on travaille sur la piste A.** La piste B
est un effort de reverse engineering séparé.

⚠️ **Cibles de version — à vérifier avant de toucher au code :**
- Serveur, offline-diff, idaidc → ciblent **Brave Frontier v2.19.6.0**.
- Le repo `client` mentionne **v2.9.16.0** (Android x86_64). Cet écart de version est
  réel dans les READMEs ; **ne jamais supposer qu'un patch d'une piste s'applique à
  l'autre**. En cas de doute, confirmer la version exacte du client avant de patcher.

---

## 2. Carte des dépôts (par priorité)

| Prio | Repo | Langage | Rôle |
| ---- | ---- | ------- | ---- |
| 1 | `decompfrontier/server` | C++ | **Serveur émulé principal** (cœur du projet) |
| 2 | `decompfrontier/offline-proxy` | C++ | DLL proxy Windows x86 : route le client vers le serveur local, embarque le serveur |
| 3 | `decompfrontier/offline-diff` | Batch + xdelta | Patches pour transformer un APK retail en client offline |
| 4 | `decompfrontier/packet-generator` | C++ | Génération/analyse des schémas de packets réseau et des MST |
| 5 | `decompfrontier/scripts` | Python | Scripts d'analyse/déchiffrement des données (réseau, MST, Burp) |
| 6 | `decompfrontier/decompfrontier.github.io` | Python | Documentation officielle (site statique) |
| 7 | `decompfrontier/client` | C | Décompilation du client (piste B) |
| 8 | `decompfrontier/client-thirdparty-libs` | Shell | Build des libs tierces du client décompilé |
| 9 | `decompfrontier/idaidc` | C/IDC | Typeinfo IDA Pro pour le reverse du client |
| 10 | `arves100/gimufrontier` | C# | ⚠️ **Déprécié** — ancien serveur PoC (2022). Référence historique uniquement |
| 11 | `arves100/emufrontier` | Python 2.7 | ⚠️ **Très ancien** — premier émulateur. Référence historique uniquement |
| 12 | `Seltraeh/Brave-Frontier-Offline` | — | Projet offline perso (externe à l'org) |

**Règle :** ne jamais s'inspirer de `gimufrontier`/`emufrontier` pour du code neuf,
sauf pour comprendre un protocole. Le code de référence vivant est `server` (C++).

---

## 3. Architecture du Offline Mod (piste A)

Flux d'exécution une fois tout assemblé :

```
[ Client Brave Frontier patché ]
        │  appels HTTP(S) du jeu
        ▼
[ offline-proxy ]  ← DLL qui remplace libcurl, redirige + désactive HTTPS
        │  HTTP en clair
        ▼
[ server (émulateur C++) ]  ← écoute sur 127.0.0.1:9960, répond aux packets/MST
        │
        ▼
[ SQLite (drogon ORM) ]  ← état du joueur / données de jeu
```

Points clés de l'archi :
- Le **proxy** se fait passer pour `libcurl` côté client, force le trafic vers
  `127.0.0.1:9960` en HTTP (HTTPS désactivé).
- Sur Windows, le serveur peut être **embarqué dans le proxy** (frontend `PROXYAPPX`)
  ou tourner en **standalone** (frontend `STANDALONE`).
- Les **packets** et **MST** (tables de données maîtresses du jeu) sont le langage
  d'échange ; `packet-generator` produit leurs schémas, `scripts` aide à les décoder.

Doc de référence (à consulter pour les détails d'archi, non clonable ici mais lisible en ligne) :
- `https://decompfrontier.github.io/pages/Architecture/Offlinemod-architecture.html`
- `https://decompfrontier.github.io/pages/Architecture/Emulator-architecture.html`
- `https://decompfrontier.github.io/pages/Tutorial/index.html` (section « Development setup »)

---

## 4. Prérequis globaux (toolchain)

À installer une fois, partagés par plusieurs repos :

- **Git**
- **CMake ≥ 3.21**
- **vcpkg** — cloner et exposer la variable d'environnement `VCPKG_ROOT`
  ```bash
  git clone https://github.com/microsoft/vcpkg
  cd vcpkg && ./bootstrap-vcpkg.sh   # .bat sous Windows
  export VCPKG_ROOT="$PWD"           # à mettre dans ~/.bashrc ou variables système Windows
  ```
- **Compilateur** :
  - Windows : **Visual Studio 2022** (avec « Desktop development with C++ »)
  - Linux : **GCC ou Clang** + **Ninja**
- Piste B (client) : **Android Studio / IntelliJ**, **Android NDK**, un compte **Firebase**.
- Reverse (idaidc) : **IDA Pro**.

---

## 5. Installation & build par repo

### 5.1 `server` — serveur émulé (prio 1)

Dépendances vcpkg (déclarées dans `vcpkg.json`, installées auto au configure) :
`drogon` [sqlite3, ctl, orm], `cryptopp`, `sqlite3`, `glaze`.

Presets CMake disponibles :

| Preset | Plateforme | Frontend | Usage |
| ------ | ---------- | -------- | ----- |
| `debug-win64` | Windows x64 (VS 2022) | `STANDALONE` | Dév/test sous Windows |
| `debug-lnx64` | Linux x64 (Ninja) | `STANDALONE` | Dév/test sous Linux |
| `release-win32` | Windows (VS 2022) | `PROXYAPPX` | Déploiement embarqué dans le proxy (x86-windows-static) |

Build Linux (dév) :
```bash
git clone https://github.com/decompfrontier/server
cd server
cmake --preset debug-lnx64
cmake --build build --config Debug      # ajuste le dossier build selon le preset
```

Build Windows (dév) :
```bash
git clone https://github.com/decompfrontier/server
cd server
cmake --preset debug-win64
cmake --build build --config Debug
```

> Variable clé `GIMUSRV_FRONTEND` : `STANDALONE` (serveur indépendant) ou
> `PROXYAPPX` (serveur compilé pour être embarqué dans `offline-proxy`).

### 5.2 `offline-proxy` — proxy DLL Windows (prio 2)

> ⚠️ **Windows 8+ uniquement, 32-bit (x86)**. Produit `libcurl.dll`.
> Requiert CMake ≥ 3.20, VS 2022 **ou** MinGW, et `libcurl` via vcpkg.

```bash
git clone https://github.com/decompfrontier/offline-proxy
cd offline-proxy
mkdir build && cd build
cmake --preset debug-vs ../
cmake --build . --config Debug
# => libcurl.dll généré dans build/bin/
```

Presets :

| Preset | Compilateur | Usage |
| ------ | ----------- | ----- |
| `deploy-vs` | Visual Studio | Déploiement sur le client BF |
| `deploy-mingw` | MinGW | Déploiement sur le client BF |
| `debug-vs` | Visual Studio | Test client BF + serveur local |
| `debug-mingw` | MinGW | Test client BF + serveur local |

Variables CMake configurables :

| Variable | Description | Défaut |
| -------- | ----------- | ------ |
| `OFFLINEMOD_IP` | IP du serveur à joindre | `127.0.0.1` |
| `OFFLINEMOD_PORT` | Port du serveur | `9960` |
| `OFFLINEMOD_DISABLE_HTTPS` | Patch pour autoriser le HTTP non sécurisé | `1` (true) |

Déploiement : copier `libcurl.dll` dans le dossier d'install de Brave Frontier.

### 5.3 `offline-diff` — patches client Android (prio 3)

Transforme un Android Brave Frontier **v2.19.6.0** retail en client offline.

Procédure (depuis le README) :
1. Récupérer la décompilation Android : `https://github.com/decompfrontier/client`
2. Éditer `sg.gumi.util.BFConfig` → mettre `OFFLINE_MOD` à `true`
3. Appliquer les patches **xdelta** aux fichiers `.so` (fournis dans `jniFiles`)
4. Builder et déployer l'APK modifié
5. Builder le serveur (cf. 5.1)
6. Copier les différents `libofflinemod.so` dans les dossiers `lib/` correspondants (par ABI)

Ce que les patches font côté jeu :
- Charger les assets depuis `/sdcard/data/sg.gumi.bravefrontier/offline_assets`
  (optionnel mais économise 2 Go+)
- Rediriger l'IP du jeu vers `127.0.0.1:9960`
- Désactiver la vérification HTTPS et passer le serveur de jeu en HTTP

### 5.4 `packet-generator` — schémas packets & MST (prio 4)

Génère les schémas de packets réseau et de MST. C++ + CMake/vcpkg (même base toolchain
que `server`). Cloner, configurer avec un preset, builder :
```bash
git clone https://github.com/decompfrontier/packet-generator
cd packet-generator
cmake --preset debug-lnx64   # ou le preset exposé par le repo
cmake --build build
```
> Vérifier le `CMakePresets.json` / `vcpkg.json` du repo pour les presets exacts.

### 5.5 `scripts` — outils d'analyse Python (prio 5)

Scripts autonomes (Python 3). Aucun build, juste un interpréteur.

| Script | Rôle | Exemple |
| ------ | ---- | ------- |
| `networkdecr.py` | Déchiffre des données réseau | `python networkdecr.py` |
| `decfromburp.py` | Décode un export XML Burp (dossier `burp_exports/`) | `python decfromburp.py` |
| `mstdec.py` | Décode un MST | `python mstdec.py Ver1084_2r9cNSdt_1.dat` |

### 5.6 `client` — décompilation (piste B, prio 7)

> WIP, **peut ne pas être buildable**. Cible Android x86_64.

1. Ouvrir `src/android` dans Android Studio / IntelliJ
2. Config Firebase :
   - Créer un projet sur `https://console.firebase.google.com`
   - Ajouter une app Android, nom `Brave Frontier`, package id `sg.gumi.bravefrontier`
   - Télécharger `google-services.json` → le placer dans `src/android/app/`
3. Builder via Android Studio

Libs tierces du client : voir `client-thirdparty-libs` (scripts shell, match par ABI ;
seul x86_64 a libcurl/openssl ✅ à ce jour). Infra inspirée du projet de décompilation
*Zelda BotW*.

### 5.7 `idaidc` — typeinfo IDA Pro (prio 9)

Pour le reverse du client BF 2.19.6.0 :
1. Extraire `libgame.so` depuis `lib/arm64-v8a` de l'APK (aarch64)
2. Laisser IDA finir le dump
3. Importer le fichier `.idc`

---

## 6. Chaîne complète « de zéro à jouable » (piste A)

Ordre recommandé pour un environnement jouable :

1. Installer la toolchain (section 4) + `VCPKG_ROOT`.
2. **Builder `server`** (`debug-lnx64` ou `debug-win64`) et le lancer → écoute sur `127.0.0.1:9960`.
3. **Builder `offline-proxy`** (`debug-vs`) → obtenir `libcurl.dll`.
4. **Appliquer `offline-diff`** à un client Brave Frontier v2.19.6.0 (flag `OFFLINE_MOD`,
   patches xdelta, `libofflinemod.so`).
5. Déployer le client patché + le proxy, pointer vers `127.0.0.1:9960`.
6. Lancer le jeu → il parle au serveur local.

> Sous Windows, alternative : compiler `server` en `release-win32` (`PROXYAPPX`) pour
> l'**embarquer dans le proxy** plutôt que de lancer deux process.

---

## 7. Constantes & config à connaître

- **Adresse serveur par défaut** : `127.0.0.1:9960` (HTTP, pas HTTPS).
- **Package Android** : `sg.gumi.bravefrontier`.
- **Flag offline client** : `OFFLINE_MOD = true` dans `sg.gumi.util.BFConfig`.
- **Frontends serveur** : `STANDALONE` | `PROXYAPPX` (`GIMUSRV_FRONTEND`).
- **Triplets vcpkg** : `x64-linux`, `x64-windows`, `x86-windows-static` (deploy proxy).
- **Versions** : serveur/mod = 2.19.6.0 ; client décomp = 2.9.16.0 (cf. §1).

---

## 8. Conventions & contraintes

- **Licence** : serveur et proxy en **AGPL v3.0+**. Tout dérivé doit rester open source.
- **Non-commercial strict** : aucun ajout de microtransactions, pub, ou monétisation.
  Le projet existe « par les joueurs, pour les joueurs ».
- **Mentions légales** : Brave Frontier © Alim Co., Ltd. Projet **non affilié** à Alim/gumi.
  Ne jamais redistribuer d'assets/binaires originaux du jeu, seulement des patches/diffs.
- **C++** : suivre le style du repo `server` (CMake + vcpkg, drogon/glaze/cryptopp).
- **Python** : scripts autonomes, Python 3 (ne pas régresser vers le Python 2.7 d'`emufrontier`).

---

## 9. Notes de travail pour Claude

- **Avant de patcher** : confirmer la version exacte du client cible (2.19.6.0 vs 2.9.16.0).
  Un patch d'une piste ne s'applique pas à l'autre.
- **Build C++** : toujours vérifier que `VCPKG_ROOT` est défini avant `cmake --preset …`.
  Au premier configure, vcpkg télécharge/compile les deps (drogon est long).
- **Toujours relire** le `CMakePresets.json` et `vcpkg.json` du repo concerné avant de
  proposer une commande de build — les presets évoluent.
- **Ne pas** réutiliser `gimufrontier`/`emufrontier` comme base de code : dépréciés,
  utiles seulement pour comprendre un protocole legacy.
- **Cross-repo** : ce projet est multi-dépôts. Une feature « jouable » touche souvent
  `server` + `offline-proxy` + `offline-diff` ensemble — penser à la cohérence des trois
  (IP/port, version de protocole, ABI des `.so`).
- **Doc** : la source de vérité pour l'archi est le site `decompfrontier.github.io`.

---

## 10. Démarrage rapide (clone de tout)

```bash
mkdir bravefrontier && cd bravefrontier
for r in server offline-proxy offline-diff packet-generator scripts \
         decompfrontier.github.io client client-thirdparty-libs idaidc; do
  git clone "https://github.com/decompfrontier/$r"
done
# Références historiques (optionnel) :
git clone https://github.com/arves100/gimufrontier      # C#, déprécié
git clone https://github.com/arves100/emufrontier       # Python 2.7, très ancien
git clone https://github.com/Seltraeh/Brave-Frontier-Offline
```
