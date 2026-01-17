#!/bin/bash
# =============================================================================
# Clean build artifacts
# =============================================================================

set -e

echo "Cleaning build artifacts..."

cd "${MMWAVE_SDK_INSTALL_PATH}/ti/demo/xwr68xx/mmw"

# Clean MSS
if [[ -f "mss/mmw_mss.mak" ]]; then
    make -f mss/mmw_mss.mak clean 2>/dev/null || true
fi

# Clean DSS
if [[ -f "dss/mmw_dss.mak" ]]; then
    make -f dss/mmw_dss.mak clean 2>/dev/null || true
fi

# Remove generated files
rm -rf obj_xwr68xx 2>/dev/null || true
rm -rf mmw_configPkg_* 2>/dev/null || true
rm -f *.xer4f *.xe674 *.bin *.map *.rov.xs 2>/dev/null || true

echo "Clean complete."
