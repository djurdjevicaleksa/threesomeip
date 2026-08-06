#!/bin/env bash
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

mkdir ${SCRIPT_DIR}/build; cd ${SCRIPT_DIR}/build && cmake -B . -S .. && cmake --build .