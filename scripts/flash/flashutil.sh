#!/bin/bash
#
# flashutil.sh - Flash firmware to IWR6843AOPEVM
#
# Usage:
#   flashutil.sh [options] [firmware.bin]
#
# Examples:
#   flashutil.sh                                    # Flash default VS firmware
#   flashutil.sh firmware/custom/my_build.bin      # Flash specific file
#   flashutil.sh -p /dev/ttyUSB1 firmware.bin      # Use different port
#   flashutil.sh --detect                          # Check device connection
#

set -euo pipefail

# =============================================================================
# Configuration
# =============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

UNIFLASH_DIR="$PROJECT_ROOT/vendor/uniflash"
DSLITE="$UNIFLASH_DIR/dslite.sh"
CCXML="$PROJECT_ROOT/configs/uniflash_iwr6843aop_serial.ccxml"

DEFAULT_PORT="/dev/ttyUSB0"
DEFAULT_FIRMWARE="$PROJECT_ROOT/firmware/custom/iwr6843aop_mmw_vitalsigns.bin"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color
BOLD='\033[1m'

# =============================================================================
# Helper Functions
# =============================================================================

print_banner() {
    echo -e "${CYAN}"
    echo "  ╔═══════════════════════════════════════════╗"
    echo "  ║           flashutil - IWR6843AOP          ║"
    echo "  ║         Firmware Flash Utility            ║"
    echo "  ╚═══════════════════════════════════════════╝"
    echo -e "${NC}"
}

print_help() {
    echo -e "${BOLD}USAGE${NC}"
    echo "    flashutil.sh [OPTIONS] [FIRMWARE_FILE]"
    echo ""
    echo -e "${BOLD}DESCRIPTION${NC}"
    echo "    Flash firmware to IWR6843AOPEVM via UART bootloader."
    echo ""
    echo "    Before flashing, ensure the device is in flash programming mode:"
    echo "      1. Set SOP jumpers to 001 (SOP0=1, SOP1=0, SOP2=0)"
    echo "      2. Power cycle the board"
    echo "      3. Run this script"
    echo ""
    echo -e "${BOLD}OPTIONS${NC}"
    echo "    -p, --port PORT     Serial port (default: $DEFAULT_PORT)"
    echo "    -f, --file FILE     Firmware file to flash"
    echo "    -d, --detect        Detect device and check connection"
    echo "    -l, --list-ports    List available serial ports"
    echo "    -v, --verify        Verify after flashing"
    echo "    -n, --dry-run       Show what would be done without flashing"
    echo "    -h, --help          Show this help message"
    echo ""
    echo -e "${BOLD}EXAMPLES${NC}"
    echo "    # Flash the default vital signs firmware"
    echo "    flashutil.sh"
    echo ""
    echo "    # Flash a specific firmware file"
    echo "    flashutil.sh firmware/custom/my_build.bin"
    echo ""
    echo "    # Flash using a different serial port"
    echo "    flashutil.sh -p /dev/ttyUSB1"
    echo ""
    echo "    # Check if device is connected and ready"
    echo "    flashutil.sh --detect"
    echo ""
    echo "    # Dry run - show command without executing"
    echo "    flashutil.sh --dry-run"
    echo ""
    echo -e "${BOLD}DEVICE SETUP${NC}"
    echo "    SOP Pin Configuration:"
    echo "    ┌──────┬──────┬──────┬─────────────────────────┐"
    echo "    │ SOP2 │ SOP1 │ SOP0 │ Mode                    │"
    echo "    ├──────┼──────┼──────┼─────────────────────────┤"
    echo "    │  0   │  0   │  0   │ Functional (run flash)  │"
    echo "    │  0   │  0   │  1   │ Flash Programming Mode  │"
    echo "    └──────┴──────┴──────┴─────────────────────────┘"
    echo ""
    echo "    After flashing, set SOP back to 000 and power cycle."
    echo ""
}

log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[OK]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

check_prerequisites() {
    local has_error=0

    # Check UniFlash installation
    if [[ ! -f "$DSLITE" ]]; then
        log_error "UniFlash not found at: $UNIFLASH_DIR"
        log_info "Ensure vendor/uniflash symlink points to UniFlash installation"
        has_error=1
    fi

    # Check ccxml config
    if [[ ! -f "$CCXML" ]]; then
        log_error "CCXML config not found at: $CCXML"
        has_error=1
    fi

    # Check serial port permissions
    if [[ -e "$PORT" ]]; then
        if [[ ! -r "$PORT" ]] || [[ ! -w "$PORT" ]]; then
            log_error "No read/write permission for $PORT"
            log_info "Try: sudo usermod -a -G dialout \$USER (then re-login)"
            has_error=1
        fi
    fi

    return $has_error
}

list_serial_ports() {
    echo -e "${BOLD}Available serial ports:${NC}"
    echo ""

    # List USB serial ports
    if ls /dev/ttyUSB* 2>/dev/null | head -10; then
        echo ""
    else
        echo "  (no /dev/ttyUSB* devices found)"
    fi

    # List ACM ports (some USB-serial adapters)
    if ls /dev/ttyACM* 2>/dev/null | head -10; then
        echo ""
    fi

    # Show which ports are likely IWR6843
    echo -e "${BOLD}Device identification:${NC}"
    local ports
    ports=$(ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true)
    if [[ -n "$ports" ]]; then
        for port in $ports; do
            if [[ -e "$port" ]]; then
                # Try to get device info via udevadm
                local info
                info=$(udevadm info --query=property --name="$port" 2>/dev/null | grep -E "ID_MODEL=|ID_VENDOR=" | head -2)
                if [[ -n "$info" ]]; then
                    echo "  $port:"
                    echo "$info" | sed 's/^/    /'
                fi
            fi
        done
    else
        echo "  (no serial ports found)"
    fi
}

detect_device() {
    echo -e "${BOLD}Detecting IWR6843AOPEVM...${NC}"
    echo ""

    # Check if port exists
    if [[ ! -e "$PORT" ]]; then
        log_error "Serial port $PORT does not exist"
        echo ""
        list_serial_ports
        return 1
    fi

    log_success "Serial port $PORT exists"

    # Check permissions
    if [[ -r "$PORT" ]] && [[ -w "$PORT" ]]; then
        log_success "Serial port permissions OK"
    else
        log_error "Cannot read/write $PORT - check permissions"
        return 1
    fi

    # Check if port is in use
    if fuser "$PORT" 2>/dev/null | grep -q .; then
        log_warn "Serial port $PORT is in use by another process"
        fuser -v "$PORT" 2>&1 | head -5
    else
        log_success "Serial port is available"
    fi

    echo ""
    echo -e "${BOLD}Device info:${NC}"
    udevadm info --query=property --name="$PORT" 2>/dev/null | grep -E "ID_MODEL=|ID_VENDOR=|ID_SERIAL=" | sed 's/^/  /'

    echo ""
    echo -e "${YELLOW}Note:${NC} Ensure device is in flash programming mode (SOP=001)"

    return 0
}

flash_firmware() {
    local firmware="$1"
    local verify_flag="$2"

    echo -e "${BOLD}Flashing firmware to IWR6843AOPEVM${NC}"
    echo ""
    echo "  Port:     $PORT"
    echo "  Firmware: $firmware"
    echo "  Size:     $(du -h "$firmware" | cut -f1)"
    echo ""

    # Use direct Python flash script (bypasses DSLite file order bug)
    local flash_script="$SCRIPT_DIR/flash_direct.py"

    if [[ ! -f "$flash_script" ]]; then
        log_error "Flash script not found: $flash_script"
        return 1
    fi

    local cmd="python3 $flash_script -p $PORT"

    if [[ "$DRY_RUN" == "true" ]]; then
        echo -e "${YELLOW}[DRY RUN] Would execute:${NC}"
        echo "  $cmd $firmware"
        return 0
    fi

    echo -e "${CYAN}Starting flash process...${NC}"
    echo ""

    # Execute flash command
    if $cmd "$firmware"; then
        return 0
    else
        echo ""
        log_error "Flash failed!"
        echo ""
        echo -e "${YELLOW}Troubleshooting:${NC}"
        echo "  - Verify SOP jumpers are set to 001"
        echo "  - Power cycle the device and try again"
        echo "  - Check serial port permissions"
        echo "  - Ensure no other program is using $PORT"
        return 1
    fi
}

# =============================================================================
# Argument Parsing
# =============================================================================

PORT="$DEFAULT_PORT"
FIRMWARE=""
VERIFY="false"
DRY_RUN="false"
ACTION="flash"  # flash, detect, list-ports, help

while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--port)
            PORT="$2"
            shift 2
            ;;
        -f|--file)
            FIRMWARE="$2"
            shift 2
            ;;
        -d|--detect)
            ACTION="detect"
            shift
            ;;
        -l|--list-ports)
            ACTION="list-ports"
            shift
            ;;
        -v|--verify)
            VERIFY="true"
            shift
            ;;
        -n|--dry-run)
            DRY_RUN="true"
            shift
            ;;
        -h|--help)
            ACTION="help"
            shift
            ;;
        -*)
            log_error "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
        *)
            # Positional argument - treat as firmware file
            FIRMWARE="$1"
            shift
            ;;
    esac
done

# =============================================================================
# Main
# =============================================================================

case $ACTION in
    help)
        print_banner
        print_help
        exit 0
        ;;
    list-ports)
        print_banner
        list_serial_ports
        exit 0
        ;;
    detect)
        print_banner
        detect_device
        exit $?
        ;;
    flash)
        print_banner

        # Use default firmware if not specified
        if [[ -z "$FIRMWARE" ]]; then
            FIRMWARE="$DEFAULT_FIRMWARE"
            log_info "Using default firmware: $FIRMWARE"
        fi

        # Verify firmware file exists
        if [[ ! -f "$FIRMWARE" ]]; then
            log_error "Firmware file not found: $FIRMWARE"
            exit 1
        fi

        # Check prerequisites
        if ! check_prerequisites; then
            log_error "Prerequisites check failed"
            exit 1
        fi

        # Flash the firmware
        flash_firmware "$FIRMWARE" "$VERIFY"
        exit $?
        ;;
esac
