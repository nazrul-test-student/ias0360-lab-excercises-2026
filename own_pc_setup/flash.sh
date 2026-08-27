#!/usr/bin/env bash
set -euo pipefail

MAX_ATTEMPTS=3
RETRY_DELAY=2

if [[ $# -ne 1 ]]; then
    echo "Usage: $0 <path-to-uf2-file>"
    exit 1
fi

UF2_PATH="$1"

if [[ ! -f "${UF2_PATH}" ]]; then
    echo "File not found: ${UF2_PATH}"
    exit 1
fi

for attempt in $(seq 1 "${MAX_ATTEMPTS}"); do
    echo "Flash attempt ${attempt}/${MAX_ATTEMPTS}..."
    if picotool load "${UF2_PATH}" -f -x; then
        echo "Flash succeeded."
        exit 0
    fi

    if [[ "${attempt}" -lt "${MAX_ATTEMPTS}" ]]; then
        echo "Retrying in ${RETRY_DELAY}s..."
        sleep "${RETRY_DELAY}"
    fi
done

echo "Flash failed after ${MAX_ATTEMPTS} attempts."
exit 1
