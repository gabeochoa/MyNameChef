#!/usr/bin/env python3

import os
import re
import sys
from pathlib import Path

BASE_DIR = Path(__file__).parent.parent
TEST_DIR = BASE_DIR / "src" / "testing" / "tests"

VIOLATIONS = []


def check_file(file_path):
    """Check a single test file for anti-patterns."""
    with open(file_path, "r") as f:
        content = f.read()
        lines = content.split("\n")

    file_violations = []

    for i, line in enumerate(lines, 1):
        stripped = line.strip()

        if re.search(r"EntityHelper::merge_entity_arrays\(\)", stripped):
            file_violations.append(
                (i, "manual_merge", "Manual entity merging detected. Use app.wait_for_frames(1) instead.")
            )

        if re.search(r"System\(\)\s*;", stripped) and "for_each_with" in "\n".join(lines[max(0, i-5):i+5]):
            file_violations.append(
                (i, "manual_system_call", "Manual system instantiation and call detected. Use app.wait_for_frames() instead.")
            )

        if re.search(r"\.for_each_with\s*\(", stripped) and "System" in "\n".join(lines[max(0, i-10):i]):
            file_violations.append(
                (i, "manual_system_call", "Manual system call detected. Use app.wait_for_frames() instead.")
            )

        if re.search(r"GameStateManager::get\(\)\.update_screen\(\)", stripped):
            file_violations.append(
                (i, "direct_state_manipulation", "Direct game state manipulation detected. Use app.wait_for_*() methods instead.")
            )

    return file_violations


def main():
    """Lint all test files."""
    if not TEST_DIR.exists():
        print(f"Error: Test directory not found: {TEST_DIR}")
        return 1

    test_files = sorted(TEST_DIR.glob("*.h"))

    print("Linting test files...")
    print("")

    total_violations = 0

    for test_file in test_files:
        violations = check_file(test_file)
        if violations:
            total_violations += len(violations)
            rel_path = test_file.relative_to(BASE_DIR)
            print(f"{rel_path}:")
            for line_num, rule, message in violations:
                print(f"  Line {line_num}: [{rule}] {message}")
            print("")

    if total_violations > 0:
        print(f"Found {total_violations} violation(s)")
        return 1
    else:
        print("No violations found")
        return 0


if __name__ == "__main__":
    sys.exit(main())

