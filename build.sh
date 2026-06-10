#!/usr/bin/env bash
set -euo pipefail

ScriptDir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BuildDir="${ScriptDir}/build"
BuildType="${1:-Release}"

cmake -S "${ScriptDir}" -B "${BuildDir}" -DCMAKE_BUILD_TYPE="${BuildType}"
cmake --build "${BuildDir}" --parallel "$(nproc)"

echo "Built: ${BuildDir}/UnknownTxd"
