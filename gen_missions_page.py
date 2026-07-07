# Genere la page HTML "Missions jouables" (catalogue browsable) depuis missions_list.json
import json, html, os

SP = r"C:/Users/mewrd/AppData/Local/Temp/claude/c--Users-mewrd-Desktop-brv/51353d21-bfac-4765-a647-1d171bf5447a/scratchpad"
rows = json.load(open(os.path.join(SP, "missions_list.json"), encoding="utf-8"))

# grouper par zone, ordonner les zones par plus petit id numerique
from collections import defaultdict
zones = defaultdict(list)
for r in rows:
    zones[r["area"]].append(r)
def zkey(z):
    try: return min(int(x["id"]) for x in zones[z])
    except: return 10**12
def znice(z):
    return "Événements & Vortex" if z.isdigit() else z
order = sorted(zones, key=zkey)

tot_missions = len(rows)
tot_waves = sum(r["waves"] for r in rows)
tot_mons  = sum(r["mons"] for r in rows)

def esc(s): return html.escape(str(s))

cards_html = []
for z in order:
    ms = sorted(zones[z], key=lambda r: int(r["id"]))
    zid = "z" + str(abs(hash(z)) % 10**8)
    items = []
    for r in ms:
        foes = "".join(f'<span class="foe">{esc(f)}</span>' for f in r["foes"][:6])
        more = f'<span class="foe more">+{r["mons"]-len(r["foes"][:6])}</span>' if r["mons"] > len(r["foes"][:6]) else ""
        en = f'{r["energy"]}' if r["energy"] else "—"
        search = esc((r["name"] + " " + z + " " + r["id"] + " " + " ".join(r["foes"])).lower())
        items.append(f'''<article class="mission" data-s="{search}">
          <header class="m-head">
            <h3 class="m-name">{esc(r["name"])}</h3>
            <span class="m-id">#{esc(r["id"])}</span>
          </header>
          <div class="m-stats">
            <span class="stat" title="Vagues"><b>{r["waves"]}</b> vagues</span>
            <span class="stat" title="Ennemis au total"><b>{r["mons"]}</b> ennemis</span>
            <span class="stat energy" title="Énergie"><b>{en}</b> én.</span>
          </div>
          <div class="foes">{foes}{more}</div>
        </article>''')
    cards_html.append(f'''<section class="zone" data-zone="{esc(z.lower())}">
      <div class="z-head"><h2>{esc(znice(z))}</h2><span class="z-count">{len(ms)}</span></div>
      <div class="grid">{"".join(items)}</div>
    </section>''')

BODY = f'''<title>Brave Frontier — Missions jouables</title>
<style>
:root {{
  --bg:#14121f; --surface:#1d1a2b; --surface-2:#262236; --border:#332f47;
  --text:#ece9f6; --muted:#9b94b1; --gold:#e2b445; --gold-dim:#b98f34;
  --shadow:0 1px 0 rgba(255,255,255,.03), 0 8px 24px rgba(0,0,0,.35);
  --serif:"Iowan Old Style","Palatino Linotype",Palatino,Georgia,serif;
  --sans:system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;
  --mono:"Cascadia Code","SFMono-Regular",Consolas,"Liberation Mono",monospace;
}}
@media (prefers-color-scheme: light) {{
  :root {{ --bg:#f4f0e7; --surface:#fffdf8; --surface-2:#ece5d7; --border:#e0d7c5;
    --text:#241f2e; --muted:#6c6478; --gold:#9c6f16; --gold-dim:#7f5a12;
    --shadow:0 1px 0 rgba(255,255,255,.6), 0 6px 18px rgba(60,45,20,.10); }}
}}
:root[data-theme="dark"] {{ --bg:#14121f; --surface:#1d1a2b; --surface-2:#262236; --border:#332f47;
  --text:#ece9f6; --muted:#9b94b1; --gold:#e2b445; --gold-dim:#b98f34;
  --shadow:0 1px 0 rgba(255,255,255,.03), 0 8px 24px rgba(0,0,0,.35); }}
:root[data-theme="light"] {{ --bg:#f4f0e7; --surface:#fffdf8; --surface-2:#ece5d7; --border:#e0d7c5;
  --text:#241f2e; --muted:#6c6478; --gold:#9c6f16; --gold-dim:#7f5a12;
  --shadow:0 1px 0 rgba(255,255,255,.6), 0 6px 18px rgba(60,45,20,.10); }}

* {{ box-sizing:border-box; }}
body {{ margin:0; background:var(--bg); color:var(--text); font-family:var(--sans);
  line-height:1.5; -webkit-font-smoothing:antialiased; }}
.wrap {{ max-width:1120px; margin:0 auto; padding:0 20px 72px; }}

header.top {{ position:sticky; top:0; z-index:10; background:color-mix(in srgb,var(--bg) 88%,transparent);
  backdrop-filter:blur(10px); border-bottom:1px solid var(--border); }}
.top-in {{ max-width:1120px; margin:0 auto; padding:18px 20px; display:flex; flex-wrap:wrap;
  align-items:baseline; gap:8px 18px; }}
.brand {{ font-family:var(--serif); font-size:26px; font-weight:700; letter-spacing:.2px;
  color:var(--text); margin:0; text-wrap:balance; }}
.brand b {{ color:var(--gold); font-weight:700; }}
.tagline {{ color:var(--muted); font-size:13.5px; }}
.summary {{ display:flex; gap:22px; margin-left:auto; align-items:baseline; }}
.sm {{ display:flex; flex-direction:column; align-items:flex-end; line-height:1.15; }}
.sm b {{ font-size:19px; font-variant-numeric:tabular-nums; color:var(--gold); }}
.sm span {{ font-size:11px; text-transform:uppercase; letter-spacing:.08em; color:var(--muted); }}
.searchbar {{ width:100%; margin-top:14px; position:relative; }}
.searchbar input {{ width:100%; padding:11px 14px 11px 38px; border-radius:10px; font-size:15px;
  background:var(--surface); border:1px solid var(--border); color:var(--text); font-family:var(--sans); }}
.searchbar input:focus {{ outline:2px solid var(--gold); outline-offset:1px; border-color:var(--gold); }}
.searchbar svg {{ position:absolute; left:12px; top:50%; transform:translateY(-50%); color:var(--muted); }}
.count-line {{ color:var(--muted); font-size:13px; margin:10px 2px 0; }}

.zone {{ margin-top:34px; }}
.z-head {{ display:flex; align-items:center; gap:10px; padding-bottom:10px;
  border-bottom:1px solid var(--border); margin-bottom:16px; }}
.z-head h2 {{ font-family:var(--serif); font-size:20px; font-weight:700; margin:0; }}
.z-count {{ font-size:12px; color:var(--gold); background:color-mix(in srgb,var(--gold) 14%,transparent);
  border:1px solid color-mix(in srgb,var(--gold) 34%,transparent); padding:1px 9px; border-radius:99px;
  font-variant-numeric:tabular-nums; }}

.grid {{ display:grid; grid-template-columns:repeat(auto-fill,minmax(280px,1fr)); gap:14px; }}
.mission {{ background:var(--surface); border:1px solid var(--border); border-radius:12px;
  padding:14px 15px 13px; box-shadow:var(--shadow); display:flex; flex-direction:column; gap:10px; }}
.m-head {{ display:flex; align-items:baseline; justify-content:space-between; gap:8px; }}
.m-name {{ font-size:15.5px; font-weight:650; margin:0; text-wrap:balance; }}
.m-id {{ font-family:var(--mono); font-size:11.5px; color:var(--muted); white-space:nowrap; }}
.m-stats {{ display:flex; gap:7px; flex-wrap:wrap; }}
.stat {{ font-size:12px; color:var(--muted); background:var(--surface-2); border:1px solid var(--border);
  padding:2px 8px; border-radius:7px; }}
.stat b {{ color:var(--text); font-variant-numeric:tabular-nums; }}
.stat.energy b {{ color:var(--gold-dim); }}
.foes {{ display:flex; flex-wrap:wrap; gap:5px; }}
.foe {{ font-size:11px; color:var(--muted); background:transparent; border:1px solid var(--border);
  padding:1px 7px; border-radius:99px; }}
.foe.more {{ color:var(--gold-dim); border-color:color-mix(in srgb,var(--gold) 30%,transparent); }}

.empty {{ display:none; color:var(--muted); text-align:center; padding:60px 0; font-size:15px; }}
footer {{ margin-top:48px; color:var(--muted); font-size:12.5px; text-align:center; }}
</style>

<header class="top">
  <div class="top-in">
    <h1 class="brand">Brave&nbsp;Frontier — <b>Missions jouables</b></h1>
    <span class="tagline">recréées à l'identique (ennemis · vagues · stats · loot)</span>
    <div class="summary">
      <div class="sm"><b>{tot_missions}</b><span>missions</span></div>
      <div class="sm"><b>{len(order)}</b><span>zones</span></div>
      <div class="sm"><b>{tot_waves}</b><span>vagues</span></div>
      <div class="sm"><b>{tot_mons}</b><span>ennemis</span></div>
    </div>
  </div>
  <div class="top-in" style="padding-top:0">
    <div class="searchbar">
      <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="11" cy="11" r="7"/><path d="m21 21-4.3-4.3"/></svg>
      <input id="q" type="search" placeholder="Rechercher une mission, une zone ou un ennemi (ex: Vargas, Mistral, Metal…)" autocomplete="off" aria-label="Rechercher">
    </div>
  </div>
</header>

<div class="wrap">
  <p class="count-line" id="count"></p>
  {"".join(cards_html)}
  <p class="empty" id="empty">Aucune mission ne correspond à « <b id="qe"></b> ».</p>
  <footer>Généré depuis cheahjs (missions.json · missions_parsed · info.json) — positions générées en formation. Les ~2900 autres missions (events récents) n'ont pas de données de vagues.</footer>
</div>

<script>
(function(){{
  const q=document.getElementById('q'), count=document.getElementById('count'),
        empty=document.getElementById('empty'), qe=document.getElementById('qe'),
        missions=[...document.querySelectorAll('.mission')],
        zonesEls=[...document.querySelectorAll('.zone')];
  const total=missions.length;
  function render(){{
    const term=q.value.trim().toLowerCase();
    let shown=0;
    missions.forEach(m=>{{
      const ok = !term || m.dataset.s.includes(term);
      m.style.display = ok ? '' : 'none';
      if(ok) shown++;
    }});
    zonesEls.forEach(z=>{{
      const any=[...z.querySelectorAll('.mission')].some(m=>m.style.display!=='none');
      z.style.display=any?'':'none';
    }});
    empty.style.display = shown ? 'none' : 'block';
    qe.textContent=q.value;
    count.textContent = term ? (shown+' / '+total+' missions affichées') : (total+' missions — tape pour filtrer');
  }}
  q.addEventListener('input', render); render();
}})();
</script>'''

out = r"C:/Users/mewrd/Desktop/brv/missions_page.html"
open(out, "w", encoding="utf-8").write(BODY)
print("ecrit", out, len(BODY), "octets")
