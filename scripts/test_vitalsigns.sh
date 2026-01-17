#!/bin/bash
# Quick test script for vital signs firmware

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG_FILE="${SCRIPT_DIR}/../configs/chirp_profiles/vital_signs_2m.cfg"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${CYAN}================================${NC}"
echo -e "${CYAN} IWR6843 Vital Signs Test${NC}"
echo -e "${CYAN}================================${NC}"

# Check for serial ports
if [ -e /dev/ttyUSB0 ] && [ -e /dev/ttyUSB1 ]; then
    CLI_PORT="/dev/ttyUSB0"
    DATA_PORT="/dev/ttyUSB1"
    echo -e "${GREEN}Found ports: CLI=$CLI_PORT, Data=$DATA_PORT${NC}"
elif [ -e /dev/ttyACM0 ] && [ -e /dev/ttyACM1 ]; then
    CLI_PORT="/dev/ttyACM0"
    DATA_PORT="/dev/ttyACM1"
    echo -e "${GREEN}Found ports: CLI=$CLI_PORT, Data=$DATA_PORT${NC}"
else
    echo -e "${RED}Error: Could not find serial ports${NC}"
    echo "Please ensure device is connected"
    exit 1
fi

# Check if config file exists
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${RED}Config file not found: $CONFIG_FILE${NC}"
    echo "Running without config..."
    CONFIG_ARG=""
else
    echo -e "${GREEN}Using config: $CONFIG_FILE${NC}"
    CONFIG_ARG="--config $CONFIG_FILE"
fi

echo ""
echo -e "${CYAN}Starting monitor...${NC}"
echo "Press Ctrl+C to stop"
echo ""

# Run the monitor
python3 "${SCRIPT_DIR}/vs_monitor.py" \
    --cli "$CLI_PORT" \
    --data "$DATA_PORT" \
    $CONFIG_ARG \
    "$@"
