# My Name Chef

A game project forked from supermarket-afterhours template.

## Building

Use `make` for building:
```bash
make
```

The game executable will be `my_name_chef.exe` in the `output/` directory.

## Project Structure

- `src/` contains main game code
- `vendor/` contains third-party libraries
- `resources/` contains assets (images, sounds, fonts)
- `output/` contains build artifacts

## Development

This project uses the same coding standards and patterns as the original supermarket-afterhours project.
# MyNameChef

## Asset Packing (pixelfood spritesheet)

Generate a grouped-row spritesheet (no padding) at 2048px width using a temporary virtual environment:

```bash
python3 -m venv .tmp_venv && source .tmp_venv/bin/activate && python -m pip install --quiet pillow && python scripts/pack_pixelfood.py --src resources/images/pixelfood --out-dir resources/images --max-width 2048 --padding 0; deactivate; rm -rf .tmp_venv
```

Outputs:
- `resources/images/spritesheet_pixelfood.png`
- `resources/images/spritesheet_pixelfood.json`
