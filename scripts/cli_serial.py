#!/usr/bin/env python3
import argparse
import sys
import threading
import time

try:
    import serial
except ImportError as exc:
    sys.stderr.write("pyserial is required. Install with: pip install pyserial\n")
    raise


def reader_loop(ser):
    while True:
        try:
            data = ser.read(ser.in_waiting or 1)
        except serial.SerialException as exc:
            sys.stderr.write(f"\nSerial read error: {exc}\n")
            break
        if data:
            try:
                sys.stdout.write(data.decode("utf-8", errors="replace"))
                sys.stdout.flush()
            except Exception:
                pass
        else:
            time.sleep(0.01)


def main():
    parser = argparse.ArgumentParser(description="Simple serial CLI for mmWave devices.")
    parser.add_argument("--port", default="/dev/ttyUSB1")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--eol", default="crlf", choices=["lf", "crlf"], help="Line ending to append.")
    args = parser.parse_args()

    eol = "\n" if args.eol == "lf" else "\r\n"

    try:
        ser = serial.Serial(args.port, args.baud, timeout=0.1)
    except serial.SerialException as exc:
        sys.stderr.write(f"Failed to open {args.port}: {exc}\n")
        return 1

    sys.stdout.write(f"Connected to {args.port} @ {args.baud}. Type commands; Ctrl-C to exit.\n")
    sys.stdout.flush()

    t = threading.Thread(target=reader_loop, args=(ser,), daemon=True)
    t.start()

    try:
        for line in sys.stdin:
            if not line:
                continue
            ser.write((line.rstrip("\n") + eol).encode("utf-8", errors="replace"))
            ser.flush()
    except KeyboardInterrupt:
        pass
    finally:
        try:
            ser.close()
        except Exception:
            pass
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
