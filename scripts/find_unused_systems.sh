#!/bin/bash

# Find Systems That Aren't Registered in main.cpp
# This script checks which system files exist but aren't registered

echo "🔍 Finding unregistered systems..."
echo ""

SYSTEMS_DIR="src/systems"
MAIN_CPP="src/main.cpp"

# Get all system header files
ALL_SYSTEMS=$(find "$SYSTEMS_DIR" -name "*.h" -type f | sed 's|.*/||' | sed 's|\.h||' | sort)

# Get systems registered in main.cpp
# Extract system names from patterns like: make_unique<SystemName>()
REGISTERED=$(grep -oE "make_unique<[^>]*System>" "$MAIN_CPP" | \
    sed 's/make_unique<//' | \
    sed 's/System>//' | \
    sed 's/>//' | \
    awk '{print $0 "System"}' | \
    sort | uniq)

echo "📊 System Registration Status:"
echo ""

UNREGISTERED_COUNT=0
REGISTERED_COUNT=0

for system in $ALL_SYSTEMS; do
    # Skip TestSystem (special case)
    if [[ "$system" == "TestSystem" ]]; then
        continue
    fi
    
    # Check if registered
    if echo "$REGISTERED" | grep -q "^${system}$"; then
        echo "✅ $system"
        REGISTERED_COUNT=$((REGISTERED_COUNT + 1))
    else
        echo "❌ $system (NOT REGISTERED)"
        UNREGISTERED_COUNT=$((UNREGISTERED_COUNT + 1))
    fi
done

echo ""
echo "📈 Summary:"
echo "   Registered: $REGISTERED_COUNT"
echo "   Unregistered: $UNREGISTERED_COUNT"
echo ""
echo "💡 Unregistered systems may be dead code or intentionally unused"
echo "💡 Check each system to determine if it should be removed or registered"

