# Brave Frontier Offline — ROADMAP / Avancement

> Objectif global : **faire revivre Brave Frontier** via le mod offline (decompfrontier, piste A) :
> serveur émulé + client retail patché, jouable, puis activer les features une par une.

Dernière mise à jour : **2026-06-24**

---

## 🟢 v2 — "Summon FONCTIONNEL : le tirage donne des unités" (BACKUP ACTUEL)

État figé dans `avancementbrv/v2 - summon donne des unites/`.

### Nouveautés depuis v1
- ✅ **Le tirage donne des unités** : commande GME `F7JvPk5H` (clé AES `bL9fipzaSy7xN2w1`) implémentée (handler `GatchaExec` dans `gme/GatchaList.cpp`). Réponse = sections typées `{1IR86sAv gacha_info, qC2tJs4E unités obtenues, 6FrKacq7 signal, fEi17cnx team}`. L'unité (id 50253 pour test) s'ajoute à la box.
- ✅ Écran détail + confirmation de bannière OK (Super Honor) ; Honor Points donnés (friend_point=99999) ; capacité d'unités=100 ; user_unit_id unique (doublons OK).
- ✅ Fix images gacha manquantes (fallback image sœur, non destructif dans `BfWebController`).
- ✅ Patch carrousel conditionnel (`cdb_patch.txt`) : NOP date+items + garde idiv (count==0→divise par 1) → bannières affichées sans crash, sélection préservée.

### ⚠️ Limite connue — ANIMATION DE TIRAGE NON RÉSOLUE (point de reprise)
- L'**animation/écran de résultat ne se déclenche pas** : la réponse du tirage n'est jamais **routée vers la connexion en attente** (flag "réponse reçue" bit 0x20 jamais mis sur la connexion ; `conn[0x42]`/objet résultat reste null) → CONNECT_ERROR. L'unité est quand même ajoutée (traitement global séparé).
- Tenté sans succès : réponse serveur complète (sections OK, données appliquées), patch client force-result (`FUN_00c7ca30` est contextuel, sans effet), hook du process unités (flag +0x1c non discriminant au runtime). ~64 angles de RE.
- Captures réseau d'un vrai tirage = inexistantes/impossibles (serveurs gumi morts). RE statique (Ghidra/IDA) insuffisant car **dispatch indirect au runtime**.
- **🔜 REPRISE = FRIDA** (traceur dynamique) : hooker le moteur réseau, tracer le routage de la réponse du tirage vs une requête qui marche, voir pourquoi la connexion n'est pas notifiée. Détails techniques (RVA, fonctions, code cave) dans la mémoire `brave-frontier-features.md`.

---

## 🟢 v1 — "Summoning : les bannières apparaissent" (historique)

État figé dans `avancementbrv/v1 - summoning baniere apparait/`.

### Ce qui marche
- ✅ **Build complet** : serveur `gimuserverw` (VS2026 + vcpkg + packet-generator), `libcurl.dll` (offline-proxy), client APPX patché + signé.
- ✅ **Boot du jeu** : écran-titre → accueil. Connexion quotidienne (Claim).
- ✅ **Onglet Unit** : View Units, le starter s'affiche.
- ✅ **Crash Summon corrigé** : cause = `team_info.reinforcement_deck` (clé `TwqMChon`) vide faisait planter le parser client. Fix serveur : `reinforcement_deck = {-1,-99,-99}` dans `gme/GatchaList.cpp`.
- ✅ **Bannières d'invocation AFFICHÉES** (ex "Summer Summon", art complet) via **patch à chaud cdb** :
  NOP de 3 sauts dans `FUN_0055b020` (RVA 0x15b178 / 0x15b182 / 0x15b18d) = bypass des
  filtres date+items du client → les 380 catégories MST passent.
  **→ Lancer avec `play_banners.ps1`** (serveur + client + patch cdb automatique).

### Ce qui ne marche pas encore
- ❌ **Tirage d'invocation** (cliquer une bannière → faire un summon) : pas encore implémenté côté serveur.
- ❌ **Town** : crash client (offset 0xa50e5f, données/assets ville).
- ⚠️ **Shop** : contenu PC-only partiellement absent.
- ❌ **Quêtes** : non testées/implémentées.

### Pièges connus (NE PAS refaire)
- ⛔ **Ne jamais réactiver `ENABLE_FILE_GENERATOR`** dans `BfWebController.cpp` : il était dans
  la branche "fichier existe" et **écrasait les vrais fichiers** par des placeholders 21o →
  a corrompu les 147 MST → boucle "Refreshing the latest data files".
- ⛔ **Ne pas graver le patch bannières dans le .exe + réinstaller l'APPX** : réinstaller vide le
  cache de données du client → relance toute la 1ère synchro. Le patch reste **à chaud (cdb)**.
- 📦 **Miroir d'assets propre** = `projet teste/assets` (content+mst complets) = source de vérité
  pour réparer si game_content est cassé/incomplet.

---

## 🗺️ Prochaines étapes

### v2 — Faire un vrai tirage (summon fonctionnel)
1. Implémenter l'action de tirage côté serveur (commande GME du summon) : tirer N unités selon
   la bannière, renvoyer la réponse attendue par le client.
2. Vérifier l'animation de tirage + l'ajout des unités à l'inventaire.
3. (Idéal) trouver un fix bannières plus propre que le patch cdb (dates MST courantes côté serveur).

### v3 — Quêtes / contenu
4. Réparer le crash Town.
5. Activer une quête simple (start → combat → récompense).

### v4 — Stabilité / confort
6. Persistance utilisateur (DB) : inventaire, gems, progression.
7. Nettoyer les handlers stubs (HomeInfo, UpdateInfoLight).

---

## ▶️ Comment lancer (rappel)
```powershell
# Tout-en-un avec bannières :
powershell -ExecutionPolicy Bypass -File play_banners.ps1
```
Détails build/run : voir la mémoire `brave-frontier-build.md`.
Détails features/RE : `brave-frontier-features.md`. Debug : `brave-frontier-debugging.md`.
