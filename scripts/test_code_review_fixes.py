#!/usr/bin/env python3
"""Focused regression tests for CODE_REVIEW_100_ISSUES fixes (76,77,82,83)."""
import subprocess, sys, unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0, str(Path(__file__).parent))
from pack_pixelfood import ImageEntry, pack_by_group
import run_tests as rt

def e(name, w, h, g="mains"):
    return ImageEntry(filepath=name, filename=name, basekey=name, width=w, height=h, group=g)

class Pack(unittest.TestCase):
    def test_oversized_rejected(self):
        with self.assertRaises(ValueError):
            pack_by_group([e("a.png", 128, 10)], 64, 0)
    def test_wrapped_group_rows_full_height(self):
        _, _, _, rows = pack_by_group([e("a.png", 48, 32), e("b.png", 48, 32)], 64, 0)
        self.assertEqual(rows["mains"], {"y": 0, "h": 64})

class Runner(unittest.TestCase):
    def test_crash_with_success_phrase_is_not_pass(self):
        fake = subprocess.CompletedProcess([], 139, stdout="TEST COMPLETED: x", stderr="")
        with patch.object(rt.subprocess, "run", return_value=fake):
            ok, _ = rt.TestExecutor().run_test("validate_x")
        self.assertFalse(ok)
    def test_discovery_failure_raises(self):
        fake = subprocess.CompletedProcess([], 1, stdout="", stderr="boom")
        with patch.object(rt.subprocess, "run", return_value=fake):
            with self.assertRaises(RuntimeError):
                rt.TestDiscovery.discover_client_tests()

if __name__ == "__main__":
    unittest.main()
