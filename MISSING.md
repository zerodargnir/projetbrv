# État d'implémentation du serveur émulé — ce qui marche / ce qui manque

> Carte dressée le 2026-06-21 par analyse du code `gimuserver` + logs d'une session réelle.
> But : savoir quelles features fonctionnent avant de s'y heurter (crash = réponse `{}` ou commande absente).

## Légende
- ✅ **OK** : renvoie de vraies données
- ⚠️ **STUB** : enregistré mais renvoie `{}` (vide) → peut faire **crasher** le client selon l'écran
- ❌ **ABSENT** : aucune route/handler → `{}` → **crash** quand le jeu l'appelle

---

## 1. Endpoints HTTP (routes de haut niveau)

| Route | Handler | État | Note |
|-------|---------|------|------|
| `GET /accounts/guest/login/` | HandleGuest | ✅ | token/user_id codés en dur |
| `GET /dls` | HandleDls | ✅ | liste de téléchargement (cache) |
| `GET /bf/gme/featureCheck.php` | HandleFeatureCheck | ✅ | flags de features |
| `GET /bf/gme/action/getServerTime.php` | HandleServerTime | ✅ | timestamp |
| `POST /bf/gme/action/Daily_login.php` | HandleDailyLogin | ✅ | **ancien** système daily (réponse base64 figée, "not used anymore") |
| `POST /bf/gme/action.php` | HandleGame | ⚙️ | **dispatcher** de 11 sous-commandes (section 2) |
| `GET /` + pages web | BfWebController | ⚙️ | versioninfo/terms partiels ; `/` désactivé |

---

## 2. Commandes `action.php` (le cœur du gameplay)

**11 commandes enregistrées sur les dizaines utilisées par le jeu.**

### ✅ Implémentées (9)
| ID | Nom | Rôle approximatif |
|----|-----|-------------------|
| `MfZyu1q9` | Initialize | sync initial + liste des MST |
| `cTZ3W2JG` | UserInfo | infos du compte joueur |
| `2o4axPIC` | FriendGet | liste d'amis (renvoie liste vide) |
| `uYF93Mhc` | ControlCenterEnter | entrée centre de contrôle |
| `nJ3A7qFp` | BadgeInfo | badges/notifications |
| `Zw3WIoWu` | ChallengeArenaResetInfo | reset challenge arena |
| `m2Ve9PkJ` | DeckEdit | édition d'escouade |
| `Uo86DcRh` | GatchaList | liste des bannières d'invocation |
| `jE6Sp0q4` | MissionStart | démarrage de mission |

### ⚠️ Stubs — renvoient `{}` (sources de crash)
| ID | Nom | Conséquence |
|----|-----|-------------|
| `NiYWKdzs` | **HomeInfo** | **crash des cadeaux quotidiens** (le daily login moderne vit ici) ; bannières/vortex/guild/daily_tasks vides |
| `ynB7X5P9` | **UpdateInfoLight** | infos de mise à jour vides |

### ❌ Absentes — TOUT le reste du gameplay
Toute commande non listée ci-dessus n'a pas de handler → `{}` → crash dès que le jeu l'appelle. Cela couvre notamment :
- **Invocation réelle** (exécuter un tirage gacha)
- **Combat / quêtes** (entrer en donjon, début/fin de bataille, récompenses)
- **Items** (utiliser, vendre, fabriquer, sphères)
- **Unités** (évolution, fusion, amélioration, OE/OE+)
- **Boutique / achats**
- **Guilde** (rejoindre, contribuer, skills)
- **Raid / Arena / Colosseum** (batailles PvP/coop)
- **Frontier Hunter / Trials / Grand Quest / Frontier Gate**
- **Village / Town** (facilities, crafting)
- **Cadeaux entre amis** (envoi/réception)
- **Journal du summoner**, **minigames**, etc.

---

## 3. Modélisation des données (schémas reverse-engineered)

- Structures requête définies : **5** (`*Req`)
- Structures réponse définies : **7** (`*Resp`)

→ Même la *description* des paquets est très partielle : implémenter une nouvelle commande implique souvent d'abord d'en reverse-engineer le schéma (via `packet-generator` + `scripts`).

---

## 4. Bilan

| Catégorie | Compte |
|-----------|--------|
| Endpoints HTTP fonctionnels | 5 |
| Commandes `action.php` OK | 9 |
| Commandes stub (crash possible) | 2 |
| Commandes/features absentes | la grande majorité |

**Ce qui marche aujourd'hui** : boot → login invité → écran d'accueil (affichage).
**Premier mur** : cadeaux quotidiens (HomeInfo stub).
**Pour aller plus loin** : implémenter les handlers un par un (chacun = reverse du schéma + remplissage + build + test). C'est le chantier de fond du projet decompfrontier.
