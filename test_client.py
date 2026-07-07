#!/usr/bin/env python3
"""
Tests de l'etat du client patche (Windows uniquement) :
- le package APPX patche est installe
- l'exemption loopback est en place

Lancer: python -m unittest test_client -v
(se skip proprement hors Windows ou si non installe)
"""
import subprocess
import sys
import unittest


def _ps(cmd, timeout=30):
    """Execute une commande PowerShell, retourne (rc, stdout)."""
    p = subprocess.run(
        ["powershell", "-NoProfile", "-NonInteractive", "-Command", cmd],
        capture_output=True, text=True, timeout=timeout,
    )
    return p.returncode, (p.stdout or "")


@unittest.skipUnless(sys.platform == "win32", "client APPX = Windows uniquement")
class TestClientInstalle(unittest.TestCase):
    def test_appx_brave_frontier_installe(self):
        rc, out = _ps("(Get-AppxPackage *BraveFrontier*).Name")
        if "BraveFrontier" not in out:
            self.skipTest("client patche pas encore installe (Add-AppxPackage)")
        self.assertIn("gumi.BraveFrontier", out)

    def test_publisher_est_notre_cert(self):
        rc, out = _ps("(Get-AppxPackage *BraveFrontier*).Publisher")
        if not out.strip():
            self.skipTest("client non installe")
        self.assertIn("CN=MyBraveFrontier", out)

    def test_exemption_loopback_presente(self):
        rc, out = _ps("CheckNetIsolation LoopbackExempt -s")
        if rc != 0:
            self.skipTest("CheckNetIsolation indisponible")
        self.assertIn("bravefrontier", out.lower(),
                      "exemption loopback absente pour Brave Frontier")


if __name__ == "__main__":
    unittest.main(verbosity=2)
