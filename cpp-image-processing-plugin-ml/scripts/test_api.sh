#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

BASE_URL="${BASE_URL:-http://localhost:8099}"
TEST_IMAGE="${TEST_IMAGE:-/tmp/test_image.fits}"

# Generate a test FITS file if one doesn't already exist
if [ ! -f "$TEST_IMAGE" ]; then
    echo "Generating test FITS image..."
    python3 "${SCRIPT_DIR}/generate_test_fits.py" "$TEST_IMAGE" 512 512
    echo ""
fi

echo "Image: $TEST_IMAGE"
echo ""

code=$(curl -s -o /tmp/_body -w '%{http_code}' \
    -X POST "${BASE_URL}/custom-image-processing/v1/images" \
    -H 'Content-Type: application/json' \
    -d "{
        \"rawImagePath\": \"${TEST_IMAGE}\",
        \"timeoutSeconds\": 30.0
    }")
body=$(cat /tmp/_body)
cat /tmp/_body

if [ "$code" -ne 200 ]; then
    echo "FAILED (HTTP $code)"
    echo "$body"
    exit 1
fi

echo "Results:"
python3 -c "
import sys, json
data = json.loads(sys.argv[1])
for r in data.get('results', []):
    print(f'  {r[\"confidence\"]:8.4f}  {r[\"label\"]}')
" "$body"

rm -f /tmp/_body
