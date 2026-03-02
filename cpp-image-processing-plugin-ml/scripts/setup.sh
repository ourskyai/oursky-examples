#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

SNPE_VERSION="2.28.2.241116"
SNPE_SDK_DIR="${PROJECT_ROOT}/third-party/snpe-sdk/${SNPE_VERSION}"
HTP_SOC="sm8550"
SETUP_IMAGE="snpe-inception-setup"
RUNTIME_IMAGE="inception-v3-snpe-run"

# ── (a) Check SNPE SDK ──────────────────────────────────────────────
echo "=== Checking prerequisites ==="

if [ ! -d "$SNPE_SDK_DIR" ]; then
    echo "ERROR: SNPE SDK not found at:"
    echo "  ${SNPE_SDK_DIR}"
    echo ""
    echo "Download the SNPE SDK and extract it to:"
    echo "  third-party/snpe-sdk/${SNPE_VERSION}"
    exit 1
fi

if [ ! -f "${SNPE_SDK_DIR}/bin/envsetup.sh" ]; then
    echo "ERROR: SNPE SDK appears incomplete (missing bin/envsetup.sh)"
    exit 1
fi

echo "SNPE SDK: ${SNPE_SDK_DIR}"

# ── (b) Build model conversion image ────────────────────────────────
echo ""
echo "=== Building setup Docker image (${SETUP_IMAGE}) ==="
docker build -f "${PROJECT_ROOT}/docker/Dockerfile.setup" \
    -t "$SETUP_IMAGE" \
    "${PROJECT_ROOT}/docker"

# ── (c) Run InceptionV3 setup inside container ──────────────────────
#
# Uses the official Qualcomm setup script which:
#   1. Downloads the InceptionV3 TensorFlow frozen graph
#   2. Preprocesses sample images (center-crop, resize 299x299, normalize)
#   3. Converts TF frozen graph to SNPE DLC (float32 for CPU)
#   4. Quantizes the DLC to INT8 (for DSP)
#   5. Generates HTP offline cache for the target SoC
#
echo ""
echo "=== Running InceptionV3 setup (CPU + DSP for ${HTP_SOC}) ==="

docker run --rm \
    --user "$(id -u):$(id -g)" \
    -e HOME=/tmp \
    -e TENSORFLOW_HOME=/usr/local/lib/python3.10/dist-packages/tensorflow \
    -v "${SNPE_SDK_DIR}:/opt/snpe-sdk" \
    -w /opt/snpe-sdk/examples/Models/InceptionV3/scripts \
    "$SETUP_IMAGE" \
    bash -c '
        source /opt/snpe-sdk/bin/envsetup.sh && \
        python3 setup_inceptionv3_snpe.py \
            -a /tmp/inception_assets \
            -d \
            -r dsp \
            -l '"${HTP_SOC}"'
    '

# ── (d) Copy outputs to prerequisites/ ──────────────────────────────
echo ""
echo "=== Copying model artifacts to prerequisites/ ==="

MODELS_DIR="${PROJECT_ROOT}/prerequisites/models"
mkdir -p "$MODELS_DIR"

DLC_DIR="${SNPE_SDK_DIR}/examples/Models/InceptionV3/dlc"
DATA_DIR="${SNPE_SDK_DIR}/examples/Models/InceptionV3/data"

GENERATED=()
MISSING=()

for artifact in \
    "inception_v3.dlc|CPU model (float32)|${DLC_DIR}|${MODELS_DIR}" \
    "inception_v3_quantized.dlc|DSP model (${HTP_SOC}, INT8 + HTP cache)|${DLC_DIR}|${MODELS_DIR}" \
    "imagenet_slim_labels.txt|ImageNet labels (1001 classes)|${DATA_DIR}|${PROJECT_ROOT}/prerequisites"; do

    IFS='|' read -r filename desc src_dir dst_dir <<< "$artifact"

    if [ -f "${src_dir}/${filename}" ]; then
        cp "${src_dir}/${filename}" "${dst_dir}/"
        GENERATED+=("${dst_dir}/${filename}|${desc}")
    else
        MISSING+=("${filename}")
    fi
done

if [ ${#MISSING[@]} -gt 0 ]; then
    echo ""
    echo "ERROR: The following expected artifacts were not produced:"
    for m in "${MISSING[@]}"; do
        echo "  - ${m}"
    done
    exit 1
fi

# ── (e) Build arm64 runtime image and export tar ────────────────────
echo ""
echo "=== Building arm64 runtime image (${RUNTIME_IMAGE}) ==="

docker buildx build --platform linux/arm64 \
    -f "${PROJECT_ROOT}/docker/Dockerfile.snpe-run" \
    -t "$RUNTIME_IMAGE" \
    --load \
    "$PROJECT_ROOT"

TAR_PATH="${MODELS_DIR}/${RUNTIME_IMAGE}.tar"
docker save "$RUNTIME_IMAGE" -o "$TAR_PATH"
GENERATED+=("${TAR_PATH}|Runtime image (arm64, CPU + DSP)")

# ── Summary ──────────────────────────────────────────────────────────
echo ""
echo "=== Setup complete ==="
echo ""
echo "Generated files:"
for entry in "${GENERATED[@]}"; do
    IFS='|' read -r path desc <<< "$entry"
    size=$(du -h "$path" | cut -f1)
    echo "  ${path}"
    echo "    ${desc} (${size})"
done
echo ""
echo "To run on device:"
echo "  docker load -i ${TAR_PATH}"
echo "  docker run -p 8099:8099 ${RUNTIME_IMAGE}"
echo ""
echo "  # For DSP inference on sm8550:"
echo "  docker run -p 8099:8099 --device /dev/adsprpc-smd \\"
echo "    -e MODEL_PATH=/opt/models/inception_v3_quantized.dlc \\"
echo "    -e SNPE_RUNTIME=dsp ${RUNTIME_IMAGE}"
