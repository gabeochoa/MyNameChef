#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
OUTPUT_FILE="$PROJECT_ROOT/src/utils/code_hash_generated.h"
REGISTRY_FILE="$PROJECT_ROOT/src/systems/battle_system_registry.cpp"
TEMP_HASH_FILE=$(mktemp)

echo "Generating code hash..."

FILES_TO_HASH=()
SYSTEM_FILES=()
SHARED_UTILS_FOUND=()

if [ ! -f "$REGISTRY_FILE" ]; then
    echo "Error: $REGISTRY_FILE not found" >&2
    exit 1
fi

echo "  Parsing battle_system_registry.cpp for system files..."

FILES_TO_HASH+=("$REGISTRY_FILE")

while IFS= read -r include_file; do
    system_file="$PROJECT_ROOT/src/systems/$include_file"
    if [ -f "$system_file" ]; then
        SYSTEM_FILES+=("$system_file")
        FILES_TO_HASH+=("$system_file")
    fi
done < <(grep -E '^#include "[^/]+\.h"' "$REGISTRY_FILE" | sed 's/^#include "\(.*\)"/\1/')

echo "  Discovering component files..."

while IFS= read -r file; do
    FILES_TO_HASH+=("$file")
done < <(find "$PROJECT_ROOT/src/components" -type f \( -name "*.h" -o -name "*.cpp" \) | sort)

echo "  Discovering shared utility files from system includes..."

for system_file in "${SYSTEM_FILES[@]}"; do
    if [ -f "$system_file" ]; then
        while IFS= read -r include_line; do
            if [[ "$include_line" =~ ^#include\ \"\.\./([^\"]+)\"$ ]]; then
                util_file="${BASH_REMATCH[1]}"
                full_path="$PROJECT_ROOT/src/$util_file"
                if [ -f "$full_path" ] && [[ "$util_file" != components/* ]] && [[ "$util_file" != systems/* ]] && [[ "$util_file" != server/* ]] && [[ "$util_file" != ui/* ]] && [[ "$util_file" != testing/* ]] && [[ "$util_file" != utils/code_hash_generated.h ]]; then
                    SHARED_UTILS_FOUND+=("$full_path")
                fi
            fi
        done < <(grep -E '^#include "\.\./[^"]+"' "$system_file" 2>/dev/null || true)
    fi
done

for util_file in "${SHARED_UTILS_FOUND[@]}"; do
    if ! printf '%s\n' "${FILES_TO_HASH[@]}" | grep -Fxq "$util_file"; then
        FILES_TO_HASH+=("$util_file")
    fi
done

echo "  Found ${#SHARED_UTILS_FOUND[@]} shared utility files (auto-discovered from system includes)"

echo "  Hashing ${#FILES_TO_HASH[@]} files..."

for file in "${FILES_TO_HASH[@]}"; do
    if [ -f "$file" ]; then
        rel_path="${file#$PROJECT_ROOT/}"
        file_hash=$(sha256sum < "$file" | cut -d' ' -f1)
        path_hash=$(echo -n "$rel_path" | sha256sum | cut -d' ' -f1)
        combined="$rel_path|$path_hash|$file_hash"
        echo "$combined" >> "$TEMP_HASH_FILE"
    fi
done

FINAL_HASH=$(sort "$TEMP_HASH_FILE" | sha256sum | cut -d' ' -f1)

rm -f "$TEMP_HASH_FILE"

echo "  Generated hash: $FINAL_HASH"

cat > "$OUTPUT_FILE" << EOF
#pragma once

constexpr const char* SHARED_CODE_HASH = "$FINAL_HASH";
EOF

echo "  Generated $OUTPUT_FILE"
echo "Code hash generation complete."

