# Brave Frontier Offline — TODO / Features en attente

## 🎨 Affichage des drops (demandé 2026-07-06) — À FAIRE
Le système de drops d'unités **marche** (obtention + persistance + jouable), mais il manque le **retour visuel** :
- [ ] Indicateur "capture d'ennemi" **pendant la quête** (quand on tue un mob qui drop)
- [ ] Affichage des drops sur l'**écran de fin de mission**

**Piste** : remplir les champs drop du MonsterGroup (`hw3L0uVj`=unit_drop, `i30R8TAs`=item_drop,
`5wB9SHAV`=treasure_drop) dans MissionStart, actuellement vides.
**Blocage** : le `readParam` .so (0xf6eea0) les stocke juste en string → il faut **RE le format exact**
du consommateur. ⚠️ Risque de crash si mauvais format (cf. le champ position `3g8PW6x0` qui crashait).
Démarche : décompiler le consommateur du getter unit_drop → format → remplir → tester prudemment.

## 💎 Items en drop — À FAIRE
- [ ] Rouler les item drops (déjà dans missions_data : champ `items` = "itemId:pct,...")
- [ ] Ajouter un **inventaire d'items** dans UserInfo (section item) + persistance (comme les unités)

## 👤 Modification summoner (`ZJYkXcHo`) — À FAIRE (dé-enregistré pour l'instant)
Déclenchée en pressant accueil-sur-accueil. **Clé AES extraite = `lyR0us9b`** (crash dump).
- Requête : section `pxjFxjDZ` = [{mnZ5K4Ii:1, 9i2xhMaJ:2, iNy0ZU5M:6(element), btZizNep:2, RVVgyuor:10}]
- ⚠️ Une réponse "{}" / HomeInfo / echo client_id ne complète PAS le flux → **animation en boucle**
  (le client reste coincé en mode édition). Dé-enregistré (SummonerEdit.cpp existe, REGISTER commenté)
  → montre un "Unsupported" dismissable au lieu de boucler.
- À faire : **RE la vraie structure de réponse** (classe réponse ZJYkXcHo dans la .so) pour compléter
  l'édition. Peut-être async (client_id à échoer comme F7JvPk5H).

## ⚗️ Fusion d'unités (`Mw08Clg2`) — À FAIRE
Handler non implémenté (erreur "Unsupported request: Mw08Clg2"). Vrai chantier :
- [ ] Parser la requête (unité base + fodder), calculer XP/niveau, consommer le fodder, persister

## 📋 Autres requêtes non gérées (à capturer)
Le dispatch logue maintenant `>>> UNSUPPORTED request key` pour chaque handler manquant.
- [ ] Reproduire les actions qui donnent "Unsupported" et lister les clés à implémenter

---
## ✅ Déjà fait (rappel)
Combat (positions serveur, formation, spawn en place), Brave Burst, 204 vraies missions
(missions_data.json), Vargas/Eze 7★, DeckEdit, récompenses XP/zel/karma réelles + collectées,
**drop d'unités persistant + jouable**.
