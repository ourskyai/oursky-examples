#!/bin/bash
ADSPRPC=$(ldconfig -p | awk '/libadsprpc\.so/{print $NF; exit}')
CDSPRPC=$(ldconfig -p | awk '/libcdsprpc\.so/{print $NF; exit}')
LOGSO=$(ldconfig -p | awk '/liblog\.so\.0/{print $NF; exit}')
CUTILS=$(ldconfig -p | awk '/libcutils\.so\.0/{print $NF; exit}')
ION=$(ldconfig -p | awk '/libion\.so\.0/{print $NF; exit}')
DMABUF=$(ldconfig -p | awk '/libdmabufheap\.so\.0/{print $NF; exit}')
VMMEM=$(ldconfig -p | awk '/libvmmem\.so\.0/{print $NF; exit}')
BASE=$(ldconfig -p | awk '/libbase\.so\.0/{print $NF; exit}')

docker run --rm -it   \
  --privileged   \
  --device /dev/adsprpc\
  -v /dsp:/dsp   \
  -v /tmp/snpe-run:/tmp/snpe-run   \
  -v "$ADSPRPC":/usr/lib/libadsprpc.so:ro   \
  -v "$CDSPRPC":/usr/lib/libcdsprpc.so:ro   \
  -v "$LOGSO":/usr/lib/liblog.so.0:ro   \
  -v "$CUTILS":/usr/lib/libcutils.so.0:ro   \
  -v "$ION":/usr/lib/libion.so.0:ro   \
  -v "$DMABUF":/usr/lib/libdmabufheap.so.0:ro   \
  -v "$VMMEM":/usr/lib/libvmmem.so.0:ro   \
  -v "$BASE":/usr/lib/libbase.so.0:ro   \
  -e LD_LIBRARY_PATH="/usr/lib:/tmp/snpe-run"   \
  -e ADSP_LIBRARY_PATH="/dsp/cdsp;/dsp/adsp"   \
  inception-v3-snpe-run: bash
