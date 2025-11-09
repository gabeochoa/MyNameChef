#!/usr/bin/env python3

"""
My Name Chef - Unified Python Test Runner
Replaces bash scripts with a more maintainable Python implementation.
"""

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import List, Optional, Tuple

# Colors for output
class Colors:
    RED = '\033[0;31m'
    GREEN = '\033[0;32m'
    YELLOW = '\033[1;33m'
    BLUE = '\033[0;34m'
    NC = '\033[0m'  # No Color

# Configuration
EXECUTABLE = "./output/my_name_chef.exe"
SERVER_EXECUTABLE = "./output/battle_server.exe"
VERIFY_SCRIPT = "./scripts/verify_battle_endpoint.sh"
DEFAULT_TIMEOUT = 30
SERVER_PORT = 8080
BASE_DIR = Path(__file__).parent.parent


class TestDiscovery:
    """Discovers available tests from C++ executables."""
    
    @staticmethod
    def discover_client_tests() -> List[str]:
        """Discover client tests from game executable."""
        try:
            result = subprocess.run(
                [EXECUTABLE, "--list-tests"],
                capture_output=True,
                text=True,
                timeout=10,
                cwd=BASE_DIR
            )
            if result.returncode == 0:
                tests = [line.strip() for line in result.stdout.strip().split('\n') if line.strip()]
                return tests
            return []
        except Exception as e:
            print(f"{Colors.RED}Error discovering client tests: {e}{Colors.NC}")
            return []
    
    @staticmethod
    def discover_server_tests() -> List[str]:
        """Discover server tests from server executable."""
        if not os.path.exists(SERVER_EXECUTABLE):
            return []
        try:
            result = subprocess.run(
                [SERVER_EXECUTABLE, "--list-tests"],
                capture_output=True,
                text=True,
                timeout=10,
                cwd=BASE_DIR
            )
            if result.returncode == 0:
                tests = [line.strip() for line in result.stdout.strip().split('\n') if line.strip()]
                return tests
            return []
        except Exception as e:
            print(f"{Colors.YELLOW}Warning: Could not discover server tests: {e}{Colors.NC}")
            return []


class ServerManager:
    """Manages battle server lifecycle for client tests."""
    
    def __init__(self, port: int = SERVER_PORT):
        self.port = port
        self.server_process: Optional[subprocess.Popen] = None
    
    def start(self) -> bool:
        """Start the battle server."""
        if not os.path.exists(SERVER_EXECUTABLE):
            print(f"{Colors.YELLOW}Warning: Server executable not found at {SERVER_EXECUTABLE}{Colors.NC}")
            return False
        
        print(f"{Colors.BLUE}Starting battle server on port {self.port}...{Colors.NC}")
        
        # Kill any existing battle_server processes
        try:
            subprocess.run(["killall", "battle_server.exe"], 
                         capture_output=True, timeout=2)
            time.sleep(0.5)
        except:
            pass
        
        try:
            self.server_process = subprocess.Popen(
                [SERVER_EXECUTABLE],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=BASE_DIR
            )
            
            # Wait for server to start
            max_attempts = 15
            for attempt in range(max_attempts):
                try:
                    import urllib.request
                    urllib.request.urlopen(f"http://localhost:{self.port}/health", timeout=1)
                    print(f"  {Colors.GREEN}✅ Server started{Colors.NC}")
                    return True
                except:
                    if not self.server_process.poll() is None:
                        # Process died
                        print(f"  {Colors.RED}❌ Server process died during startup{Colors.NC}")
                        return False
                    time.sleep(1)
            
            print(f"  {Colors.RED}❌ Server failed to respond on port {self.port}{Colors.NC}")
            self.stop()
            return False
        except Exception as e:
            print(f"  {Colors.RED}❌ Failed to start server: {e}{Colors.NC}")
            return False
    
    def stop(self):
        """Stop the battle server."""
        if self.server_process:
            try:
                self.server_process.terminate()
                self.server_process.wait(timeout=5)
            except:
                try:
                    self.server_process.kill()
                except:
                    pass
            self.server_process = None


class TestExecutor:
    """Executes individual tests."""
    
    def __init__(self, timeout: int = DEFAULT_TIMEOUT, headless: bool = True):
        self.timeout = timeout
        self.headless = headless
    
    def run_test(self, test_name: str, test_number: int = 0, total: int = 0) -> Tuple[bool, str]:
        """Run a single test and return (success, message)."""
        prefix = f"{Colors.BLUE}[{test_number}/{total}]{Colors.NC} " if total > 0 else ""
        print(f"{prefix}Running test: {Colors.YELLOW}{test_name}{Colors.NC}")
        
        # Determine if this is an integration test (starts with validate_server_)
        is_integration_test = test_name.startswith("validate_server_") and (
            "integration" in test_name or "opponent_match" in test_name or "checksum" in test_name
        )
        
        # Integration tests run in visible mode and need longer timeout
        test_timeout = 60 if is_integration_test else self.timeout
        headless_flag = [] if (is_integration_test or not self.headless) else ["--headless"]
        
        # Server failure tests use fast network checks
        env = os.environ.copy()
        if "server_failure" in test_name:
            test_timeout = 10
            env["NETWORK_CHECK_INTERVAL_SECONDS"] = "0.25"
            env["NETWORK_TIMEOUT_MS"] = "200"
        else:
            env.pop("NETWORK_CHECK_INTERVAL_SECONDS", None)
            env.pop("NETWORK_TIMEOUT_MS", None)
        
        # Use 'timeout' command like bash script does for consistent behavior
        cmd = ["timeout", str(test_timeout), EXECUTABLE, "--run-test", test_name] + headless_flag + ["--timing-speed-scale", "5"]
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=True,
                text=True,
                cwd=BASE_DIR,
                env=env
            )
            
            output = result.stdout + result.stderr
            
            # Check output first (even on non-zero exit code) - test might have passed
            # This matches bash script behavior: check output file even on timeout/errors
            if any(phrase in output for phrase in ["TEST COMPLETED:", "TEST VALIDATION PASSED:", "TEST PASSED:"]):
                if result.returncode == 0:
                    print(f"  {Colors.GREEN}✅ PASSED{Colors.NC} - Test completed successfully")
                else:
                    print(f"  {Colors.GREEN}✅ PASSED{Colors.NC} - Test completed successfully (despite exit code {result.returncode})")
                return (True, "passed")
            
            # Test didn't pass - check exit code for specific error types
            if result.returncode == 0:
                print(f"  {Colors.RED}❌ INCOMPLETE{Colors.NC} - Test ran but didn't complete properly")
                return (False, "incomplete")
            elif result.returncode == 124:  # timeout command exit code
                print(f"  {Colors.RED}❌ TIMEOUT{Colors.NC} - Test exceeded {test_timeout}s timeout")
                return (False, "timeout")
            elif result.returncode == 139:  # SIGSEGV
                print(f"  {Colors.RED}❌ SEGFAULT{Colors.NC} - Test caused segmentation fault")
                return (False, "segfault")
            else:
                print(f"  {Colors.RED}❌ FAILED{Colors.NC} - Test failed with exit code {result.returncode}")
                return (False, f"exit_code_{result.returncode}")
        except subprocess.TimeoutExpired:
            # On timeout, we can't check output, so report timeout
            print(f"  {Colors.RED}❌ TIMEOUT{Colors.NC} - Test exceeded {test_timeout}s timeout")
            return (False, "timeout")
        except Exception as e:
            print(f"  {Colors.RED}❌ ERROR{Colors.NC} - {e}")
            return (False, str(e))


class TestReporter:
    """Formats and displays test results."""
    
    @staticmethod
    def print_summary(passed: int, failed: int, total: int):
        """Print test summary."""
        print("")
        print(f"{Colors.BLUE}📊 Test Summary{Colors.NC}")
        print(f"{Colors.BLUE}{'=' * 15}{Colors.NC}")
        print(f"Total: {total}")
        print(f"  {Colors.GREEN}Passed: {passed}{Colors.NC}")
        print(f"  {Colors.RED}Failed: {failed}{Colors.NC}")
        print("")
        
        if failed == 0:
            print(f"{Colors.GREEN}🎉 All tests passed!{Colors.NC}")
        else:
            print(f"{Colors.RED}⚠️  Some tests failed. Check the output above for details.{Colors.NC}")


def categorize_tests(tests: List[str]) -> Tuple[List[str], List[str]]:
    """Categorize tests into client tests and integration tests.
    
    Uses the same test lists as the bash script to maintain compatibility.
    """
    # Client tests that need the shared server (from bash script)
    CLIENT_TESTS = [
        "validate_main_menu",
        "validate_dish_system",
        "validate_debug_dish_creation",
        "validate_debug_dish_onserve_flavor_stats",
        "validate_debug_dish_onserve_target_scopes",
        "validate_debug_dish_onserve_combat_mods",
        "validate_debug_dish_onstartbattle",
        "validate_debug_dish_oncoursestart",
        "validate_debug_dish_onbitetaken",
        "validate_debug_dish_ondishfinished",
        "validate_debug_dish_oncoursecomplete",
        "validate_trigger_system",
        "validate_effect_system",
        "play_navigates_to_shop",
        "goto_battle",
        "validate_shop_navigation",
        "validate_shop_functionality",
        "validate_reroll_cost",
        "validate_dish_merging",
        "validate_shop_purchase",
        "validate_shop_purchase_no_gold",
        "validate_shop_purchase_insufficient_funds",
        "validate_shop_purchase_full_inventory",
        "validate_shop_purchase_exact_gold",
        "validate_shop_purchase_nonexistent_item",
        "validate_shop_purchase_wrong_screen",
        "validate_seeded_rng_determinism",
        "validate_seeded_rng_helper_methods",
        "validate_combat_system",
        "validate_battle_results",
        "validate_ui_navigation",
        "validate_full_game_flow",
        "validate_server_failure_during_shop",
        "validate_server_failure_during_battle",
    ]
    
    # Integration tests that start their own server (from bash script)
    INTEGRATION_TESTS = [
        "validate_server_battle_integration",
        "validate_server_opponent_match",
    ]
    
    # Filter to only tests that exist in discovered tests
    client_tests = [t for t in CLIENT_TESTS if t in tests]
    integration_tests = [t for t in INTEGRATION_TESTS if t in tests]
    
    return client_tests, integration_tests


def run_test_suite(executor: TestExecutor, client_tests: List[str], 
                   integration_tests: List[str], server_mgr: Optional[ServerManager] = None) -> Tuple[int, int]:
    """Run a test suite and return (passed, failed) counts."""
    passed = 0
    failed = 0
    
    # Run client tests (with shared server)
    if client_tests:
        if server_mgr and not server_mgr.start():
            print(f"{Colors.RED}❌ Failed to start server, skipping client tests{Colors.NC}")
            return (0, len(client_tests))
        
        try:
            for i, test_name in enumerate(client_tests, 1):
                success, _ = executor.run_test(test_name, i, len(client_tests))
                if success:
                    passed += 1
                else:
                    failed += 1
                time.sleep(0.1)  # Small delay between tests
        finally:
            if server_mgr:
                server_mgr.stop()
    
    # Run integration tests (they start their own server)
    if integration_tests:
        for i, test_name in enumerate(integration_tests, 1):
            success, _ = executor.run_test(test_name, i, len(integration_tests))
            if success:
                passed += 1
            else:
                failed += 1
            time.sleep(0.1)
    
    return (passed, failed)


def main():
    parser = argparse.ArgumentParser(description="My Name Chef - Unified Test Runner")
    parser.add_argument("-v", "--visible", action="store_true",
                       help="Run tests in visible mode after headless (runs headless first, then visible)")
    parser.add_argument("-t", "--timeout", type=int, default=DEFAULT_TIMEOUT,
                       help=f"Set timeout per test in seconds (default: {DEFAULT_TIMEOUT})")
    parser.add_argument("--no-server", action="store_true",
                       help="Skip server unit tests")
    parser.add_argument("--no-endpoint", action="store_true",
                       help="Skip battle endpoint verification")
    parser.add_argument("--client-only", action="store_true",
                       help="Skip server tests and endpoint verification")
    
    args = parser.parse_args()
    
    print(f"{Colors.BLUE}🧪 My Name Chef - Test Suite Runner{Colors.NC}")
    print(f"{Colors.BLUE}{'=' * 35}{Colors.NC}")
    print("")
    
    # Check if executables exist
    if not os.path.exists(EXECUTABLE):
        print(f"{Colors.RED}❌ Error: Executable not found at {EXECUTABLE}{Colors.NC}")
        print("Please build the project first with: xmake")
        return 1
    
    # Discover tests
    print(f"{Colors.BLUE}Discovering tests...{Colors.NC}")
    client_tests, integration_tests = categorize_tests(TestDiscovery.discover_client_tests())
    server_tests = [] if args.no_server or args.client_only else TestDiscovery.discover_server_tests()
    
    print(f"  Found {len(client_tests)} client tests")
    print(f"  Found {len(integration_tests)} integration tests")
    if server_tests:
        print(f"  Found {len(server_tests)} server tests")
    print("")
    
    reporter = TestReporter()
    
    total_passed = 0
    total_failed = 0
    
    # Always run headless first
    print(f"{Colors.BLUE}Running tests in headless mode first...{Colors.NC}")
    print("")
    
    executor_headless = TestExecutor(timeout=args.timeout, headless=True)
    
    # Run server unit tests first (if enabled)
    if server_tests and not args.client_only:
        print(f"{Colors.BLUE}🔧 Server Unit Tests{Colors.NC}")
        print(f"{Colors.BLUE}{'=' * 20}{Colors.NC}")
        print("Running server unit tests...")
        
        try:
            result = subprocess.run(
                [SERVER_EXECUTABLE, "--run-tests"],
                capture_output=True,
                text=True,
                timeout=30,
                cwd=BASE_DIR
            )
            
            output = result.stdout + result.stderr
            if result.returncode == 0 and "0 failed" in output:
                print(f"  {Colors.GREEN}✅ PASSED{Colors.NC} - All server tests passed")
            else:
                print(f"  {Colors.RED}❌ FAILED{Colors.NC} - Some server tests failed")
                print("  Server test output:")
                for line in output.split('\n')[-20:]:
                    print(f"    {line}")
                total_failed += 1
        except Exception as e:
            print(f"  {Colors.RED}❌ FAILED{Colors.NC} - {e}")
            total_failed += 1
        print("")
    
    # Run battle endpoint verification (if enabled)
    if not args.no_endpoint and not args.client_only and os.path.exists(VERIFY_SCRIPT):
        print(f"{Colors.BLUE}🌐 Battle Endpoint Verification{Colors.NC}")
        print(f"{Colors.BLUE}{'=' * 33}{Colors.NC}")
        
        server_mgr = ServerManager()
        if server_mgr.start():
            try:
                result = subprocess.run(
                    [VERIFY_SCRIPT, str(SERVER_PORT)],
                    capture_output=True,
                    text=True,
                    timeout=30,
                    cwd=BASE_DIR
                )
                if result.returncode == 0:
                    print(f"  {Colors.GREEN}✅ PASSED{Colors.NC} - All endpoint verification tests passed")
                else:
                    print(f"  {Colors.RED}❌ FAILED{Colors.NC} - Endpoint verification failed")
                    total_failed += 1
            except Exception as e:
                print(f"  {Colors.RED}❌ FAILED{Colors.NC} - {e}")
                total_failed += 1
            finally:
                server_mgr.stop()
        print("")
    
    # Run client tests (with shared server) - headless mode
    if client_tests:
        print(f"{Colors.BLUE}🎮 Client Tests (Headless){Colors.NC}")
        print(f"{Colors.BLUE}{'=' * 25}{Colors.NC}")
        print(f"Found {len(client_tests)} client tests to run")
        print(f"Timeout per test: {args.timeout}s")
        print("")
        
        server_mgr = ServerManager()
        passed, failed = run_test_suite(executor_headless, client_tests, [], server_mgr)
        total_passed += passed
        total_failed += failed
        print("")
    
    # Run integration tests (they start their own server) - headless mode (but integration tests run visible)
    if integration_tests:
        print(f"{Colors.BLUE}🔗 Integration Tests{Colors.NC}")
        print(f"{Colors.BLUE}{'=' * 20}{Colors.NC}")
        print(f"Found {len(integration_tests)} integration tests to run")
        print(f"{Colors.YELLOW}Note: Integration tests start their own server and run in visible mode{Colors.NC}")
        print("")
        
        passed, failed = run_test_suite(executor_headless, [], integration_tests)
        total_passed += passed
        total_failed += failed
        print("")
    
    # If -v flag is set, also run tests in visible mode
    if args.visible:
        print(f"{Colors.BLUE}Running tests in visible mode...{Colors.NC}")
        print("")
        
        executor_visible = TestExecutor(timeout=args.timeout, headless=False)
        
        # Run client tests in visible mode
        if client_tests:
            print(f"{Colors.BLUE}🎮 Client Tests (Visible){Colors.NC}")
            print(f"{Colors.BLUE}{'=' * 24}{Colors.NC}")
            print("")
            
            server_mgr = ServerManager()
            passed, failed = run_test_suite(executor_visible, client_tests, [], server_mgr)
            total_passed += passed
            total_failed += failed
            print("")
        
        # Integration tests already run in visible mode, so skip them here
    
    # Print summary
    total_tests = total_passed + total_failed
    reporter.print_summary(total_passed, total_failed, total_tests)
    
    return 0 if total_failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())

