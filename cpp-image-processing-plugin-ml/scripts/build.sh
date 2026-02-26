#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

RUNTIME_IMAGE="inception-v3-snpe-run"
TAR_PATH="${PROJECT_ROOT}/prerequisites/models/${RUNTIME_IMAGE}.tar"

echo "=== Building arm64 runtime image (${RUNTIME_IMAGE}) ==="

docker buildx build --platform linux/arm64 \
    -f "${PROJECT_ROOT}/docker/Dockerfile.snpe-run" \
    -t "$RUNTIME_IMAGE" \
    --load \
    "$PROJECT_ROOT"

echo ""
echo "=== Exporting image tar ==="

mkdir -p "$(dirname "$TAR_PATH")"
docker save "$RUNTIME_IMAGE" -o "$TAR_PATH"

size=$(du -h "$TAR_PATH" | cut -f1)
echo ""
echo "Done: ${TAR_PATH} (${size})"
echo ""
echo "To load on device:"
echo "  docker load -i ${TAR_PATH}"
echo "  docker run -p 8099:8099 ${RUNTIME_IMAGE}"