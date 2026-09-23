# My Name Chef

A game project forked from supermarket-afterhours template.

## Setup (fresh checkout)

```bash
git clone <repo> && cd MyNameChef
git submodule update --init vendor/afterhours   # afterhours engine (required)
# Prerequisites: zig (provides `zig c++`), pkg-config, raylib 5.5 (e.g. `brew install raylib zig pkg-config`)
make            # builds output/my_name_chef.exe + output/battle_server.exe (extensionless on Linux)
./output/battle_server.exe &   # server on :8080 - menu Play requires connection (Continue works offline with a local save)
./output/my_name_chef.exe      # run from repo root (resources/ paths are relative)
```

## Tests

```bash
python3 scripts/run_tests.py          # headless: server unit + endpoint + 88 client/integration
python3 scripts/run_tests.py -v       # also visible mode
python3 scripts/test_code_review_fixes.py
./output/battle_server.exe --run-tests
```

## Project Structure

- `src/` main game code, `vendor/` third-party (afterhours submodule), `resources/` assets, `output/` build artifacts/saves/battle reports

## Asset Packing (pixelfood spritesheet)

```bash
python3 -m venv .tmp_venv && source .tmp_venv/bin/activate && python -m pip install --quiet pillow && python scripts/pack_pixelfood.py --src resources/images/pixelfood --out-dir resources/images --max-width 2048 --padding 0; deactivate; rm -rf .tmp_venv
```
Outputs: `resources/images/spritesheet_pixelfood.png`, `resources/images/spritesheet_pixelfood.json`
