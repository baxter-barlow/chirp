#!/usr/bin/env python3
"""
parse_vitalsigns.py - Parser for IWR6843AOPEVM Vital Signs TLV output

This script parses the vital signs TLV (type 0x410) from the mmWave demo
UART data stream.

Usage:
    python parse_vitalsigns.py /dev/ttyACM1
    python parse_vitalsigns.py /dev/ttyACM1 --log vitalsigns.csv

Author: Custom firmware project
"""

import argparse
import struct
import serial
import sys
import time
from datetime import datetime

# Magic word for frame sync
MAGIC_WORD = bytes([0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07])

# TLV types
TLV_TYPE_DETECTED_POINTS = 1
TLV_TYPE_RANGE_PROFILE = 2
TLV_TYPE_NOISE_PROFILE = 3
TLV_TYPE_AZIMUTH_HEATMAP = 4
TLV_TYPE_RANGE_DOPPLER_HEATMAP = 5
TLV_TYPE_STATS = 6
TLV_TYPE_SIDE_INFO = 7
TLV_TYPE_AZIMUTH_ELEVATION_HEATMAP = 8
TLV_TYPE_TEMPERATURE = 9
TLV_TYPE_VITALSIGNS = 0x410

# Header structure: 8 bytes magic + 32 bytes header = 40 bytes total
HEADER_SIZE = 40

class VitalSignsOutput:
    """Vital signs output structure"""
    def __init__(self, data):
        if len(data) < 20:
            raise ValueError(f"Invalid vital signs data length: {len(data)}")

        # Parse according to MmwDemo_output_message_vitalsigns structure
        self.target_id = struct.unpack('<H', data[0:2])[0]
        self.range_bin = struct.unpack('<H', data[2:4])[0]
        self.heart_rate = struct.unpack('<f', data[4:8])[0]
        self.breathing_rate = struct.unpack('<f', data[8:12])[0]
        self.breathing_deviation = struct.unpack('<f', data[12:16])[0]
        self.valid = data[16]
        # reserved[3] at bytes 17-19

    def __str__(self):
        status = "VALID" if self.valid else "INVALID"
        return (f"Vital Signs [{status}]: "
                f"HR={self.heart_rate:.1f} BPM, "
                f"BR={self.breathing_rate:.1f} BPM, "
                f"Dev={self.breathing_deviation:.4f}, "
                f"RangeBin={self.range_bin}, "
                f"TargetID={self.target_id}")


class FrameHeader:
    """Frame header parser"""
    def __init__(self, data):
        if len(data) < HEADER_SIZE:
            raise ValueError(f"Invalid header length: {len(data)}")

        # Skip magic word (8 bytes)
        offset = 8

        self.version = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.total_packet_len = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.platform = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.frame_number = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.time_cpu_cycles = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.num_detected_obj = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.num_tlvs = struct.unpack('<I', data[offset:offset+4])[0]
        offset += 4

        self.subframe_number = struct.unpack('<I', data[offset:offset+4])[0]


def find_magic_word(ser, timeout=5.0):
    """Find magic word in serial stream"""
    buffer = b''
    start_time = time.time()

    while time.time() - start_time < timeout:
        if ser.in_waiting:
            buffer += ser.read(ser.in_waiting)

            # Look for magic word
            idx = buffer.find(MAGIC_WORD)
            if idx >= 0:
                # Return data starting from magic word
                return buffer[idx:]

        time.sleep(0.001)

    return None


def read_frame(ser, timeout=1.0):
    """Read a complete frame from serial port"""
    # Find magic word
    data = find_magic_word(ser, timeout)
    if data is None:
        return None

    # Read header if not complete
    while len(data) < HEADER_SIZE:
        if ser.in_waiting:
            data += ser.read(min(HEADER_SIZE - len(data), ser.in_waiting))
        time.sleep(0.001)

    # Parse header to get total length
    header = FrameHeader(data)

    # Read remaining data
    while len(data) < header.total_packet_len:
        if ser.in_waiting:
            remaining = header.total_packet_len - len(data)
            data += ser.read(min(remaining, ser.in_waiting))
        time.sleep(0.001)

    return data


def parse_tlvs(data, header):
    """Parse TLVs from frame data"""
    offset = HEADER_SIZE
    tlvs = {}

    for _ in range(header.num_tlvs):
        if offset + 8 > len(data):
            break

        tlv_type = struct.unpack('<I', data[offset:offset+4])[0]
        tlv_length = struct.unpack('<I', data[offset+4:offset+8])[0]
        offset += 8

        if offset + tlv_length > len(data):
            break

        tlv_data = data[offset:offset+tlv_length]
        tlvs[tlv_type] = tlv_data
        offset += tlv_length

    return tlvs


def main():
    parser = argparse.ArgumentParser(description='Parse IWR6843 Vital Signs TLV output')
    parser.add_argument('port', help='Serial port (e.g., /dev/ttyACM1)')
    parser.add_argument('--baud', type=int, default=921600, help='Baud rate (default: 921600)')
    parser.add_argument('--log', help='Log file (CSV format)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    args = parser.parse_args()

    # Open serial port
    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
        print(f"Opened {args.port} at {args.baud} baud")
    except Exception as e:
        print(f"Error opening serial port: {e}")
        sys.exit(1)

    # Open log file if specified
    log_file = None
    if args.log:
        log_file = open(args.log, 'w')
        log_file.write("timestamp,frame,heart_rate,breathing_rate,deviation,valid,range_bin,target_id\n")
        print(f"Logging to {args.log}")

    print("Waiting for vital signs data...")
    print("Press Ctrl+C to stop\n")

    try:
        while True:
            # Read frame
            frame_data = read_frame(ser)
            if frame_data is None:
                continue

            # Parse header
            try:
                header = FrameHeader(frame_data)
            except ValueError as e:
                if args.verbose:
                    print(f"Header parse error: {e}")
                continue

            # Parse TLVs
            tlvs = parse_tlvs(frame_data, header)

            # Check for vital signs TLV
            if TLV_TYPE_VITALSIGNS in tlvs:
                try:
                    vs = VitalSignsOutput(tlvs[TLV_TYPE_VITALSIGNS])

                    # Print to console
                    print(f"Frame {header.frame_number:5d}: {vs}")

                    # Log to file
                    if log_file:
                        timestamp = datetime.now().isoformat()
                        log_file.write(f"{timestamp},{header.frame_number},"
                                      f"{vs.heart_rate:.2f},{vs.breathing_rate:.2f},"
                                      f"{vs.breathing_deviation:.6f},{vs.valid},"
                                      f"{vs.range_bin},{vs.target_id}\n")
                        log_file.flush()

                except ValueError as e:
                    if args.verbose:
                        print(f"VS parse error: {e}")
            elif args.verbose:
                print(f"Frame {header.frame_number}: No vital signs TLV (TLVs: {list(tlvs.keys())})")

    except KeyboardInterrupt:
        print("\nStopped by user")
    finally:
        ser.close()
        if log_file:
            log_file.close()


if __name__ == '__main__':
    main()
