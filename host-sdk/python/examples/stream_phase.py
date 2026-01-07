#!/usr/bin/env python3
"""
Stream phase data from chirp radar.

Example script demonstrating real-time phase streaming from
chirp firmware configured in PHASE output mode.

Usage:
    python stream_phase.py /dev/ttyUSB1 921600
    python stream_phase.py COM3 921600
"""

import argparse
import sys
import time

try:
    import serial
except ImportError:
    print("Error: pyserial not installed. Run: pip install pyserial")
    sys.exit(1)

# Add parent directory to path for development
sys.path.insert(0, str(__file__).rsplit("/", 2)[0])

from chirp import FrameParser


def main():
    parser = argparse.ArgumentParser(description="Stream phase data from chirp radar")
    parser.add_argument("port", help="Serial port (e.g., /dev/ttyUSB1, COM3)")
    parser.add_argument("baudrate", type=int, default=921600, nargs="?", help="Baud rate (default: 921600)")
    parser.add_argument("-v", "--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--csv", type=str, help="Output CSV file")
    args = parser.parse_args()

    # Open serial port
    print(f"Opening {args.port} at {args.baudrate} baud...")
    try:
        ser = serial.Serial(args.port, args.baudrate, timeout=0.1)
    except serial.SerialException as e:
        print(f"Error opening serial port: {e}")
        sys.exit(1)

    # Optional CSV output
    csv_file = None
    if args.csv:
        csv_file = open(args.csv, "w")
        csv_file.write("timestamp_us,frame,bin,phase_rad,magnitude,motion\n")

    frame_parser = FrameParser()
    frame_count = 0
    start_time = time.time()

    print("Streaming phase data (Ctrl+C to stop)...")
    print("-" * 60)

    try:
        while True:
            # Read available data
            data = ser.read(4096)
            if data:
                frame_parser.feed(data)

            # Process complete frames
            for frame in frame_parser.parse_frames():
                frame_count += 1
                frame_num = frame.header.frame_number

                # Check for phase output
                if frame.phase_output:
                    po = frame.phase_output
                    print(f"Frame {frame_num}: Phase output with {po.num_bins} bins, center={po.center_bin}")

                    for bin_data in po.bins:
                        phase_rad = bin_data.phase_radians
                        motion_flag = "M" if bin_data.has_motion else " "

                        if args.verbose:
                            print(
                                f"  Bin {bin_data.bin_index:3d}: "
                                f"phase={phase_rad:+7.4f} rad, "
                                f"mag={bin_data.magnitude:5d} "
                                f"[{motion_flag}]"
                            )

                        if csv_file:
                            csv_file.write(
                                f"{po.timestamp_us},{frame_num},"
                                f"{bin_data.bin_index},{phase_rad:.6f},"
                                f"{bin_data.magnitude},{1 if bin_data.has_motion else 0}\n"
                            )

                # Check for presence detection
                if frame.presence:
                    p = frame.presence
                    print(f"Frame {frame_num}: Presence={p.presence_str}, range={p.range_meters:.2f}m, conf={p.confidence}%")

                # Check for motion status
                if frame.motion_status and frame.motion_status.motion_detected:
                    ms = frame.motion_status
                    print(f"Frame {frame_num}: Motion detected! level={ms.motion_level}, bin={ms.peak_motion_bin}")

                # Check for target info
                if frame.target_info and args.verbose:
                    ti = frame.target_info
                    print(f"Frame {frame_num}: Target bin={ti.primary_bin}, range={ti.range_meters:.2f}m")

            # Periodic status
            elapsed = time.time() - start_time
            if elapsed > 0 and frame_count > 0 and frame_count % 100 == 0:
                fps = frame_count / elapsed
                print(f"[{frame_count} frames, {fps:.1f} fps]")

    except KeyboardInterrupt:
        print("\nStopped.")
    finally:
        ser.close()
        if csv_file:
            csv_file.close()
            print(f"Saved to {args.csv}")

        elapsed = time.time() - start_time
        if elapsed > 0:
            print(f"Received {frame_count} frames in {elapsed:.1f}s ({frame_count / elapsed:.1f} fps)")


if __name__ == "__main__":
    main()
