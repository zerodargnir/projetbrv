#!/usr/bin/env python3
"""
Tests d'integration de l'emulateur serveur Brave Frontier.
Lance le serveur au besoin (depuis deploy/) puis frappe les endpoints reels.
Lancer: python -m unittest test_server -v
"""
import unittest

import bf_paths as bf

_proc = None


def setUpModule():
    """Demarre le serveur s'il n'ecoute pas deja."""
    global _proc
    try:
        _proc = bf.ensure_server()
    except (FileNotFoundError, RuntimeError, TimeoutError) as e:
        raise unittest.SkipTest(f"serveur indisponible: {e}")


def tearDownModule():
    bf.stop_server(_proc)


class TestServerReachable(unittest.TestCase):
    def test_port_en_ecoute(self):
        self.assertTrue(bf.is_port_open(), "le serveur n'ecoute pas sur 9960")

    def test_route_inconnue_renvoie_404(self):
        # confirme que le routage drogon fonctionne (et ne crashe pas)
        status, _ = bf.http_get("/")
        self.assertEqual(status, 404)


class TestServerTime(unittest.TestCase):
    def test_renvoie_timestamp_numerique(self):
        status, body = bf.http_get("/bf/gme/action/getServerTime.php")
        self.assertEqual(status, 200)
        self.assertTrue(body.strip().isdigit(), f"reponse non numerique: {body!r}")

    def test_timestamp_plausible(self):
        _, body = bf.http_get("/bf/gme/action/getServerTime.php")
        ts = int(body.strip())
        # un timestamp unix moderne (> 2020-01-01, < 2100)
        self.assertGreater(ts, 1_577_836_800)
        self.assertLess(ts, 4_102_444_800)


class TestFeatureCheck(unittest.TestCase):
    def test_json_valide(self):
        status, data = bf.http_get_json("/bf/gme/featureCheck.php")
        self.assertEqual(status, 200)
        self.assertIsInstance(data, dict, "featureCheck ne renvoie pas un JSON objet")

    def test_contient_flags_attendus(self):
        _, data = bf.http_get_json("/bf/gme/featureCheck.php")
        for key in ("raid", "dungeon_key", "trial"):
            with self.subTest(flag=key):
                self.assertIn(key, data)

    def test_flags_booleens_valent_0_ou_1(self):
        # featureCheck melange des booleens (raid, trial...) et des entiers de
        # config (battle_item_limit:100, cooldown_timer:5). On verifie que les
        # flags booleens CONNUS sont bien 0/1.
        _, data = bf.http_get_json("/bf/gme/featureCheck.php")
        for flag in ("raid", "dungeon_key", "trial", "autobattle"):
            with self.subTest(flag=flag):
                self.assertIn(data.get(flag), (0, 1),
                              f"{flag} devrait etre un booleen 0/1, vaut {data.get(flag)!r}")


class TestGuestLogin(unittest.TestCase):
    def test_login_reussit(self):
        status, data = bf.http_get_json("/accounts/guest/login/")
        self.assertEqual(status, 200)
        self.assertIsInstance(data, dict)
        self.assertEqual(data.get("status"), "successful")

    def test_login_fournit_token_et_user_id(self):
        _, data = bf.http_get_json("/accounts/guest/login/")
        self.assertIn("token", data)
        self.assertTrue(data.get("token"), "token vide")
        self.assertIn("game_user_id", data)


if __name__ == "__main__":
    unittest.main(verbosity=2)
