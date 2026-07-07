#!/usr/bin/env python3
"""
Verifie que tous les artefacts de build de la piste A existent et sont valides.
Lancer: python -m unittest test_build_artifacts -v
"""
import os
import unittest

import bf_paths as bf


class TestServerBuild(unittest.TestCase):
    def test_serveur_compile(self):
        self.assertTrue(os.path.isfile(bf.SERVER_EXE), f"manquant: {bf.SERVER_EXE}")
        self.assertGreater(os.path.getsize(bf.SERVER_EXE), 100_000, "exe serveur trop petit")

    def test_dlls_runtime_presentes(self):
        exe_dir = os.path.dirname(bf.SERVER_EXE)
        for dll in bf.SERVER_DLLS:
            with self.subTest(dll=dll):
                self.assertTrue(
                    os.path.isfile(os.path.join(exe_dir, dll)),
                    f"DLL runtime manquante a cote de l'exe: {dll}",
                )

    def test_packets_generes(self):
        self.assertTrue(os.path.isfile(bf.GENERATED_PACKETS), "all.hpp non genere (generate.py)")
        self.assertGreater(os.path.getsize(bf.GENERATED_PACKETS), 0, "all.hpp vide")

    def test_config_deploy_presente(self):
        self.assertTrue(os.path.isfile(os.path.join(bf.SERVER_DEPLOY, "config.json")))


class TestProxyBuild(unittest.TestCase):
    def test_libcurl_dll_existe(self):
        self.assertTrue(os.path.isfile(bf.PROXY_DLL), f"manquant: {bf.PROXY_DLL}")

    def test_libcurl_est_la_version_patchee(self):
        # l'original retail fait ~263 Ko ; le proxy patche ~3.5 Mo
        size = os.path.getsize(bf.PROXY_DLL)
        self.assertGreater(
            size, bf.PROXY_DLL_MIN_SIZE,
            f"libcurl.dll fait {size} octets, semble etre l'original et non le proxy patche",
        )


class TestPacketGeneratorBuild(unittest.TestCase):
    def test_binaire_rust_compile(self):
        self.assertTrue(os.path.isfile(bf.PKTGEN_EXE), f"manquant: {bf.PKTGEN_EXE}")


class TestClientArtifacts(unittest.TestCase):
    def _is_zip(self, path):
        with open(path, "rb") as f:
            return f.read(4) == bf.ZIP_MAGIC

    def test_appx_retail_telecharge_et_valide(self):
        self.assertTrue(os.path.isfile(bf.APPX_RETAIL), "APPX retail non telecharge")
        self.assertTrue(self._is_zip(bf.APPX_RETAIL), "APPX retail n'est pas une archive valide (HTML d'erreur?)")

    def test_appx_patche_existe_et_valide(self):
        if not os.path.isfile(bf.APPX_PATCHED):
            self.skipTest("APPX patche pas encore genere")
        self.assertTrue(self._is_zip(bf.APPX_PATCHED), "APPX patche corrompu")
        self.assertGreater(os.path.getsize(bf.APPX_PATCHED), 1_000_000)


if __name__ == "__main__":
    unittest.main(verbosity=2)
