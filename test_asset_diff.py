#!/usr/bin/env python3
"""
Tests unitaires pour asset_diff.py.
Lancer:  python -m unittest test_asset_diff -v
(aucune dependance externe, stdlib uniquement)
"""
import os
import tempfile
import unittest

import asset_diff


class TestSplitRequested(unittest.TestCase):
    """Tests de la fonction pure de parsing (le coeur fragile)."""

    def test_concatenated_paths_sont_segmentes(self):
        # Cas reel : chemins colles sans separateur, comme dans dlc_404.log
        raw = "/content/wallpaper/title_logo20151028.jpg/mst/Ver1002_B57MJYoF.dat/mst/Ver10_FA69am2p.dat"
        got = asset_diff.split_requested(raw)
        self.assertEqual(
            got,
            {
                "content/wallpaper/title_logo20151028.jpg",
                "mst/Ver1002_B57MJYoF.dat",
                "mst/Ver10_FA69am2p.dat",
            },
        )

    def test_deduplication(self):
        # Le jeu redemande souvent les memes fichiers -> on doit dedupliquer
        raw = "/mst/A.dat/mst/A.dat/mst/B.dat/mst/A.dat"
        got = asset_diff.split_requested(raw)
        self.assertEqual(got, {"mst/A.dat", "mst/B.dat"})

    def test_classification_mst_vs_content(self):
        raw = "/mst/X.dat/content/y/z.png"
        got = asset_diff.split_requested(raw)
        mst = {p for p in got if p.startswith("mst/")}
        content = {p for p in got if p.startswith("content/")}
        self.assertEqual(mst, {"mst/X.dat"})
        self.assertEqual(content, {"content/y/z.png"})

    def test_chaine_vide(self):
        self.assertEqual(asset_diff.split_requested(""), set())

    def test_ignore_tokens_inconnus(self):
        # Du bruit qui ne commence pas par /mst/ ou /content/ doit etre ignore
        raw = "GARBAGE/api/whatever/mst/Real.dat"
        got = asset_diff.split_requested(raw)
        self.assertEqual(got, {"mst/Real.dat"})

    def test_pas_de_prefixe_slash_dans_resultat(self):
        raw = "/mst/A.dat"
        got = asset_diff.split_requested(raw)
        self.assertTrue(all(not p.startswith("/") for p in got))

    def test_suffixes_numerotes_distincts(self):
        # Ver1084_..._1.dat et _2.dat sont des fichiers differents
        raw = "/mst/Ver1084_2r9cNSdt_1.dat/mst/Ver1084_2r9cNSdt_2.dat"
        got = asset_diff.split_requested(raw)
        self.assertEqual(len(got), 2)


class TestComputeMissing(unittest.TestCase):
    """Tests de la logique de diff (pure)."""

    def test_aucun_manquant_quand_tout_fourni(self):
        req = {"mst/A.dat", "content/w.jpg"}
        prov = {"mst/A.dat", "content/w.jpg", "mst/extra.dat"}
        miss = asset_diff.compute_missing(req, prov)
        self.assertEqual(miss["all"], set())
        self.assertEqual(miss["mst"], [])
        self.assertEqual(miss["content"], [])

    def test_detecte_les_manquants(self):
        req = {"mst/A.dat", "mst/B.dat", "content/w.jpg"}
        prov = {"mst/A.dat"}
        miss = asset_diff.compute_missing(req, prov)
        self.assertEqual(miss["all"], {"mst/B.dat", "content/w.jpg"})
        self.assertEqual(miss["mst"], ["mst/B.dat"])
        self.assertEqual(miss["content"], ["content/w.jpg"])

    def test_resultats_tries(self):
        req = {"mst/C.dat", "mst/A.dat", "mst/B.dat"}
        miss = asset_diff.compute_missing(req, set())
        self.assertEqual(miss["mst"], ["mst/A.dat", "mst/B.dat", "mst/C.dat"])

    def test_fourni_en_trop_ne_compte_pas(self):
        # Des fichiers fournis mais jamais demandes n'apparaissent pas comme manquants
        req = {"mst/A.dat"}
        prov = {"mst/A.dat", "mst/B.dat", "mst/C.dat"}
        miss = asset_diff.compute_missing(req, prov)
        self.assertEqual(miss["all"], set())


class TestListProvided(unittest.TestCase):
    """Tests du parcours du dossier game_content."""

    def test_dossier_inexistant_retourne_vide(self):
        self.assertEqual(asset_diff.list_provided("/chemin/qui/nexiste/pas/xyz"), set())

    def test_walk_et_normalisation_separateurs(self):
        with tempfile.TemporaryDirectory() as root:
            os.makedirs(os.path.join(root, "mst"))
            os.makedirs(os.path.join(root, "content", "wallpaper"))
            open(os.path.join(root, "mst", "A.dat"), "w").close()
            open(os.path.join(root, "content", "wallpaper", "t.jpg"), "w").close()
            got = asset_diff.list_provided(root)
            self.assertEqual(got, {"mst/A.dat", "content/wallpaper/t.jpg"})
            # tous les chemins doivent utiliser '/' meme sous Windows
            self.assertTrue(all("\\" not in p for p in got))


class TestIntegrationParseFichier(unittest.TestCase):
    """Bout-en-bout : ecrit un faux log et verifie parse_requested + diff."""

    def test_parse_requested_depuis_fichier(self):
        with tempfile.TemporaryDirectory() as d:
            log = os.path.join(d, "dlc_404.log")
            with open(log, "w", encoding="utf-8") as f:
                f.write("/content/wallpaper/title.jpg/mst/V1.dat/mst/V2.dat/mst/V1.dat")
            requested = asset_diff.parse_requested(log)
            self.assertEqual(
                requested,
                {"content/wallpaper/title.jpg", "mst/V1.dat", "mst/V2.dat"},
            )
            # diff : on fournit V1 seulement -> manquent V2 + le wallpaper
            miss = asset_diff.compute_missing(requested, {"mst/V1.dat"})
            self.assertEqual(miss["mst"], ["mst/V2.dat"])
            self.assertEqual(miss["content"], ["content/wallpaper/title.jpg"])


if __name__ == "__main__":
    unittest.main(verbosity=2)
