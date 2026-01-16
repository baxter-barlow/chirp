#!/usr/bin/env python3
import argparse
import struct
import sys
import time

try:
    import serial
except ImportError as exc:
    sys.stderr.write("pyserial is required. Install with: pip install pyserial\n")
    raise


MAGIC = b"\x02\x01\x04\x03\x06\x05\x08\x07"
HEADER_LEN = 40  # bytes after magic word for mmWave demo header


def find_magic(buf):
    idx = buf.find(MAGIC)
    if idx == -1:
        # Keep only last 7 bytes to handle magic split across reads
        return -1, buf[-7:]
    return idx, buf


def parse_header(buf, start):
    if len(buf) < start + len(MAGIC) + HEADER_LEN:
        return None
    off = start + len(MAGIC)
    fields = struct.unpack_from("<9I", buf, off)
    version, total_len, platform, frame_num, cpu_cycles, num_obj, num_tlvs, subframe, reserved = fields
    return {
        "version": version,
        "total_len": total_len,
        "platform": platform,
        "frame_num": frame_num,
        "cpu_cycles": cpu_cycles,
        "num_obj": num_obj,
        "num_tlvs": num_tlvs,
        "subframe": subframe,
    }


def main():
    parser = argparse.ArgumentParser(description="Show live number of detected objects.")
    parser.add_argument("--port", default="/dev/ttyUSB1")
    parser.add_argument("--baud", type=int, default=921600)
    parser.add_argument("--quiet", action="store_true", help="Only print object count.")
    args = parser.parse_args()

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except serial.SerialException as exc:
        sys.stderr.write(f"Failed to open {args.port}: {exc}\n")
        return 1

    buf = b""
    last_frame = None
    try:
        while True:
            chunk = ser.read(4096)
            if chunk:
                buf += chunk
            else:
                time.sleep(0.005)
                continue

            idx, buf = find_magic(buf)
            if idx == -1:
                continue

            hdr = parse_header(buf, idx)
            if hdr is None:
                continue

            total_len = hdr["total_len"]
            if len(buf) < idx + total_len:
                continue

            frame_num = hdr["frame_num"]
            num_obj = hdr["num_obj"]
            if frame_num != last_frame:
                last_frame = frame_num
                if args.quiet:
                    sys.stdout.write(f"{num_obj}\n")
                else:
                    sys.stdout.write(f"frame {frame_num} objects {num_obj}\n")
                sys.stdout.flush()

            # drop processed packet
            buf = buf[idx + total_len :]
    except KeyboardInterrupt:
        pass
    finally:
        ser.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
