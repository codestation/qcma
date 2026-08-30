#!/bin/bash

set -euo pipefail

if [[ $# -ne 1 || ! -d $1/Contents ]]; then
    echo "usage: $0 Qcma.app" >&2
    exit 2
fi

app=$1
report=$(mktemp -t qcma-otool.XXXXXX)
trap 'rm -f -- "$report"' EXIT HUP INT TERM

while IFS= read -r -d '' file_path; do
    file_info=$(file -b "$file_path")
    if [[ $file_info == *Mach-O* ]]; then
        printf '%s: %s\n' "$file_path" "$file_info"
        if [[ $file_info != *arm64* ]]; then
            echo "non-arm64 Mach-O file: $file_path" >&2
            exit 1
        fi
        otool -L "$file_path" >> "$report"
        if command -v vtool >/dev/null 2>&1; then
            if ! vtool -show-build "$file_path" 2>/dev/null | awk '
                $1 == "minos" {
                    split($2, v, ".")
                    if ((v[1] + 0) > 13) exit 1
                }
            '; then
                echo "deployment target newer than macOS 13: $file_path" >&2
                exit 1
            fi
        fi
    fi
done < <(find "$app" -type f -print0)

if grep -E '/opt/homebrew|/usr/local/(Cellar|opt)|vitamtp-prefix' "$report"; then
    echo 'The app bundle contains non-portable Homebrew/build paths.' >&2
    exit 1
fi

codesign --verify --deep --strict --verbose=2 "$app"
echo 'macOS bundle validation passed.'
