#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
TMP_DIR=${TMPDIR:-/tmp}/ylc_clap-deps

CLAP_REF=${CLAP_REF:-main}
REAPER_SDK_REF=${REAPER_SDK_REF:-main}
WDL_REF=${WDL_REF:-main}

CLAP_URL="https://codeload.github.com/free-audio/clap/tar.gz/refs/heads/${CLAP_REF}"
REAPER_SDK_URL="https://codeload.github.com/justinfrankel/reaper-sdk/tar.gz/refs/heads/${REAPER_SDK_REF}"
WDL_URL="https://codeload.github.com/justinfrankel/WDL/tar.gz/refs/heads/${WDL_REF}"

rm -rf "$TMP_DIR"
mkdir -p "$TMP_DIR" "$ROOT_DIR/include" "$ROOT_DIR/include/reaper" "$ROOT_DIR/include/WDL/swell"

echo "fetching CLAP SDK (${CLAP_REF})"
curl -L "$CLAP_URL" -o "$TMP_DIR/clap.tar.gz"
tar -xzf "$TMP_DIR/clap.tar.gz" -C "$TMP_DIR"
rm -rf "$ROOT_DIR/include/clap"
cp -R "$TMP_DIR/clap-${CLAP_REF}/include/clap" "$ROOT_DIR/include/clap"

echo "fetching REAPER SDK (${REAPER_SDK_REF})"
curl -L "$REAPER_SDK_URL" -o "$TMP_DIR/reaper-sdk.tar.gz"
tar -xzf "$TMP_DIR/reaper-sdk.tar.gz" -C "$TMP_DIR"
cp "$TMP_DIR/reaper-sdk-${REAPER_SDK_REF}/reaper-plugins/reaper_plugin.h" \
  "$ROOT_DIR/include/reaper/reaper_plugin.h"

echo "fetching WDL/SWELL (${WDL_REF})"
curl -L "$WDL_URL" -o "$TMP_DIR/wdl.tar.gz"
tar -xzf "$TMP_DIR/wdl.tar.gz" -C "$TMP_DIR"
rm -rf "$ROOT_DIR/include/WDL/swell"
mkdir -p "$ROOT_DIR/include/WDL/swell"
find "$TMP_DIR/WDL-${WDL_REF}/WDL/swell" -maxdepth 1 -type f -name '*.h' \
  -exec cp {} "$ROOT_DIR/include/WDL/swell/" \;

echo "vendored headers updated"
