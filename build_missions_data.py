#!/usr/bin/env python3
# Pre-compile cheahjs (missions.json + missions_parsed/*.json + info.json) -> missions_data.json
# indexe par mission_id, pret a etre lu par le serveur (MissionStart dynamique).
import json, glob, os, re, sys

BASE = r"c:/Users/mewrd/Desktop/brv/cheahjs_data"
OUT  = r"c:/Users/mewrd/Desktop/brv/server/deploy/system/missions_data.json"

def load(p): return json.load(open(os.path.join(BASE, p), encoding="utf-8"))

missions = load("missions.json")          # id -> {name, battle_count, energy_use, xp, zel, karma, ...}
info     = load("info.json")              # unit_id(str) -> full unit/monster def

# indexer missions_parsed par NOM (fichier = "Nom_timestamp.json") ; garder 1 fichier par nom
parsed = {}
for f in glob.glob(os.path.join(BASE, "missions_parsed", "*.json")):
    name = re.sub(r"_\d{4}-.*\.json$", "", os.path.basename(f))
    parsed.setdefault(name, f)  # 1er trouve

def i(x, d=0):
    try: return int(float(str(x)))
    except: return d

items = load("items.json")                # id(str) -> {name, ...}
items_by_name = { v.get("name"): str(k) for k, v in items.items() }

def build_monster(mon):
    uid = str(mon.get("unit id", ""))
    inf = info.get(uid, {})
    st  = mon.get("stats", {}) or {}
    mv  = (inf.get("movement", {}) or {})
    mv_atk = mv.get("attack", {}) or {}
    mv_skl = mv.get("skill", {}) or {}
    # act counts "min~max"
    ac = str(mon.get("act counts", "1~1"))
    amn = re.findall(r"\d+", ac); amin = i(amn[0],1) if amn else 1; amax = i(amn[-1],1) if amn else 1
    # DROPS : unit drop % (le mob lui-meme) + item drops [id, %]
    ud = mon.get("unit drop") or {}
    drop_pct = i(ud.get("percent"), 0) if isinstance(ud, dict) else 0
    idrops = []
    for it in (mon.get("item drop") or []):
        iid = items_by_name.get(it.get("_name"))
        if iid: idrops.append(iid + ":" + str(i(it.get("percent"), 0)))
    # toutes les valeurs en STRING (les champs MonsterDef cote serveur sont des std::string)
    return {
        "unit_id": uid,
        "name": mon.get("_name") or inf.get("name") or ("mob"+uid),
        "element": mon.get("element") or inf.get("element") or "fire",
        "hp":  str(i(st.get("hp"), 100)),
        "atk": str(i(st.get("atk"), 50)),
        "def": str(i(st.get("def"), 10)),
        "drop_check": str(i(inf.get("drop check count"), 2)),
        "min_act": str(amin), "max_act": str(amax),
        "zel_cnt":   str(i(mon.get("zel drop count"), 1)),
        "zel_max":   str(i(mon.get("zel max drop"), 10)),
        "karma_cnt": str(i(mon.get("karma drop count"), 1)),
        "karma_max": str(i(mon.get("karma max drop"), 5)),
        # mouvement (info.json) : valeurs client pour deplacement/attaque
        "move_speed": str(mv_atk.get("move speed", "")) or "100",
        "atk_move":   str(mv_atk.get("move speed type", "0")),
        "skill_move": str(mv_skl.get("move type", "0")),
        "drop_pct":   str(drop_pct),      # % de drop de l'unite (le mob lui-meme)
        "items":      ",".join(idrops),   # "itemId:pct,itemId:pct" items droppables
    }

out = {}
covered = 0
for mid, m in missions.items():
    name = m.get("name")
    pf = parsed.get(name)
    if not pf:
        continue  # pas de donnees de vagues detaillees
    try:
        pdata = list(json.load(open(pf, encoding="utf-8")).values())[0]
    except Exception:
        continue
    battles = pdata.get("battles") or []
    waves = []
    for b in battles:
        mons = [build_monster(x) for x in (b.get("monsters") or []) if str(x.get("unit id",""))]
        if mons: waves.append({"monsters": mons})
    if not waves:
        continue
    out[str(mid)] = {
        "name": name,
        "energy": str(i(m.get("energy_use"), 0)),
        "xp": str(i(m.get("xp"), 0)),
        "zel": str(i(m.get("zel"), 0)),
        "karma": str(i(m.get("karma"), 0)),
        "waves": waves,
    }
    covered += 1

os.makedirs(os.path.dirname(OUT), exist_ok=True)
json.dump(out, open(OUT, "w", encoding="utf-8"), ensure_ascii=False)
# rapport
tot_waves = sum(len(v["waves"]) for v in out.values())
tot_mons  = sum(len(w["monsters"]) for v in out.values() for w in v["waves"])
print(f"missions couvertes : {covered}")
print(f"total vagues       : {tot_waves}")
print(f"total monstres     : {tot_mons}")
print(f"fichier ecrit      : {OUT} ({os.path.getsize(OUT)//1024} Ko)")
# exemple mission 1
if "1" in out:
    m1 = out["1"]
    print(f"\n--- mission 1 '{m1['name']}' : {len(m1['waves'])} vagues ---")
    for wi, w in enumerate(m1["waves"][:4]):
        names = [f"{mo['name']}(hp{mo['hp']})" for mo in w["monsters"]]
        print(f"  vague {wi+1}: {names}")
