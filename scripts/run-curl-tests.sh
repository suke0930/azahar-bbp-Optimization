#!/bin/bash
# Remote Debug API curl test suite
# Usage: ./scripts/run-curl-tests.sh <windows-ip> [port]
# Example: ./scripts/run-curl-tests.sh 100.64.x.x 49355

set -euo pipefail

IP="${1:?Usage: $0 <windows-ip> [port]}"
PORT="${2:-49355}"
BASE="http://${IP}:${PORT}"
PASS=0
FAIL=0

green() { echo -e "\033[32m$1\033[0m"; }
red() { echo -e "\033[31m$1\033[0m"; }
yellow() { echo -e "\033[33m$1\033[0m"; }

check() {
    local desc="$1" expected="$2" actual="$3"
    if [ "$expected" = "$actual" ]; then
        green "  PASS: $desc"
        ((PASS++))
    else
        red "  FAIL: $desc (expected $expected, got $actual)"
        ((FAIL++))
    fi
}

check_body_not_contains() {
    local desc="$1" body="$2" pattern="$3"
    if ! echo "$body" | grep -q "$pattern"; then
        green "  PASS: $desc (no '$pattern')"
        ((PASS++))
    else
        red "  FAIL: $desc (contains '$pattern')"
        ((FAIL++))
    fi
}

echo "======================================"
green " Remote Debug API Test Suite"
echo " Target: $BASE"
echo "======================================"
echo ""

# A: Basic connectivity
echo "[A] GET /api/v1/emulator/status"
status=$(curl -s -o /dev/null -w "%{http_code}" "$BASE/api/v1/emulator/status")
check "HTTP 200" "200" "$status"

# B: Emulator control
echo "[B] POST /api/v1/emulator/control (pause/resume)"
status=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/api/v1/emulator/control" \
    -H "Content-Type: application/json" -d '{"action":"pause"}')
check "pause -> 200" "200" "$status"
status=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/api/v1/emulator/control" \
    -H "Content-Type: application/json" -d '{"action":"resume"}')
check "resume -> 200" "200" "$status"

# C: Speed setting
echo "[C] POST /api/v1/emulator/speed"
status=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/api/v1/emulator/speed" \
    -H "Content-Type: application/json" -d '{"speed_percent":200}')
check "speed 200% -> 200" "200" "$status"

# D: Save/load state
echo "[D] POST /api/v1/state/save + /api/v1/state/load"
status=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/api/v1/state/save" \
    -H "Content-Type: application/json" -d '{"slot":0}')
check "save slot 0 -> 200" "200" "$status"
status=$(curl -s -o /dev/null -w "%{http_code}" -X POST "$BASE/api/v1/state/load" \
    -H "Content-Type: application/json" -d '{"slot":0}')
check "load slot 0 -> 200" "200" "$status"

# E: JSON type mismatch -> 400, no _http_status leak
echo "[E] JSON type mismatch -> 400 + no _http_status leak"
resp=$(curl -s -w "\n%{http_code}" -X POST "$BASE/api/v1/emulator/speed" \
    -H "Content-Type: application/json" -d '{"speed_percent":"not_a_number"}')
code=$(echo "$resp" | tail -1)
body=$(echo "$resp" | head -n -1)
check "type_error -> 400" "400" "$code"
check_body_not_contains "_http_status in body" "$body" "_http_status"
check_body_not_contains "Internal server error" "$body" "Internal server error"

# F: 404 + no _http_status leak
echo "[F] GET /api/v1/nonexistent -> 404 + no _http_status leak"
resp=$(curl -s -w "\n%{http_code}" "$BASE/api/v1/nonexistent")
code=$(echo "$resp" | tail -1)
body=$(echo "$resp" | head -n -1)
check "nonexistent -> 404" "404" "$code"
check_body_not_contains "_http_status in body" "$body" "_http_status"

# G: JSON parse error -> 400 + no _http_status leak
echo "[G] JSON parse error -> 400 + no _http_status leak"
resp=$(curl -s -w "\n%{http_code}" -X POST "$BASE/api/v1/emulator/speed" \
    -H "Content-Type: application/json" -d '{invalid json')
code=$(echo "$resp" | tail -1)
body=$(echo "$resp" | head -n -1)
check "parse error -> 400" "400" "$code"
check_body_not_contains "_http_status in body" "$body" "_http_status"

echo ""
echo "======================================"
if [ "$FAIL" -eq 0 ]; then
    green " ALL $PASS TESTS PASSED"
else
    red " $PASS passed, $FAIL failed"
fi
echo "======================================"
exit "$FAIL"
