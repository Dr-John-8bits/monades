#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
: "${JIELI_TOOLCHAIN:?Set JIELI_TOOLCHAIN}"
: "${AC79_SDK:?Set AC79_SDK}"
exec "${PYTHON:-python3}" base/tools/build.py app --release 0.1.5 # Monades 0.1.5: identity FM-1_714000105
