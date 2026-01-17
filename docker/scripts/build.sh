#!/bin/bash
# =============================================================================
# Build script for Docker container
# =============================================================================

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

echo -e "${CYAN}"
echo "  ╔═══════════════════════════════════════════════════════╗"
echo "  ║     IWR6843AOPEVM Build Environment (Docker)          ║"
echo "  ╚═══════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check toolchain availability
check_toolchain() {
    local name=$1
    local path=$2
    if [[ -d "$path" ]]; then
        echo -e "${GREEN}[OK]${NC} $name"
    else
        echo -e "${RED}[MISSING]${NC} $name: $path"
        return 1
    fi
}

echo -e "${YELLOW}Checking toolchains...${NC}"
MISSING=0
check_toolchain "ARM Compiler" "$R4F_CODEGEN_INSTALL_PATH" || MISSING=1
check_toolchain "DSP Compiler" "$C674_CODEGEN_INSTALL_PATH" || MISSING=1
check_toolchain "XDC Tools" "$XDC_INSTALL_PATH" || MISSING=1
check_toolchain "SYS/BIOS" "$BIOS_INSTALL_PATH" || MISSING=1
check_toolchain "mmWave SDK" "$MMWAVE_SDK_INSTALL_PATH" || MISSING=1
check_toolchain "DSPLib C674x" "$C674x_DSPLIB_INSTALL_PATH" || MISSING=1
check_toolchain "MathLib C674x" "$C674x_MATHLIB_INSTALL_PATH" || MISSING=1

if [[ $MISSING -eq 1 ]]; then
    echo ""
    echo -e "${RED}ERROR: Missing toolchains!${NC}"
    echo ""
    echo "Ensure toolchains are mounted at /opt/ti"
    echo "On Mac/Linux, set TOOLCHAIN_PATH environment variable:"
    echo "  export TOOLCHAIN_PATH=/path/to/ti/toolchains"
    exit 1
fi

# Check for radarss firmware
if [[ ! -f "$XWR68XX_RADARSS_IMAGE_BIN" ]]; then
    echo -e "${YELLOW}[WARN]${NC} RadarSS firmware not found at $XWR68XX_RADARSS_IMAGE_BIN"
    echo "       Looking in alternative locations..."

    ALT_RADARSS="/opt/ti/packages/../firmware/radarss/xwr6xxx_radarss_rprc.bin"
    if [[ -f "$ALT_RADARSS" ]]; then
        export XWR68XX_RADARSS_IMAGE_BIN="$ALT_RADARSS"
        echo -e "${GREEN}[OK]${NC} Found RadarSS at $ALT_RADARSS"
    fi
fi

echo ""

# Build parameters
VITAL_SIGNS=${VITAL_SIGNS:-1}
USE_LOCAL_MATHLIB=${USE_LOCAL_MATHLIB:-1}
USE_LOCAL_MATHUTILS=${USE_LOCAL_MATHUTILS:-1}
USE_LOCAL_DSPLIB=${USE_LOCAL_DSPLIB:-0}

echo -e "${YELLOW}Build configuration:${NC}"
echo "  VITAL_SIGNS=$VITAL_SIGNS"
echo "  USE_LOCAL_MATHLIB=$USE_LOCAL_MATHLIB"
echo "  USE_LOCAL_MATHUTILS=$USE_LOCAL_MATHUTILS"
echo "  USE_LOCAL_DSPLIB=$USE_LOCAL_DSPLIB"
echo ""

# Change to build directory
cd "${MMWAVE_SDK_INSTALL_PATH}/ti/demo/xwr68xx/mmw"

# Clean first to remove stale dependency files with host paths
# (These get baked in from native builds and break Docker builds)
echo -e "${YELLOW}Cleaning previous build artifacts...${NC}"
make clean 2>/dev/null || true
rm -rf obj_xwr68xx mmw_configPkg_* 2>/dev/null || true
echo ""

# Build
echo -e "${CYAN}Building firmware...${NC}"
echo ""

make all \
    VITAL_SIGNS=$VITAL_SIGNS \
    USE_LOCAL_MATHLIB=$USE_LOCAL_MATHLIB \
    USE_LOCAL_MATHUTILS=$USE_LOCAL_MATHUTILS \
    USE_LOCAL_DSPLIB=$USE_LOCAL_DSPLIB

BUILD_RESULT=$?

echo ""

if [[ $BUILD_RESULT -eq 0 ]]; then
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}  BUILD SUCCESSFUL${NC}"
    echo -e "${GREEN}════════════════════════════════════════════════════════${NC}"
    echo ""

    # Copy output to workspace
    if [[ -f "xwr68xx_mmw_demo.bin" ]]; then
        OUTPUT_DIR="/workspace/firmware/custom"
        mkdir -p "$OUTPUT_DIR"
        cp xwr68xx_mmw_demo.bin "$OUTPUT_DIR/iwr6843aop_mmw_vitalsigns.bin"
        echo -e "Output: ${CYAN}firmware/custom/iwr6843aop_mmw_vitalsigns.bin${NC}"
        ls -lh "$OUTPUT_DIR/iwr6843aop_mmw_vitalsigns.bin"
    fi
else
    echo -e "${RED}════════════════════════════════════════════════════════${NC}"
    echo -e "${RED}  BUILD FAILED${NC}"
    echo -e "${RED}════════════════════════════════════════════════════════${NC}"
    exit 1
fi
