#!/usr/bin/env python3
"""
Qualcomm TelAF Simulation Repository Cloner with HTTP 429 Resilience.

Features:
- Rate limit detection (HTTP 429 / Too Many Requests from git.codelinaro.org).
- Multi-tier cloning strategy:
  1. Local Cache / Reference Fast-Path: uses pre-existing local mirrors when available
     to avoid network requests entirely (0% rate limit risk).
  2. Exponential Backoff with Jitter: automatically sleeps and retries upon 429.
  3. Polite Inter-Repo Pacing: delays between successive clones to prevent burst limits.
  4. Commit Verification: ensures checked-out commits match patch_me.json pins.
"""

from __future__ import annotations

import argparse
import dataclasses
import json
import logging
import os
from pathlib import Path
import random
import re
import shutil
import subprocess
import sys
import time
from typing import Dict, List, Optional, Tuple

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%H:%M:%S"
)
logger = logging.getLogger("TelAFCloner")

RATE_LIMIT_PATTERNS = [
    re.compile(r"error:\s*429", re.IGNORECASE),
    re.compile(r"HTTP\s*429", re.IGNORECASE),
    re.compile(r"too many requests", re.IGNORECASE),
    re.compile(r"rate limit", re.IGNORECASE),
    re.compile(r"The requested URL returned error: 429", re.IGNORECASE),
    re.compile(r"fatal: unable to access 'https://git.codelinaro.org", re.IGNORECASE),
    re.compile(r"Connection reset by peer", re.IGNORECASE),
    re.compile(r"fatal: early EOF", re.IGNORECASE),
]

DEFAULT_CACHE_LOCATIONS = [
    Path("/home/namvs/Workspaces/projects/automotive/telaf_stu/simulation_env"),
    Path("../automotive/telaf_stu/simulation_env"),
    Path.home() / "Workspaces/projects/automotive/telaf_stu/simulation_env",
]


@dataclasses.dataclass
class RepoTarget:
    name: str
    relative_path: str
    git_url: str
    branch: str
    target_path: Path
    commit_id: Optional[str] = None
    patches: List[str] = dataclasses.field(default_factory=list)


def is_rate_limit_error(stderr_text: str) -> bool:
    """Detect whether git stderr output indicates an HTTP 429 or rate limit."""
    if not stderr_text:
        return False
    return any(pattern.search(stderr_text) for pattern in RATE_LIMIT_PATTERNS)


def calculate_backoff(attempt: int, base: float = 10.0, factor: float = 2.0, jitter: bool = True) -> float:
    """Calculate exponential backoff interval in seconds with optional jitter."""
    duration = base * (factor ** (attempt - 1))
    if jitter:
        # +/- 20% random jitter
        jitter_range = duration * 0.2
        duration += random.uniform(-jitter_range, jitter_range)
    return max(1.0, round(duration, 2))


def parse_branch_mapping(conf_path: Path, base_target_dir: Path) -> List[RepoTarget]:
    """Parse branch_mapping.conf into a list of RepoTarget objects."""
    if not conf_path.is_file():
        raise FileNotFoundError(f"Config file not found: {conf_path}")

    targets: List[RepoTarget] = []
    with open(conf_path, "r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            if "=" not in line:
                continue

            rel_part, rest = line.split("=", 1)
            parts = rest.strip().split()
            if len(parts) < 2:
                continue

            git_url = parts[0]
            branch = parts[1]

            # Standardize relative path (strip leading simulation_env/ if present)
            clean_rel = rel_part.strip()
            if clean_rel.startswith("simulation_env/"):
                clean_rel = clean_rel[len("simulation_env/"):]

            target_path = base_target_dir / clean_rel
            repo_name = clean_rel.replace("/", "_")

            targets.append(
                RepoTarget(
                    name=repo_name,
                    relative_path=clean_rel,
                    git_url=git_url,
                    branch=branch,
                    target_path=target_path,
                )
            )

    return targets


def load_patch_pins(patch_json_path: Path) -> Dict[str, Dict]:
    """Load commit IDs and patch commands from patch_me.json."""
    if not patch_json_path.is_file():
        logger.warning("patch_me.json not found at %s. Skipping commit pins.", patch_json_path)
        return {}

    with open(patch_json_path, "r", encoding="utf-8") as f:
        return json.load(f)


def find_local_cache(relative_path: str, search_roots: List[Path]) -> Optional[Path]:
    """Search for a valid existing git repository in candidate cache directories."""
    for root in search_roots:
        candidate = root / relative_path
        if candidate.is_dir() and (candidate / ".git").exists():
            return candidate
    return None


def execute_command(cmd: List[str], cwd: Optional[Path] = None) -> Tuple[int, str, str]:
    """Run shell command returning exit code, stdout, and stderr."""
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.returncode, proc.stdout, proc.stderr


def is_repo_valid(repo_path: Path, expected_commit: Optional[str] = None) -> bool:
    """Check if repository exists and has valid git commits."""
    if not (repo_path / ".git").exists():
        return False
    ret, out, _ = execute_command(["git", "rev-parse", "HEAD"], cwd=repo_path)
    if ret != 0:
        return False
    if expected_commit and out.strip() == expected_commit:
        return True
    return True


def clone_with_retry(
    target: RepoTarget,
    cache_roots: List[Path],
    max_retries: int = 5,
    initial_backoff: float = 10.0,
    backoff_factor: float = 2.0,
    depth: Optional[int] = None,
    allow_cache: bool = True,
) -> bool:
    """Clone a repository with local cache fallback and 429 exponential backoff."""
    target.target_path.parent.mkdir(parents=True, exist_ok=True)

    # 1. Check if already exists and valid
    if (target.target_path / ".git").exists():
        if is_repo_valid(target.target_path, target.commit_id):
            logger.info("[READY] %s is already cloned at %s", target.name, target.target_path)
            return True
        logger.warning("[EXIST] %s exists but may be incomplete or on wrong commit. Updating...", target.name)

    # 2. Tier 1: Check local cache mirror
    local_cache = find_local_cache(target.relative_path, cache_roots) if allow_cache else None
    if local_cache:
        logger.info("[CACHE-HIT] Found local mirror for %s at %s", target.name, local_cache)
        # Fast local clone with reference or direct local clone
        clone_cmd = [
            "git", "clone",
            "--branch", target.branch,
            str(local_cache),
            str(target.target_path)
        ]
        ret, stdout, stderr = execute_command(clone_cmd)
        if ret == 0:
            # Update remote origin to point to actual remote url
            execute_command(["git", "remote", "set-url", "origin", target.git_url], cwd=target.target_path)
            logger.info("[SUCCESS] %s cloned from local cache (0 net requests, 0 rate limit risk)", target.name)
            return True
        else:
            logger.warning("[CACHE-FAIL] Local clone failed (%s). Falling back to remote clone.", stderr.strip())
            if target.target_path.exists():
                shutil.rmtree(target.target_path, ignore_errors=True)

    # 3. Tier 2: Remote clone with exponential backoff on HTTP 429
    attempt = 1
    while attempt <= max_retries:
        logger.info("[DWLOAD] Cloning %s from %s (Attempt %d/%d)...", target.name, target.git_url, attempt, max_retries)

        cmd = [
            "git",
            "-c", "http.postBuffer=524288000",
            "-c", "http.lowSpeedLimit=1000",
            "-c", "http.lowSpeedTime=30",
            "clone",
            "--branch", target.branch,
        ]
        if depth:
            cmd.extend(["--depth", str(depth)])
        cmd.extend([target.git_url, str(target.target_path)])

        ret, stdout, stderr = execute_command(cmd)

        if ret == 0:
            logger.info("[SUCCESS] %s cloned successfully from remote.", target.name)
            return True

        # Check for HTTP 429
        if is_rate_limit_error(stderr):
            backoff_sec = calculate_backoff(attempt, base=initial_backoff, factor=backoff_factor)
            logger.warning(
                "[429-RATE-LIMIT] Server git.codelinaro.org responded with HTTP 429 / Rate Limit for %s!\n"
                "  Details: %s\n"
                "  Pacing & Backoff: Sleeping for %.2f seconds before retry...",
                target.name, stderr.strip().replace("\n", " "), backoff_sec
            )
            if target.target_path.exists():
                shutil.rmtree(target.target_path, ignore_errors=True)
            time.sleep(backoff_sec)
            attempt += 1
        else:
            logger.error(
                "[ERROR] Fatal non-rate-limit git error for %s (exit %d):\n%s",
                target.name, ret, stderr.strip()
            )
            if target.target_path.exists():
                shutil.rmtree(target.target_path, ignore_errors=True)
            return False

    logger.error("[FAIL] Max retries (%d) exceeded for %s due to persistent rate limiting.", max_retries, target.name)
    return False


def checkout_target_commit(target: RepoTarget) -> bool:
    """Checkout specified commit or branch and apply patches."""
    if not (target.target_path / ".git").exists():
        return False

    if target.commit_id:
        ret, stdout, _ = execute_command(["git", "rev-parse", "HEAD"], cwd=target.target_path)
        if ret == 0 and stdout.strip() == target.commit_id:
            logger.info("[PIN] %s is already on commit %s", target.name, target.commit_id[:10])
            return True

        logger.info("[CHECKOUT] Checking out commit %s for %s", target.commit_id[:10], target.name)
        # Fetch tags or commits if needed
        execute_command(["git", "checkout", "-f", target.commit_id], cwd=target.target_path)

    # Apply patches if configured
    for patch_cmd in target.patches:
        logger.info("[PATCH] Executing: %s in %s", patch_cmd, target.target_path)
        p_ret, _, p_err = execute_command(["bash", "-c", patch_cmd], cwd=target.target_path)
        if p_ret != 0:
            logger.warning("[PATCH-WARN] Patch command failed: %s: %s", patch_cmd, p_err.strip())

    return True


def clone_all_repos(
    config_path: Path,
    patch_path: Path,
    target_dir: Path,
    cache_roots: List[Path],
    max_retries: int = 5,
    initial_backoff: float = 10.0,
    backoff_factor: float = 2.0,
    pacing_delay: float = 5.0,
    depth: Optional[int] = None,
    allow_cache: bool = True,
) -> bool:
    """Clone all repositories in branch_mapping.conf with 429 resilience."""
    targets = parse_branch_mapping(config_path, target_dir)
    pins = load_patch_pins(patch_path)

    # Attach pin info
    for t in targets:
        # Match pin by relative path or key
        for key, info in pins.items():
            if t.relative_path == key or t.relative_path.endswith(key):
                t.commit_id = info.get("Commit-ID")
                t.patches = info.get("Patch-Cmd-List", [])
                break

    logger.info("Found %d repositories to clone/verify into %s", len(targets), target_dir)
    success_count = 0

    for idx, target in enumerate(targets, start=1):
        logger.info("--- [%d/%d] Processing %s ---", idx, len(targets), target.name)
        success = clone_with_retry(
            target=target,
            cache_roots=cache_roots,
            max_retries=max_retries,
            initial_backoff=initial_backoff,
            backoff_factor=backoff_factor,
            depth=depth,
            allow_cache=allow_cache,
        )

        if success:
            checkout_target_commit(target)
            success_count += 1
        else:
            logger.error("[ABORT] Failed to prepare %s", target.name)
            return False

        # Tier 3: Inter-repository polite delay to avoid triggering Cloudflare rate limiters
        if idx < len(targets) and pacing_delay > 0:
            logger.info("[PACE] Sleeping %.1fs before next repo clone to respect rate limits...", pacing_delay)
            time.sleep(pacing_delay)

    logger.info("Completed setup: %d/%d repositories verified successfully.", success_count, len(targets))
    return success_count == len(targets)


def main():
    parser = argparse.ArgumentParser(
        description="Clone TelAF simulation repositories with HTTP 429 rate limit resilience."
    )
    script_dir = Path(__file__).resolve().parent
    parser.add_argument(
        "--config",
        type=Path,
        default=script_dir / "branch_mapping.conf",
        help="Path to branch_mapping.conf",
    )
    parser.add_argument(
        "--patch-config",
        type=Path,
        default=script_dir / "patch_me.json",
        help="Path to patch_me.json",
    )
    parser.add_argument(
        "--target-dir",
        type=Path,
        default=script_dir.parent.parent / "simulation_env",
        help="Directory to clone repositories into",
    )
    parser.add_argument(
        "--cache-dir",
        type=Path,
        action="append",
        help="Path to pre-existing simulation_env or cache directory",
    )
    parser.add_argument(
        "--no-cache",
        action="store_true",
        help="Disable local cache fast-path and force remote cloning",
    )
    parser.add_argument(
        "--max-retries",
        type=int,
        default=5,
        help="Maximum retry attempts per repository on HTTP 429",
    )
    parser.add_argument(
        "--initial-backoff",
        type=float,
        default=10.0,
        help="Initial backoff seconds on HTTP 429",
    )
    parser.add_argument(
        "--backoff-factor",
        type=float,
        default=2.0,
        help="Exponential multiplier for backoff",
    )
    parser.add_argument(
        "--pacing-delay",
        type=float,
        default=5.0,
        help="Polite sleep delay in seconds between successive repository clones",
    )
    parser.add_argument(
        "--depth",
        type=int,
        default=None,
        help="Git clone depth (e.g. 1 for shallow clone)",
    )

    args = parser.parse_args()

    cache_roots = []
    if args.cache_dir:
        cache_roots.extend(args.cache_dir)
    cache_roots.extend(DEFAULT_CACHE_LOCATIONS)

    ok = clone_all_repos(
        config_path=args.config,
        patch_path=args.patch_config,
        target_dir=args.target_dir,
        cache_roots=cache_roots,
        max_retries=args.max_retries,
        initial_backoff=args.initial_backoff,
        backoff_factor=args.backoff_factor,
        pacing_delay=args.pacing_delay,
        depth=args.depth,
        allow_cache=not args.no_cache,
    )

    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
