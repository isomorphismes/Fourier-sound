#!/bin/sh
set -eu

url="${1:-https://www.youtube.com/watch?v=NcqSDIGU02I}"
out="${2:-references/diproton}"

mkdir -p "$out"

yt-dlp \
  --skip-download \
  --write-subs \
  --write-auto-subs \
  --sub-langs 'en.*' \
  --sub-format vtt \
  -o "$out/%(id)s.%(ext)s" \
  "$url"
