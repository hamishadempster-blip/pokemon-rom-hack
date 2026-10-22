#!/bin/sh
# Build a BPS or UPS patch from a clean Emerald ROM to pokeemerald.gba.
# The clean ROM and the built ROM stay out of git.
set -eu

CLEAN_SHA256=a9dec84dfe7f62ab2220bafaef7479da0929d066ece16a6885f6226db19085af
CLEAN_SHA1=f3ae088181bf583e55daf962a92bb46f4f1d07b7
CLEAN=""
OUT="${1:-emerald-coop.bps}"

for candidate in \
    "rom/Pokemon - Emerald Version (USA, Europe).gba" \
    rom/baserom.gba \
    rom/*.gba \
    pokeemerald.gba
do
    for path in $candidate; do
        if [ -f "$path" ]; then
            sum=$(sha256sum "$path" | awk '{print $1}')
            if [ "$sum" = "$CLEAN_SHA256" ]; then
                CLEAN=$path
                break 2
            fi
        fi
    done
done

if [ -z "$CLEAN" ]; then
    echo "Put the clean USA/Europe Emerald ROM in rom/."
    echo "It must be 16,777,216 bytes."
    echo "SHA-256 ${CLEAN_SHA256}"
    echo "SHA-1   ${CLEAN_SHA1}"
    exit 1
fi

sha1=$(sha1sum "$CLEAN" | awk '{print $1}')
if [ "$sha1" != "$CLEAN_SHA1" ]; then
    echo "SHA-256 matched but SHA-1 did not: ${sha1}"
    exit 1
fi

if [ ! -f pokeemerald.gba ]; then
    echo "pokeemerald.gba is missing. Build with make -j\$(nproc) first."
    exit 1
fi

if command -v flips >/dev/null 2>&1; then
    flips --create --bps "$CLEAN" pokeemerald.gba "$OUT"
    echo "Wrote ${OUT}"
    exit 0
fi

if command -v beat >/dev/null 2>&1; then
    UPS="${OUT%.bps}.ups"
    beat -C "$CLEAN" pokeemerald.gba "$UPS"
    echo "Wrote ${UPS}"
    exit 0
fi

echo "The ROM hash checks passed. No patch tool is installed."
echo "Install either of these, then run this script again:"
echo "  flips: https://github.com/Alcaro/flips  (flips --create --bps clean.gba pokeemerald.gba emerald-coop.bps)"
echo "  beat:  a UPS tool          (beat -C clean.gba pokeemerald.gba emerald-coop.ups)"
echo "Do not distribute the ROM or pokeemerald.gba. Distribute the patch only."
exit 2
