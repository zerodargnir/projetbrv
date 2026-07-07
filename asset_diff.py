#!/usr/bin/env python3
"""
Diff entre les assets DEMANDES par le jeu (dlc_404.log) et ceux FOURNIS
(server/deploy/game_content apres extraction de 21900.zip).

Le dlc_404.log concatene les chemins sans separateur, chaque entree
commencant par /mst/ ou /content/. On re-segmente la-dessus.
"""
import os
import re
import sys

BASE = os.path.dirname(os.path.abspath(__file__))
LOG = os.path.join(BASE, "server", "deploy", "log", "dlc_404.log")
CONTENT_ROOT = os.path.join(BASE, "server", "deploy", "game_content")


def split_requested(raw):
    """Fonction PURE : a partir du texte brut du log, retourne l'ensemble
    des chemins demandes normalises (ex: 'mst/Ver1002_B57MJYoF.dat').

    Le log concatene les chemins sans separateur, chaque entree commencant
    par /mst/ ou /content/. On re-segmente la-dessus.
    """
    raw = re.sub(r"/(mst|content)/", r"\n/\1/", raw)
    req = set()
    for line in raw.splitlines():
        line = line.strip()
        if line.startswith("/mst/") or line.startswith("/content/"):
            req.add(line.lstrip("/"))  # 'mst/xxx.dat' ou 'content/...'
    return req


def compute_missing(requested, provided):
    """Fonction PURE : separe les manquants par categorie. Retourne un dict
    {'mst': [...trie...], 'content': [...trie...], 'all': set(...)}.
    """
    missing = set(requested) - set(provided)
    return {
        "mst": sorted(p for p in missing if p.startswith("mst/")),
        "content": sorted(p for p in missing if p.startswith("content/")),
        "all": missing,
    }


def parse_requested(log_path):
    """Lit le fichier log et delegue a split_requested (fonction pure)."""
    with open(log_path, "r", encoding="utf-8", errors="ignore") as f:
        raw = f.read()
    return split_requested(raw)


def list_provided(root):
    """Retourne l'ensemble des chemins relatifs presents dans game_content (mst/... , content/...)."""
    prov = set()
    if not os.path.isdir(root):
        return prov
    for dirpath, _dirs, files in os.walk(root):
        for name in files:
            full = os.path.join(dirpath, name)
            rel = os.path.relpath(full, root).replace("\\", "/")
            prov.add(rel)
    return prov


def main():
    if not os.path.isfile(LOG):
        print(f"[!] dlc_404.log introuvable: {LOG}")
        sys.exit(1)

    requested = parse_requested(LOG)
    req_mst = {p for p in requested if p.startswith("mst/")}
    req_content = {p for p in requested if p.startswith("content/")}

    print("=== DEMANDES par le jeu (dlc_404.log) ===")
    print(f"  MST     : {len(req_mst)} fichiers uniques")
    print(f"  Content : {len(req_content)} fichiers uniques")
    print(f"  TOTAL   : {len(requested)} fichiers uniques")

    provided = list_provided(CONTENT_ROOT)
    if not provided:
        print(f"\n[i] game_content vide/inexistant ({CONTENT_ROOT})")
        print("    -> extrais d'abord content/ et mst/ depuis 21900.zip, puis relance.")
        # on liste quand meme un apercu de ce qui est demande
        print("\n--- Apercu MST demandes (20 premiers) ---")
        for p in sorted(req_mst)[:20]:
            print("   ", p)
        print("\n--- Content demandes ---")
        for p in sorted(req_content):
            print("   ", p)
        return

    print(f"\n=== FOURNIS (game_content) ===")
    print(f"  Total fichiers presents : {len(provided)}")

    miss = compute_missing(requested, provided)
    missing = miss["all"]
    miss_mst = miss["mst"]
    miss_content = miss["content"]

    print(f"\n=== MANQUANTS (demandes mais absents) ===")
    print(f"  MST manquants     : {len(miss_mst)}")
    print(f"  Content manquants : {len(miss_content)}")

    cov = 100 * (len(requested) - len(missing)) / max(1, len(requested))
    print(f"  Couverture        : {cov:.1f}%")

    def dump(title, items, limit=50):
        if not items:
            print(f"\n  [OK] Aucun manquant: {title}")
            return
        print(f"\n--- {title} ({len(items)}) ---")
        for p in items[:limit]:
            print("   ", p)
        if len(items) > limit:
            print(f"    ... (+{len(items)-limit} autres)")

    dump("MST manquants", miss_mst)
    dump("Content manquants", miss_content)

    # ecrit la liste complete pour usage ulterieur (recuperation ciblee)
    out = os.path.join(BASE, "missing_assets.txt")
    with open(out, "w", encoding="utf-8") as f:
        for p in miss_mst + miss_content:
            f.write(p + "\n")
    print(f"\n[i] Liste complete des manquants ecrite dans: {out}")


if __name__ == "__main__":
    main()
