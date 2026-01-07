#!/usr/bin/env python3
"""
UART Flash Script for IWR6843AOPEVM
Uses TI's mmWaveProgFlash module from UniFlash
"""

import sys
import os

# Add mmWave flash module to path
UNIFLASH_MMWAVE_PATH = "/opt/ti/uniflash_9.4.0/deskdb/content/TICloudAgent/linux/ccs_base/mmWave/gen1"
sys.path.insert(0, UNIFLASH_MMWAVE_PATH)
os.chdir(UNIFLASH_MMWAVE_PATH)

from mmWaveProgFlash import BootLdr, FilesObject

class SimpleCallback:
    """Simple callback class for progress updates"""
    def update_progress(self, msg, percent):
        print(f"[{percent:3d}%] {msg}")

    def push_message(self, msg, level):
        level_names = {-1: "DEBUG", 0: "INFO", 1: "WARN", 2: "ERROR", 3: "FATAL"}
        level_name = level_names.get(level, f"L{level}")
        if level >= 0:  # Skip debug messages
            print(f"[{level_name}] {msg}")

    def check_is_cancel_set(self):
        return False

def flash_firmware(com_port, firmware_path):
    """Flash firmware to IWR6843 via UART bootloader"""

    print("=" * 60)
    print("IWR6843 UART Flash Utility")
    print("=" * 60)
    print(f"COM Port: {com_port}")
    print(f"Firmware: {firmware_path}")
    print("=" * 60)

    # Verify firmware file exists
    if not os.path.exists(firmware_path):
        print(f"ERROR: Firmware file not found: {firmware_path}")
        return False

    file_size = os.path.getsize(firmware_path)
    print(f"Firmware size: {file_size} bytes")

    # Create callback and bootloader instance
    callback = SimpleCallback()
    bootldr = BootLdr(callback, com_port, trace_level=0)

    # Set part number - IWR6843 uses xWR68xx format internally
    # The module expects partNum[1:5] = "WR68" for header lookup
    bootldr.partNum = "IWR6843AOP"

    # Connect to device (sends break signal)
    print("\nConnecting to device (sending break signal)...")
    print("Please ensure SOP pins are set to flash mode (1-0-1)")
    print("If device doesn't respond, power cycle it first.\n")

    if not bootldr.connect(timeout=30, com_port=com_port):
        print("ERROR: Failed to connect to device")
        print("Check: SOP pins, power cycle, COM port")
        return False

    print("Connected successfully!")

    # Create file object for the firmware - use META_IMAGE1
    file_obj = FilesObject(firmware_path, 1)
    file_obj.file_id = "META_IMAGE1"
    file_obj.fileSize = file_size

    # Calculate progress values
    bootldr.calcProgressValues([file_obj], file_size, False)

    # Download the file
    print("\nDownloading firmware to SFLASH...")
    print(f"File ID: {file_obj.file_id}")
    print(f"Size: {file_size} bytes")
    print("This may take a few minutes...\n")

    image_prog_list = bootldr.getImageProgCntList(file_obj)

    success = bootldr.download_file(
        filename=firmware_path,
        file_id=file_obj.file_id,
        mirror_enabled=0,
        max_size=file_size,
        storage="SFLASH",
        imageProgList=image_prog_list
    )

    # Disconnect
    bootldr.disconnect()

    if success:
        print("\n" + "=" * 60)
        print("FLASH SUCCESSFUL!")
        print("=" * 60)
        print("\nNext steps:")
        print("1. Power off the device")
        print("2. Set SOP pins to run mode (1-0-0)")
        print("3. Power on - device should boot with new firmware")
        return True
    else:
        print("\n" + "=" * 60)
        print("FLASH FAILED!")
        print("=" * 60)
        return False

if __name__ == "__main__":
    COM_PORT = "/dev/ttyUSB0"
    FIRMWARE = "/home/baxter/experiments/iwr6843-custom-firmware/recovery/xwr68xx_mmw_demo_CUSTOM_v2.bin"

    if len(sys.argv) > 1:
        COM_PORT = sys.argv[1]
    if len(sys.argv) > 2:
        FIRMWARE = sys.argv[2]

    success = flash_firmware(COM_PORT, FIRMWARE)
    sys.exit(0 if success else 1)
