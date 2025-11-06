# Battle Server Verification

## Overview

The `verify_battle_endpoint.sh` script tests the `/battle` endpoint to ensure it:
- Responds to requests correctly
- Returns valid JSON with all required fields
- Handles debug mode properly
- Validates error responses

## Prerequisites

1. **Start the battle server:**
   ```bash
   ./output/battle_server.exe
   ```

2. **Ensure opponent files exist:**
   ```bash
   # Check if opponent directory has files
   ls resources/battles/opponents/
   
   # If empty, create a test opponent:
   cp resources/battles/opponent_sample.json resources/battles/opponents/test_opponent.json
   ```

3. **Install dependencies:**
   - `curl` (usually pre-installed)
   - `jq` (install with `brew install jq` on macOS)

## Usage

```bash
# Test on default port (8080)
./scripts/verify_battle_endpoint.sh

# Test on custom port
./scripts/verify_battle_endpoint.sh 3000
```

## What It Tests

1. **Health Check** - Verifies `/health` endpoint responds
2. **Battle Request** - Sends POST to `/battle` with test team
3. **Response Structure** - Validates presence of:
   - `seed` (number)
   - `opponentId` (string)
   - `outcomes` (array)
   - `events` (array)
   - `checksum` (string)
4. **Debug Mode** - Tests with `debug: true` to verify snapshots included
5. **Production Mode** - Tests with `debug: false` to verify snapshots excluded
6. **Error Handling** - Tests invalid team JSON returns HTTP 400

## Expected Output

If all tests pass, you'll see:
```
=========================================
✅ All verification tests passed!
=========================================
```

If any test fails, the script will exit with an error code and show what failed.

