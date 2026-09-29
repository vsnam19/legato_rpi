#!/usr/bin/env python3
"""Unit tests for TelAF simulation repository cloner with HTTP 429 handling."""

import os
from pathlib import Path
import sys
import unittest
from unittest.mock import MagicMock, call, patch

# Add scripts directory to path
REPO_ROOT = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(REPO_ROOT / "scripts" / "telaf_simulation"))

import clone_telaf_repos as cloner


class TestTelAFCloner(unittest.TestCase):
    def test_is_rate_limit_error_detection(self):
        # 429 variants
        self.assertTrue(
            cloner.is_rate_limit_error(
                "fatal: unable to access 'https://git.codelinaro.org/...': The requested URL returned error: 429"
            )
        )
        self.assertTrue(cloner.is_rate_limit_error("HTTP 429 Too Many Requests"))
        self.assertTrue(cloner.is_rate_limit_error("error: 429"))
        self.assertTrue(cloner.is_rate_limit_error("fatal: unable to access 'https://git.codelinaro.org'"))
        self.assertTrue(cloner.is_rate_limit_error("Connection reset by peer"))
        self.assertTrue(cloner.is_rate_limit_error("fatal: early EOF"))

        # Non-rate-limit errors
        self.assertFalse(cloner.is_rate_limit_error(""))
        self.assertFalse(cloner.is_rate_limit_error("fatal: remote branch not found"))
        self.assertFalse(cloner.is_rate_limit_error("Permission denied (publickey)"))

    def test_calculate_backoff_exponential(self):
        b1 = cloner.calculate_backoff(1, base=10.0, factor=2.0, jitter=False)
        self.assertEqual(b1, 10.0)

        b2 = cloner.calculate_backoff(2, base=10.0, factor=2.0, jitter=False)
        self.assertEqual(b2, 20.0)

        b3 = cloner.calculate_backoff(3, base=10.0, factor=2.0, jitter=False)
        self.assertEqual(b3, 40.0)

        # With jitter, should be within +/- 20%
        b_jitter = cloner.calculate_backoff(2, base=10.0, factor=2.0, jitter=True)
        self.assertGreaterEqual(b_jitter, 16.0)
        self.assertLessEqual(b_jitter, 24.0)

    def test_parse_branch_mapping(self):
        conf_path = REPO_ROOT / "scripts" / "telaf_simulation" / "branch_mapping.conf"
        target_dir = Path("/tmp/test_telaf_target")

        targets = cloner.parse_branch_mapping(conf_path, target_dir)
        self.assertEqual(len(targets), 7)

        target_names = [t.name for t in targets]
        self.assertIn("telaf", target_names)
        self.assertIn("legato_legato-af", target_names)
        self.assertIn("legato_3rdParty_Kconfiglib", target_names)
        self.assertIn("legato_3rdParty_jansson", target_names)
        self.assertIn("sdk", target_names)
        self.assertIn("telaf-pa", target_names)
        self.assertIn("telaf-pa-default", target_names)

        # Check a specific target
        telaf_target = next(t for t in targets if t.name == "telaf")
        self.assertEqual(telaf_target.branch, "telaf.lnx.1.1")
        self.assertEqual(
            telaf_target.git_url,
            "https://git.codelinaro.org/clo/le/platform/TelAF.git",
        )
        self.assertEqual(telaf_target.target_path, target_dir / "telaf")

    def test_load_patch_pins(self):
        patch_path = REPO_ROOT / "scripts" / "telaf_simulation" / "patch_me.json"
        pins = cloner.load_patch_pins(patch_path)
        self.assertIn("telaf", pins)
        self.assertEqual(pins["telaf"]["Commit-ID"], "86832ddb2622bdf39e391856bec8a19e0155f6eb")
        self.assertIn("sdk", pins)
        self.assertEqual(pins["sdk"]["Commit-ID"], "0eb6442c2733535b835c4c792a7a69dea0499349")

    @patch("clone_telaf_repos.execute_command")
    @patch("time.sleep")
    def test_clone_retry_on_429(self, mock_sleep, mock_execute):
        target = cloner.RepoTarget(
            name="test_repo",
            relative_path="test_repo",
            git_url="https://git.codelinaro.org/clo/le/test.git",
            branch="master",
            target_path=Path("/tmp/test_target_dir"),
        )

        # Simulate first attempt returning HTTP 429, second attempt succeeding
        mock_execute.side_effect = [
            # Attempt 1 fails with 429
            (128, "", "fatal: unable to access 'https://git.codelinaro.org/clo/le/test.git': The requested URL returned error: 429"),
            # Attempt 2 succeeds
            (0, "Cloning into...", ""),
        ]

        success = cloner.clone_with_retry(
            target=target,
            cache_roots=[],
            max_retries=3,
            initial_backoff=5.0,
            backoff_factor=2.0,
            allow_cache=False,
        )

        self.assertTrue(success)
        self.assertEqual(mock_execute.call_count, 2)
        mock_sleep.assert_called_once()  # Slept after 429 before attempt 2


if __name__ == "__main__":
    unittest.main()
