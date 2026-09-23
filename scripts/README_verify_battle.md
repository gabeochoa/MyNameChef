# Battle Server Verification

`verify_battle_endpoint.sh` was removed; verification lives in the unified runner (issue 97):

```bash
make
python3 scripts/run_tests.py --client-only --no-server   # skip endpoint check
python3 scripts/run_tests.py                              # includes EndpointVerifier: /health, /battle, error handling
./output/battle_server.exe --run-tests                    # server unit tests only
```

The verifier (in `scripts/run_tests.py:EndpointVerifier`) starts its own server child, checks `/health` for the code hash, posts a valid team to `/battle`, validates `seed/opponentId/outcomes/events/checksum`, then posts an empty team **with the current codeHash** and asserts HTTP 400 `Invalid team...` (issue 90).

Debug snapshots are controlled by server configuration (`"debug": true` in the server config JSON), not by a per-request `debug` field (issue 98) - the `/battle` handler reads `config.debug` only.
