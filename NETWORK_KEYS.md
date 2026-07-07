# Dictionnaire des clés réseau GME (Brave Frontier)

Récupéré depuis `ProjectOverview.md` (fork Seltraeh) + le code `gimuserver`.
Les clés JSON du protocole sont des hash de noms de champs (obfuscation).

## Enveloppe GME (action.php)
- `F4q6i9xe` = header  ( `Hhgi79M1` = id commande, `aV6cLn3v` = client_id )
- `a3vSYuq2` = body    ( `Kn51uR4Y` = corps chiffré base64 )
- `b5PH6mZa` = error
- Chiffrement : AES/ECB, clé par commande (zero-paddée à 16o), PKCS7, base64.

## Commandes (id -> handler -> clé AES)
- MfZyu1q9 Initialize EmcshnQoDr20TZz1
- cTZ3W2JG UserInfo ScJx6ywWEb0A3njT
- 2o4axPIC FriendGet EoYuZ2nbImhCU1c0
- uYF93Mhc ControlCenterEnter d0k6LGUu
- nJ3A7qFp BadgeInfo bGxX67KB
- Zw3WIoWu ChallengeArenaResetInfo KlwYMGF1
- m2Ve9PkJ DeckEdit d7UuQsq8
- Uo86DcRh GatchaList 8JbxFvuSaB2CK7Ln
- NiYWKdzs HomeInfo f6uOewOD
- jE6Sp0q4 MissionStart csiVLDKkxEwBfR70
- ynB7X5P9 UpdateInfoLight 7kH9NXwC

## Réponse GatchaList (Uo86DcRh)
- `1IR86sAv` = GachaInfo (bannières) — champs: id, gachaType, gachaName, btnImg, bgImg, startDate, endDate, doorImg, description...
- `IBs49NiH` = GachaCategory — champs: id, dispOrder, startDate, endDate, img, gachaIDList
- `6FrKacq7` = SignalKey ( `Kn51uR4Y` = key )  ← DOIT être non vide sinon crash client
- `fEi17cnx` = UserTeamInfo

## Clés de groupe (master data / user) — extrait
6kWq78zx ArenaRankMst, h23iRjGN BadgeInfo, Drudr2w5 DailyLoginRewardsUserInfo,
XGmGpmYW ChallengeArenaUserInfo, Pk5F1vhx BannerInfoMst, k23D7d43 DailyTaskMst,
4NG79sX1 DungeonKeyMst, 2375D38i FeatureGatingInfo, Pf97SzVw GachaEffectMst,
IBs49NiH GachaCategory, 1IR86sAv GachaInfo, 5Y4GJeo3 GachaMst, da3qD39b ResummonGatchaMst,
IkdSufj5 GuildInfo, Bd29Pqw0 LoginCampaignMst, At7Gny2V UrlMst, fEi17cnx UserTeamInfo,
4ceMWH6k UserUnitInfo, GV81ctzR UserUnitDictionary, dX7S2Lc1 UserPartyDeckInfo,
30uygM9m UserGiftInfo, 1ry6BKoe RaidUserInfo, n5mdIUqj UserSummonerInfo ...
(liste complète dans le fork ProjectOverview.md)

## DailyLoginRewardsUserInfo (Drudr2w5) champs
XIvaD6Jp id, 35JXN4Ay user_current_count, 5xStG99s user_spin_limit_count,
ad6i23pO current_day, u8iD6ka7 remaining_days_till_guaranteed_reward,
ZC0msu2L message, outas79f next_reward_id
