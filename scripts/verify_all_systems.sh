#!/bin/bash

# Comprehensive System Verification Script
# Uses 7 verification methods to check all systems

echo "🔍 Comprehensive System Verification"
echo "===================================="
echo ""

SYSTEMS_DIR="src/systems"
MAIN_CPP="src/main.cpp"
PLANNING_DOCS="HEAD_TO_HEAD_COMBAT_PLAN.md next_todo.md TODO.md FEATURE_STATUS.md COMBAT_IMPLEMENTATION_STATUS.md"

# Get all system header files
ALL_SYSTEMS=$(find "$SYSTEMS_DIR" -name "*.h" -type f | sed 's|.*/||' | sed 's|\.h||' | sort)

# Extract systems registered via helper functions
extract_helper_registrations() {
    local helper_file=$1
    if [[ -f "$helper_file" ]]; then
        # Extract all make_unique<Something> patterns, then filter for systems
        # This catches both "System" suffix and non-suffix system names
        grep -oE "make_unique<[^>]+>" "$helper_file" 2>/dev/null | \
            sed 's/make_unique<//' | \
            sed 's/>//' | \
            # Check if it's a system by looking for System in name OR if struct exists in systems/
            while read -r name; do
                if [[ "$name" =~ System$ ]] || \
                   [[ -f "src/systems/${name}.h" ]]; then
                    echo "$name"
                fi
            done | \
            sort -u
    fi
}

# Get systems registered via helpers
SHOP_HELPER_SYSTEMS=$(extract_helper_registrations "src/shop.cpp")
SOUND_HELPER_SYSTEMS=$(extract_helper_registrations "src/sound_systems.cpp")
UI_HELPER_SYSTEMS=$(extract_helper_registrations "src/ui/ui_systems.cpp")

# Also check RenderSystemHelpers.h for systems defined there
RENDER_HELPER_SYSTEMS=$(grep -E "struct.*System" "src/systems/RenderSystemHelpers.h" 2>/dev/null | \
    sed 's/struct //' | \
    sed 's/.*:.*System.*//' | \
    sed 's/ :.*//' | \
    grep -v "^$" | \
    sort -u)

# Combine all helper-registered systems
HELPER_REGISTERED=$(echo "$SHOP_HELPER_SYSTEMS $SOUND_HELPER_SYSTEMS $UI_HELPER_SYSTEMS $RENDER_HELPER_SYSTEMS" | tr ' ' '\n' | sort -u | tr '\n' ' ')

# Debug: Show what we found
# echo "Helper systems found: $HELPER_REGISTERED" >&2

# Utility files (not actual systems - these are header files with utilities, not systems)
UTILITY_FILES="LetterboxLayout RenderSystemHelpers PausableSystem PostProcessingSystems"

echo "📊 Verification Methods:"
echo "1. Direct registration in main.cpp"
echo "2. Helper function registration"
echo "3. Include file check"
echo "4. Component dependency check"
echo "5. Planning document search"
echo "6. Code reference check"
echo "7. File content analysis"
echo ""
echo "Analyzing systems..."
echo ""

UNUSED_COUNT=0
REGISTERED_COUNT=0
UTILITY_COUNT=0

for system in $ALL_SYSTEMS; do
    # Skip TestSystem (special case)
    if [[ "$system" == "TestSystem" ]]; then
        continue
    fi
    
    # Check if utility file (exclude from system check)
    if echo "$UTILITY_FILES" | grep -q "^${system}$"; then
        echo "📁 $system (UTILITY - not a system)"
        UTILITY_COUNT=$((UTILITY_COUNT + 1))
        continue
    fi
    
    # Check if system file doesn't exist (might be defined in header file)
    if [[ ! -f "src/systems/${system}.h" ]]; then
        # Check if it's defined in RenderSystemHelpers.h or PostProcessingSystems.h
        if grep -qE "struct ${system}[^a-zA-Z]" "src/systems/RenderSystemHelpers.h" 2>/dev/null || \
           grep -qE "struct ${system}[^a-zA-Z]" "src/systems/PostProcessingSystems.h" 2>/dev/null; then
            # It's defined in a header file, check if registered
            if grep -q "make_unique<${system}>" "$MAIN_CPP" 2>/dev/null; then
                echo "✅ REGISTERED $system (header file) [M1]"
                REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
                continue
            fi
        fi
    fi
    
    STATUS=""
    METHODS=""
    
    # Method 1: Direct registration (update and render systems)
    # Also check if system is defined in RenderSystemHelpers.h and registered
    if grep -q "make_unique<${system}>" "$MAIN_CPP" 2>/dev/null || \
       grep -q "register_render_system.*make_unique<${system}>" "$MAIN_CPP" 2>/dev/null; then
        STATUS="✅ REGISTERED"
        METHODS="[M1]"
        REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
    # Check if system is defined in RenderSystemHelpers.h and registered in main.cpp
    elif grep -qE "struct ${system}[^a-zA-Z]" "src/systems/RenderSystemHelpers.h" 2>/dev/null && \
         grep -q "make_unique<${system}>" "$MAIN_CPP" 2>/dev/null; then
        STATUS="✅ REGISTERED (RenderSystemHelpers.h)"
        METHODS="[M1,M3]"
        REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
    # Method 2: Helper registration
    # Check if system name appears in helper registrations (exact match or partial)
    # Also handle systems that don't have "System" suffix (e.g., GenerateShopSlots)
    elif echo "$HELPER_REGISTERED" | grep -qw "${system}" || \
         (echo "$HELPER_REGISTERED" | grep -q "${system}" && \
          echo "$HELPER_REGISTERED" | grep -q "${system}System"); then
        STATUS="✅ REGISTERED (via helper)"
        METHODS="[M2]"
        REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
    # Also check if the system name without "System" suffix matches a helper registration
    elif [[ "$system" =~ System$ ]] && \
         system_base="${system%System}" && \
         echo "$HELPER_REGISTERED" | grep -qw "${system_base}"; then
        STATUS="✅ REGISTERED (via helper)"
        METHODS="[M2]"
        REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
    # Method 3: Include check (check multiple header files)
    elif grep -q "#include.*${system}" "$MAIN_CPP" 2>/dev/null; then
        # Check if it's actually a system defined in a header file
        if grep -qE "struct ${system}[^a-zA-Z]" "src/systems/PostProcessingSystems.h" 2>/dev/null || \
           grep -qE "struct ${system}[^a-zA-Z]" "src/systems/RenderSystemHelpers.h" 2>/dev/null; then
            # Check if it's registered
            if grep -q "make_unique<${system}>" "$MAIN_CPP" 2>/dev/null; then
                STATUS="✅ REGISTERED (header file)"
                METHODS="[M1,M3]"
                REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
            else
                STATUS="⚠️  DEFINED in header but NOT REGISTERED"
                METHODS="[M3]"
            fi
        else
            STATUS="⚠️  INCLUDED (check usage)"
            METHODS="[M3]"
        fi
    # Method 4: Component check (for specific systems)
    elif [[ "$system" == "ProcessCollisionAbsorption" ]]; then
        if grep -q "CollisionAbsorber" src/components/*.h 2>/dev/null; then
            STATUS="❌ UNUSED (component exists)"
            METHODS="[M4]"
        else
            STATUS="❌ UNUSED (no component)"
            METHODS="[M4]"
        fi
    elif [[ "$system" == "UpdateTrackingEntities" ]]; then
        if grep -q "TracksEntity" src/components/*.h 2>/dev/null; then
            STATUS="❌ UNUSED (component exists)"
            METHODS="[M4]"
        else
            STATUS="❌ UNUSED (no component)"
            METHODS="[M4]"
        fi
    # Method 5: Planning docs check
    elif grep -qi "$system" $PLANNING_DOCS 2>/dev/null; then
        STATUS="📋 PLANNED (check docs)"
        METHODS="[M5]"
    # Method 6: Code reference check
    elif grep -rq "$system" src/ 2>/dev/null | grep -v "\.h:" | grep -v "\.cpp:"; then
        STATUS="⚠️  REFERENCED (check usage)"
        METHODS="[M6]"
    else
        STATUS="❌ UNUSED"
        METHODS="[M1-M6]"
        UNUSED_COUNT=$((UNUSED_COUNT + 1))
    fi
    
    echo "$STATUS $system $METHODS"
done

echo ""
echo "📈 Summary:"
echo "   Registered: $REGISTERED_COUNT"
echo "   Unused: $UNUSED_COUNT"
echo "   Utilities: $UTILITY_COUNT"
echo ""
echo "💡 Next steps:"
echo "   1. Review '❌ UNUSED' systems against planning docs (Method 5)"
echo "   2. Check '⚠️ INCLUDED/REFERENCED' systems for actual usage"
echo "   3. Verify '📋 PLANNED' systems are needed for future features"
echo ""
echo "📝 See scripts/verify_systems_plan.md for detailed verification methods"

