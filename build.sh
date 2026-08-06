#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

if [[ $# -gt 0 && "$1" != "clean" ]]; then
    echo "Unknown argument: $1"
    exit 1
elif [[ "$1" == "clean" ]]; then
    echo "Cleaning..."
    rm -rf "${BUILD_DIR}"
fi

if [[ ! -d "${BUILD_DIR}" ]]; then
    echo "Creating build/ dir..."
    mkdir -p "${BUILD_DIR}"
fi

cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j"$(nproc)"