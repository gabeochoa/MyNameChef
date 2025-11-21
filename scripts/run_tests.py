#!/usr/bin/env python3

"""
My Name Chef - Unified Python Test Runner
Replaces bash scripts with a more maintainable Python implementation.
"""

import argparse
import json
import os
import subprocess
import sys
import time
import urllib.error
import urllib.request
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
            
            # Set TEST_SERVER_PID environment variable for tests that need it
            os.environ["TEST_SERVER_PID"] = str(self.server_process.pid)
            
            # Wait for server to start
            max_attempts = 15
            for attempt in range(max_attempts):
                try:
                    import urllib.request
                    urllib.request.urlopen(f"http://localhost:{self.port}/health", timeout=1)
                    print(f"  {Colors.GREEN}✅ Server started (PID: {self.server_process.pid}){Colors.NC}")
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
            # Clear TEST_SERVER_PID environment variable
            os.environ.pop("TEST_SERVER_PID", None)


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
            elif result.returncode == 139:  # SIGSEGV (segmentation fault)
                print(f"  {Colors.RED}❌ CRASHED{Colors.NC} - Segmentation fault (SIGSEGV)")
                return (False, "crash_sigsegv")
            elif result.returncode == 134 or result.returncode == -6:  # SIGABRT
                print(f"  {Colors.RED}❌ CRASHED{Colors.NC} - Aborted (SIGABRT)")
                return (False, "crash_sigabrt")
            elif result.returncode == 136 or result.returncode == -8:  # SIGFPE
                print(f"  {Colors.RED}❌ CRASHED{Colors.NC} - Floating point exception (SIGFPE)")
                return (False, "crash_sigfpe")
            elif result.returncode == 137 or result.returncode == -9:  # SIGKILL
                print(f"  {Colors.RED}❌ CRASHED{Colors.NC} - Killed (SIGKILL)")
                return (False, "crash_sigkill")
            elif result.returncode == 138 or result.returncode == -10:  # SIGBUS
                print(f"  {Colors.RED}❌ CRASHED{Colors.NC} - Bus error (SIGBUS)")
                return (False, "crash_sigbus")
            elif result.returncode < 0:  # Other negative exit codes (signals)
                signal_num = -result.returncode
                print(f"  {Colors.RED}❌ CRASHED{Colors.NC} - Signal {signal_num}")
                return (False, f"crash_signal_{signal_num}")
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


class EndpointVerifier:
    """Verifies battle server HTTP endpoints."""
    
    def __init__(self, port: int = SERVER_PORT):
        self.port = port
        self.base_url = f"http://localhost:{port}"
    
    def verify(self) -> Tuple[bool, str]:
        """Run endpoint verification and return (success, message)."""
        try:
            # 1. Check server health
            if not self._check_health():
                return (False, "Server health check failed")
            
            # 2. Test /battle endpoint
            battle_response = self._test_battle_endpoint()
            if battle_response is None:
                return (False, "Battle endpoint test failed")
            
            # 3. Validate response structure
            if not self._validate_response_structure(battle_response):
                return (False, "Response structure validation failed")
            
            # 4. Test error handling
            if not self._test_error_handling():
                return (False, "Error handling test failed")
            
            return (True, "All endpoint verification tests passed")
        except Exception as e:
            return (False, f"Endpoint verification error: {e}")
    
    def _check_health(self) -> bool:
        """Check if server health endpoint responds."""
        try:
            req = urllib.request.Request(f"{self.base_url}/health")
            with urllib.request.urlopen(req, timeout=5) as response:
                health_data = response.read().decode('utf-8')
                print(f"  {Colors.GREEN}✅ Health check passed: {health_data}{Colors.NC}")
                return True
        except Exception as e:
            print(f"  {Colors.RED}❌ ERROR: Server not responding on {self.base_url}/health{Colors.NC}")
            return False
    
    def _test_battle_endpoint(self) -> Optional[dict]:
        """Test /battle endpoint with a test team."""
        test_team = {
            "team": [
                {"dishType": "Potato", "slot": 0, "level": 1},
                {"dishType": "Burger", "slot": 1, "level": 1},
                {"dishType": "Pizza", "slot": 2, "level": 1}
            ]
        }
        
        try:
            data = json.dumps(test_team).encode('utf-8')
            req = urllib.request.Request(
                f"{self.base_url}/battle",
                data=data,
                headers={'Content-Type': 'application/json'},
                method='POST'
            )
            
            with urllib.request.urlopen(req, timeout=30) as response:
                response_data = response.read().decode('utf-8')
                battle_response = json.loads(response_data)
                print(f"  {Colors.GREEN}✅ Received valid JSON response{Colors.NC}")
                return battle_response
        except urllib.error.HTTPError as e:
            print(f"  {Colors.RED}❌ ERROR: HTTP {e.code} - {e.read().decode('utf-8')}{Colors.NC}")
            return None
        except json.JSONDecodeError as e:
            print(f"  {Colors.RED}❌ ERROR: Response is not valid JSON: {e}{Colors.NC}")
            return None
        except Exception as e:
            print(f"  {Colors.RED}❌ ERROR: Battle endpoint test failed: {e}{Colors.NC}")
            return None
    
    def _validate_response_structure(self, response: dict) -> bool:
        """Validate that response has all required fields and correct types."""
        required_fields = ["seed", "opponentId", "outcomes", "events", "checksum"]
        missing_fields = [field for field in required_fields if field not in response]
        
        if missing_fields:
            print(f"  {Colors.RED}❌ ERROR: Missing required fields: {', '.join(missing_fields)}{Colors.NC}")
            return False
        
        # Validate outcomes is an array
        if not isinstance(response["outcomes"], list):
            print(f"  {Colors.RED}❌ ERROR: 'outcomes' is not an array{Colors.NC}")
            return False
        
        # Validate events is an array
        if not isinstance(response["events"], list):
            print(f"  {Colors.RED}❌ ERROR: 'events' is not an array{Colors.NC}")
            return False
        
        # Validate checksum is not empty
        if not response["checksum"] or response["checksum"] == "null":
            print(f"  {Colors.RED}❌ ERROR: 'checksum' is missing or empty{Colors.NC}")
            return False
        
        print(f"  {Colors.GREEN}✅ All required fields present{Colors.NC}")
        print(f"    Seed: {response['seed']}")
        print(f"    Opponent ID: {response['opponentId']}")
        print(f"    Outcomes count: {len(response['outcomes'])}")
        print(f"    Events count: {len(response['events'])}")
        print(f"    Checksum: {response['checksum']}")
        
        return True
    
    def _test_error_handling(self) -> bool:
        """Test error handling with invalid team."""
        invalid_team = {"team": []}
        
        try:
            data = json.dumps(invalid_team).encode('utf-8')
            req = urllib.request.Request(
                f"{self.base_url}/battle",
                data=data,
                headers={'Content-Type': 'application/json'},
                method='POST'
            )
            
            try:
                with urllib.request.urlopen(req, timeout=10) as response:
                    # Should not succeed
                    print(f"  {Colors.RED}❌ ERROR: Expected HTTP 400 for invalid team, got {response.code}{Colors.NC}")
                    return False
            except urllib.error.HTTPError as e:
                if e.code == 400:
                    error_body = e.read().decode('utf-8')
                    try:
                        error_json = json.loads(error_body)
                        if "error" in error_json:
                            print(f"  {Colors.GREEN}✅ Error handling test passed (HTTP 400 for invalid team){Colors.NC}")
                            return True
                        else:
                            print(f"  {Colors.RED}❌ ERROR: Error response missing 'error' field{Colors.NC}")
                            return False
                    except json.JSONDecodeError:
                        print(f"  {Colors.RED}❌ ERROR: Error response is not valid JSON{Colors.NC}")
                        return False
                else:
                    print(f"  {Colors.RED}❌ ERROR: Expected HTTP 400 for invalid team, got {e.code}{Colors.NC}")
                    return False
        except Exception as e:
            print(f"  {Colors.RED}❌ ERROR: Error handling test failed: {e}{Colors.NC}")
            return False


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
        "validate_set_bonus_american_2_piece",
        "validate_set_bonus_american_4_piece",
        "validate_set_bonus_american_6_piece",
        "validate_set_bonus_no_synergy",
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
        "validate_survivor_carryover_single",
        "validate_survivor_carryover_positions",
        "validate_survivor_carryover_multiple",
        "validate_survivor_carryover_battle_completion",
        "validate_survivor_carryover_simultaneous_defeat",
        "validate_ui_navigation",
        "validate_full_game_flow",
        "validate_server_failure_during_shop",
        "validate_server_failure_during_battle",
        "validate_code_hash",
        "validate_code_hash_mismatch_rejection",
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
    parser.add_argument("--skip-lint", action="store_true",
                       help="Skip test linting")
    
    args = parser.parse_args()
    
    print(f"{Colors.BLUE}🧪 My Name Chef - Test Suite Runner{Colors.NC}")
    print(f"{Colors.BLUE}{'=' * 35}{Colors.NC}")
    print("")
    
    # Run test linter
    if not args.skip_lint:
        print(f"{Colors.BLUE}Running test linter...{Colors.NC}")
        lint_script = BASE_DIR / "scripts" / "lint_tests.py"
        if lint_script.exists():
            import subprocess
            result = subprocess.run([sys.executable, str(lint_script)], cwd=BASE_DIR)
            if result.returncode != 0:
                print(f"{Colors.YELLOW}Warning: Test linter found violations (continuing anyway){Colors.NC}")
                print("")
        else:
            print(f"{Colors.YELLOW}Warning: Lint script not found, skipping{Colors.NC}")
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
    if not args.no_endpoint and not args.client_only:
        print(f"{Colors.BLUE}🌐 Battle Endpoint Verification{Colors.NC}")
        print(f"{Colors.BLUE}{'=' * 33}{Colors.NC}")
        print(f"Testing server on port {SERVER_PORT}")
        print("")
        
        server_mgr = ServerManager()
        if server_mgr.start():
            try:
                verifier = EndpointVerifier(SERVER_PORT)
                success, message = verifier.verify()
                if success:
                    print(f"  {Colors.GREEN}✅ PASSED{Colors.NC} - {message}")
                else:
                    print(f"  {Colors.RED}❌ FAILED{Colors.NC} - {message}")
                    total_failed += 1
            except Exception as e:
                print(f"  {Colors.RED}❌ FAILED{Colors.NC} - {e}")
                total_failed += 1
            finally:
                server_mgr.stop()
        else:
            print(f"  {Colors.RED}❌ FAILED{Colors.NC} - Could not start server for endpoint verification")
            total_failed += 1
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

