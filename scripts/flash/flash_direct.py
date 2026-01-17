#!/usr/bin/env python3
"""
Direct flash utility for IWR6843AOPEVM using mmWave bootloader protocol.
Bypasses UniFlash DSLite CLI which has issues with file order parameter.
"""

import sys
import os
import argparse

# Add mmWave flash library paths
UNIFLASH_BASE = "/home/baxter/re/vendor/uniflash/deskdb/content/TICloudAgent/linux/ccs_base/mmWave"
sys.path.insert(0, os.path.join(UNIFLASH_BASE, "gen1"))
sys.path.insert(0, os.path.join(UNIFLASH_BASE, "common/pyserial-3.5"))

import mmWaveProgFlash


class FilesObject:
    """File object matching what mmWaveProgFlash expects."""
    def __init__(self, path, order=1):
        self.path = path
        self.order = order
        self.file_id = None
        self.fileSize = 0


class FlashCallback:
    """Callback class for progress updates."""
    def __init__(self, verbose=True):
        self.verbose = verbose

    def update_progress(self, msg, percent):
        if self.verbose:
            print(f"[{percent:3d}%] {msg}")

    def push_message(self, msg, level):
        if self.verbose:
            level_names = {0: "INFO", 1: "WARN", 2: "ERROR", 3: "FATAL", 255: "DEBUG"}
            level_name = level_names.get(level, f"L{level}")
            print(f"[{level_name}] {msg}")

    def check_is_cancel_set(self):
        return False


def flash_firmware(port, firmware_path, storage="SFLASH", format_flash=False, verbose=True):
    """
    Flash firmware to IWR6843AOPEVM.

    Args:
        port: Serial port (e.g., /dev/ttyUSB2)
        firmware_path: Path to .bin firmware file
        storage: Storage type (SFLASH)
        format_flash: Whether to format flash before writing
        verbose: Print progress messages

    Returns:
        True on success, False on failure
    """

    if not os.path.exists(firmware_path):
        print(f"Error: Firmware file not found: {firmware_path}")
        return False

    callback = FlashCallback(verbose)

    # Create bootloader instance
    ldr = mmWaveProgFlash.BootLdr(callback, port)

    print(f"\nConnecting to {port}...")
    if not ldr.connect(1, port):
        print("Failed to connect. Ensure device is in flash mode (SOP=001) and power cycle.")
        return False

    print("Connected!")

    # Get device info
    if not ldr.determinePGVersion():
        print("Failed to determine device version. Power cycle and try again.")
        ldr.disconnect()
        return False

    print(f"Device: {ldr.partNum}, PG3+: {ldr.isDevicePG3OrLater()}")

    # For IWR6843AOP, the part number detection may return empty
    # Force set to IWR6843AOP if empty or not detected
    if not ldr.partNum or not ldr.isPartNumSupported(ldr.partNum):
        print("Part number not detected, using IWR6843AOP")
        ldr.setPartNum("IWR6843AOP")
    else:
        ldr.setPartNum(ldr.partNum)

    # Create file object with order=1 (META_IMAGE1)
    fileInfo = FilesObject(firmware_path, order=1)

    # Check file header
    print(f"\nChecking firmware: {firmware_path}")
    if not ldr.checkFileHeader(firmware_path, fileInfo):
        print("Error: Invalid firmware file format")
        ldr.disconnect()
        return False

    print(f"File type: {fileInfo.file_id}, Size: {fileInfo.fileSize} bytes")

    # Calculate progress values
    ldr.calcProgressValues([fileInfo], fileInfo.fileSize, format_flash)

    # Format flash if requested
    if format_flash:
        print("\nFormatting flash...")
        ldr.erase_storage()
        print("Format complete!")

    # Download the file
    print(f"\nFlashing to {storage}...")
    imageProgCntList = ldr.getImageProgCntList(fileInfo)

    success = ldr.download_file(
        fileInfo.path,
        fileInfo.file_id,
        0,  # offset
        0,  # reserved
        storage,
        imageProgCntList
    )

    ldr.disconnect()

    if success:
        print("\n" + "="*50)
        print("SUCCESS! Firmware flashed successfully.")
        print("="*50)
        print("\nNext steps:")
        print("  1. Set SOP jumpers back to 000 (functional mode)")
        print("  2. Power cycle the board")
        print("  3. Connect to CLI: python scripts/cli_serial.py")
        return True
    else:
        print("\nERROR: Flash failed!")
        return False


def main():
    parser = argparse.ArgumentParser(
        description="Flash firmware to IWR6843AOPEVM",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s -p /dev/ttyUSB2 firmware.bin
  %(prog)s -p /dev/ttyUSB2 --format firmware.bin

Before flashing:
  1. Set SOP jumpers to 001 (flash mode)
  2. Power cycle the board
  3. Run this script
        """
    )

    parser.add_argument("firmware", help="Path to firmware .bin file")
    parser.add_argument("-p", "--port", default="/dev/ttyUSB2",
                        help="Serial port (default: /dev/ttyUSB2)")
    parser.add_argument("-f", "--format", action="store_true",
                        help="Format flash before writing")
    parser.add_argument("-q", "--quiet", action="store_true",
                        help="Quiet mode (less output)")

    args = parser.parse_args()

    print("="*50)
    print("  IWR6843AOPEVM Direct Flash Utility")
    print("="*50)

    success = flash_firmware(
        port=args.port,
        firmware_path=args.firmware,
        format_flash=args.format,
        verbose=not args.quiet
    )

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
