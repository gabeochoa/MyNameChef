#!/usr/bin/env python3
"""
Pack pixelfood PNGs into a single spritesheet and JSON atlas, grouped by category.
Each category is placed on its own row to reserve space for future items.

Usage:
  python3 scripts/pack_pixelfood.py \
    --src resources/images/pixelfood \
    --out-dir resources/images \
    --max-width 4096 \
    --padding 0

Outputs:
  - spritesheet_pixelfood.png
  - spritesheet_pixelfood.json

Notes:
  - Requires Pillow: python3 -m pip install pillow
  - Groups are inferred from filename keywords; non-food/utility items are excluded.
"""

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass
from typing import Dict, List, Optional, Tuple

try:
    from PIL import Image
except Exception as exc:
    sys.stderr.write(
        "Error: Pillow (PIL) is required. Install with:\n"
        "  python3 -m pip install pillow\n"
    )
    raise


@dataclass
class ImageEntry:
    filepath: str
    filename: str
    basekey: str
    width: int
    height: int
    group: str


def normalize_key(name: str) -> str:
    key = os.path.splitext(os.path.basename(name))[0]
    key = key.strip().lower()
    key = key.replace(" ", "_")
    key = re.sub(r"[^a-z0-9_]+", "_", key)
    key = re.sub(r"_+", "_", key).strip("_")
    return key


def build_grouping_rules() -> List[Tuple[str, List[str]]]:
    return [
        ("mains", [
            "burger", "pizza", "sandwich", "hotdog", "burrito", "ramen", "spaghetti", "steak",
            "sushi", "taco", "nacho", "dumplings", "curry", "roastedchicken", "meatball", "baguette",
            "bun", "bagel", "bread_dish", "loafbread", "macncheese", "pancakes", "waffle", "omlet",
        ]),
        ("desserts", [
            "cake", "cheesecake", "chocolatecake", "fruitcake", "pie", "lemonpie", "eggtart",
            "donut", "gummy", "gummybear", "giantgummybear", "pudding", "cookies", "chocolate",
            "icecream", "jelly",
        ]),
        ("snacks", [
            "chips", "potatochips", "popcorn", "cereal", "snack", "candy", "energy_bar", "cheesepuff",
        ]),
        ("produce", [
            "apple", "banana", "strawberry", "grape", "pepper", "bell_pepper", "cabbage", "mushroom",
            "potato",
        ]),
        ("dairy", [
            "milk", "butter", "yogurt", "cheese",
        ]),
        ("beverages", [
            "wine", "water", "juice", "soda", "soft_drink", "cocoa", "coffee", "grape_soda", "orange_juice",
        ]),
        ("condiments", [
            "ketchup", "mustard", "barbeque", "olive_oil", "cooking_oil", "salt", "sugar", "peanut_butter",
            "jam", "jelly", "sauce",
        ]),
        ("protein", [
            "salmon", "fish", "meat", "sausage", "bacon", "egg", "eggsalad", "friedegg",
        ]),
        ("containers", [
            "dish", "bowl", "plate",
        ]),
    ]


def build_excluded_keywords() -> List[str]:
    return [
        # Non-food or household/tools to exclude
        "ball_pen", "bandage", "batteries", "body_lotion", "basket_metal", "basket_yellow",
        "bathroom_cleaner", "cleaning", "credit_card", "detergent", "dog_food", "eraser", "glue",
        "glue_stick", "hand_sanitiser", "kitchen_knife_set", "kitchen_soap", "light_bulb", "light_bulb_box",
        "paper_bag", "pencil_box", "power_strip", "receipt", "rubber_duck", "rubber_ducktopus", "scissors",
        "scrub", "shampoo", "soap", "soap_box", "sun_cream", "toilet_paper", "toothbrush", "toothbrush_set",
        "toothpaste", "toothpaste_box", "wax", "wet_wipe", "teakettle", "rolling_pin", "whisk", "spatula",
        # Kitchen containers only (exclude standalone utility versions)
        # Note: We keep food-bound dish variants (e.g., hotdog_dish) since they are edible assets
        # but we exclude plain generic containers like "bowl.png" in the root set
        # Separate rule will drop plain container-only assets if they match exactly
    ]


def categorize(basekey: str, rules: List[Tuple[str, List[str]]]) -> str:
    # Produce a single primary group based on first matching rule in precedence order
    for group_name, keywords in rules:
        for kw in keywords:
            if kw in basekey:
                return group_name
    return "other"


def is_food_asset(basekey: str, filename: str, excluded: List[str]) -> bool:
    for bad in excluded:
        if bad in basekey:
            return False
    # Drop obvious standalone containers
    if basekey in {"bowl", "dish", "plate", "01_dish", "02_dish_2", "03_dish_pile", "04_bowl"}:
        return False
    # Likely food if it contains these common edible keywords
    edible_hints = [
        "cake", "pie", "bread", "baguette", "bun", "bagel", "waffle", "pancake", "pancakes",
        "burger", "burrito", "hotdog", "sandwich", "pizza", "ramen", "spaghetti", "steak", "sushi",
        "taco", "nacho", "dumpling", "curry", "roastedchicken", "meatball", "macncheese",
        "donut", "pudding", "cookies", "chocolate", "icecream", "jelly", "jam", "cheesecake",
        "fruitcake", "eggtart", "gummy",
        "chips", "potatochips", "popcorn", "cereal", "snack", "candy", "energy_bar", "cheesepuff",
        "apple", "banana", "strawberry", "grape", "pepper", "cabbage", "mushroom", "potato",
        "milk", "butter", "yogurt", "cheese",
        "ketchup", "mustard", "barbeque", "olive_oil", "cooking_oil", "salt", "sugar", "peanut_butter",
        "water", "wine", "juice", "soda", "soft_drink", "cocoa", "coffee",
        "salmon", "fish", "meat", "sausage", "bacon", "egg", "eggsalad", "friedegg",
        "_dish", "_bowl",
    ]
    for hint in edible_hints:
        if hint in basekey:
            return True
    # Fallback: if the parent directory is pixelfood and we didn't exclude it, keep it
    return True


def collect_images(src_dir: str) -> List[str]:
    paths: List[str] = []
    for entry in sorted(os.listdir(src_dir)):
        if entry.lower().endswith(".png"):
            paths.append(os.path.join(src_dir, entry))
    return paths


def load_entries(paths: List[str]) -> List[ImageEntry]:
    rules = build_grouping_rules()
    excluded = build_excluded_keywords()
    entries: List[ImageEntry] = []
    for path in paths:
        filename = os.path.basename(path)
        basekey = normalize_key(filename)
        if not is_food_asset(basekey, filename, excluded):
            continue
        with Image.open(path) as im:
            width, height = im.size
        group = categorize(basekey, rules)
        entries.append(ImageEntry(
            filepath=path,
            filename=filename,
            basekey=basekey,
            width=width,
            height=height,
            group=group,
        ))
    return entries


@dataclass
class PlacedRect:
    entry: ImageEntry
    x: int
    y: int


def group_order_precedence() -> List[str]:
    # Match the order in build_grouping_rules(); unknown groups come last, with 'other' at the end
    return [
        "mains",
        "desserts",
        "snacks",
        "produce",
        "dairy",
        "beverages",
        "condiments",
        "protein",
        "containers",
    ]


def group_entries(entries: List[ImageEntry]) -> Dict[str, List[ImageEntry]]:
    grouped: Dict[str, List[ImageEntry]] = {}
    for e in entries:
        grouped.setdefault(e.group, []).append(e)
    for g, arr in grouped.items():
        # Sort largest first in each row for better packing to the left
        arr.sort(key=lambda e: (-e.height, -e.width, e.filename))
    return grouped


def pack_by_group(entries: List[ImageEntry], max_width: int, padding: int) -> Tuple[List[PlacedRect], int, int, Dict[str, Dict[str, int]]]:
    grouped = group_entries(entries)
    order = group_order_precedence()
    # Build final ordered list of groups: known precedence, then other custom groups, with 'other' last
    remaining_groups = [g for g in grouped.keys() if g not in order and g != "other"]
    final_order = [g for g in order if g in grouped] + sorted(remaining_groups) + (["other"] if "other" in grouped else [])

    y_cursor = 0
    placements: List[PlacedRect] = []
    group_rows: Dict[str, Dict[str, int]] = {}

    for g in final_order:
        row_entries = grouped[g]
        x_cursor = 0
        row_height = max(e.height for e in row_entries) if row_entries else 0
        for e in row_entries:
            if x_cursor > 0 and x_cursor + e.width > max_width:
                # If a single group's items overflow max width, wrap within the same group row
                # by moving to next line within the group (rare with large max_width)
                y_cursor += row_height + padding
                x_cursor = 0
                row_height = max(e.height for e in row_entries)
            placements.append(PlacedRect(entry=e, x=x_cursor, y=y_cursor))
            x_cursor += e.width + padding
        group_rows[g] = {"y": y_cursor, "h": row_height}
        # Advance to next group row; keep right-side empty space for future items
        y_cursor += row_height + padding

    # Force total width to max_width to reserve horizontal space for future items per row
    total_width = max_width
    total_height = max(0, y_cursor - padding)
    return placements, total_width, total_height, group_rows


def compose_image(placements: List[PlacedRect], width: int, height: int) -> Image.Image:
    sheet = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    for placed in placements:
        with Image.open(placed.entry.filepath) as im:
            sheet.paste(im, (placed.x, placed.y))
    return sheet


def build_groups_index(entries: List[ImageEntry]) -> Dict[str, List[str]]:
    groups: Dict[str, List[str]] = {}
    for e in entries:
        groups.setdefault(e.group, []).append(e.filename)
    for k in list(groups.keys()):
        groups[k] = sorted(groups[k])
    return groups


def save_atlas_json(out_json: str, image_name: str, width: int, height: int, padding: int, placements: List[PlacedRect], group_rows: Dict[str, Dict[str, int]]) -> None:
    frames: Dict[str, Dict[str, object]] = {}
    for p in placements:
        frames[p.entry.filename] = {
            "frame": {"x": p.x, "y": p.y, "w": p.entry.width, "h": p.entry.height},
            "rotated": False,
            "trimmed": False,
            "sourceSize": {"w": p.entry.width, "h": p.entry.height},
            "group": p.entry.group,
        }
    groups_index = build_groups_index([p.entry for p in placements])
    atlas = {
        "meta": {
            "app": "pack_pixelfood.py",
            "version": "1.0",
            "image": image_name,
            "size": {"w": width, "h": height},
            "padding": padding,
            "groupRows": group_rows,
        },
        "groups": groups_index,
        "frames": frames,
    }
    with open(out_json, "w", encoding="utf-8") as f:
        json.dump(atlas, f, indent=2)


def main() -> None:
    parser = argparse.ArgumentParser(description="Pack pixelfood directory into a spritesheet and JSON atlas")
    parser.add_argument("--src", default=os.path.join("resources", "images", "pixelfood"), help="Source directory of PNGs")
    parser.add_argument("--out-dir", default=os.path.join("resources", "images"), help="Output directory for sheet and atlas")
    parser.add_argument("--max-width", type=int, default=4096, help="Max spritesheet width")
    parser.add_argument("--padding", type=int, default=0, help="Padding between sprites in pixels (0 for no padding)")
    parser.add_argument("--image-name", default="spritesheet_pixelfood.png", help="Output PNG filename")
    parser.add_argument("--atlas-name", default="spritesheet_pixelfood.json", help="Output JSON filename")
    args = parser.parse_args()

    src_dir = args.src
    out_dir = args.out_dir
    max_width = args.max_width
    padding = max(0, args.padding)
    image_name = args.image_name
    atlas_name = args.atlas_name

    if not os.path.isdir(src_dir):
        sys.exit(f"Source directory not found: {src_dir}")
    os.makedirs(out_dir, exist_ok=True)

    all_paths = collect_images(src_dir)
    if not all_paths:
        sys.exit(f"No PNG files found in {src_dir}")

    entries = load_entries(all_paths)
    if not entries:
        sys.exit("No edible assets detected after filtering. Adjust filters if needed.")

    placements, total_w, total_h, group_rows = pack_by_group(entries, max_width=max_width, padding=padding)

    sheet = compose_image(placements, width=total_w, height=total_h)

    out_png = os.path.join(out_dir, image_name)
    out_json = os.path.join(out_dir, atlas_name)
    sheet.save(out_png)
    save_atlas_json(out_json, image_name=image_name, width=total_w, height=total_h, padding=padding, placements=placements, group_rows=group_rows)

    print(f"Wrote spritesheet: {out_png} ({total_w}x{total_h})")
    print(f"Wrote atlas:       {out_json}")


if __name__ == "__main__":
    main()


