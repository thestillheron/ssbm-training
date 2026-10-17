#!/usr/bin/env bash
# Makes rtc-training.iso next to this script from a vanilla Melee v1.02 NTSC ISO:
#   ./build_linux_and_mac.sh path/to/melee.iso
# Your ISO is only read, never changed. Modelled on TM-CE's script.

SCRIPT_DIR=$(cd "$(dirname "$0")" && pwd)
XDELTA_PATCH_PATH="$SCRIPT_DIR/patch.xdelta"
OUTPUT="$SCRIPT_DIR/rtc-training.iso"

# xdelta3 only: the old xdelta (v1) binary cannot apply xdelta3 patches.
if ! command -v xdelta3 > /dev/null; then
    echo "ERROR: xdelta3 is not installed. Install it with: brew install xdelta (macOS) or apt install xdelta3 (Debian/Ubuntu)."
    exit 1
fi

if [[ -z "${1}" ]]; then
    echo "ERROR: ISO was not passed. Usage: ${0} path/to/melee.iso"
    exit 1
fi

if [[ ! -f "${1}" ]]; then
    echo "ERROR: ISO '${1}' is not a valid file"
    exit 1
fi

if ! xdelta3 -f -d -s "${1}" "${XDELTA_PATCH_PATH}" "${OUTPUT}"; then
    rm -f "${OUTPUT}"
    echo "ERROR: The ISO '${1}' is not a valid v1.02 NTSC melee iso"
    exit 1
fi

echo "############ rtc-training.iso has been successfully created ############"
echo "${OUTPUT}"
