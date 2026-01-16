#!/usr/bin/env bash
set -euo pipefail

PORT=${1:-/dev/ttyUSB1}
CFG=${2:-/home/baxter/re/packages/ti/demo/xwr68xx/mmw/profiles/profile_2d.cfg}
BAUD=${3:-115200}

if [[ ! -e "$PORT" ]]; then
  echo "Serial port not found: $PORT" >&2
  exit 1
fi

if [[ ! -f "$CFG" ]]; then
  echo "Config file not found: $CFG" >&2
  exit 1
fi

stty -F "$PORT" "$BAUD" cs8 -cstopb -parenb -ixon -ixoff -echo -icanon -isig -opost

while IFS= read -r line || [[ -n "$line" ]]; do
  # Skip empty lines and lines starting with '%'
  if [[ -z "$line" || "${line:0:1}" == "%" ]]; then
    continue
  fi
  printf '%s\r\n' "$line" > "$PORT"
  sleep 0.02
done < "$CFG"

echo "Config sent to $PORT from $CFG"
