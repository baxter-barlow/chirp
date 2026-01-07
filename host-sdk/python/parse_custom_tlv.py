#!/usr/bin/env python3
"""
Host-side parser for IWR6843 UART output with custom TLV support.

Supports serial port or file capture parsing with robust magic word sync
and header/TLV validation. Designed to run without hardware in self-test
mode or with synthetic captures.
"""

import argparse
import csv
import math
import struct
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional, Sequence

try:
    import numpy as np
    HAS_NUMPY = True
except ImportError:
    HAS_NUMPY = False
    np = None

try:
    import serial
    HAS_SERIAL = True
except ImportError:
    HAS_SERIAL = False
    serial = None


# =============================================================================
# Constants
# =============================================================================

MAGIC_WORD = bytes([0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07])

MIN_HEADER_LEN = 36
MAX_HEADER_LEN = 40
MAX_FRAME_LEN = 2 * 1024 * 1024
MAX_TLVS = 64

TLV_DETECTED_POINTS = 1
TLV_RANGE_PROFILE = 2
TLV_NOISE_PROFILE = 3
TLV_AZIMUTH_STATIC_HEATMAP = 4
TLV_RANGE_DOPPLER_HEATMAP = 5
TLV_STATS = 6
TLV_DETECTED_POINTS_SIDE_INFO = 7
TLV_AZIMUTH_ELEVATION_STATIC_HEATMAP = 8
TLV_TEMPERATURE_STATS = 9

TLV_COMPLEX_RANGE_FFT = 0x0500

TLV_NAMES = {
    TLV_DETECTED_POINTS: "DETECTED_POINTS",
    TLV_RANGE_PROFILE: "RANGE_PROFILE",
    TLV_NOISE_PROFILE: "NOISE_PROFILE",
    TLV_AZIMUTH_STATIC_HEATMAP: "AZIMUTH_STATIC_HEATMAP",
    TLV_RANGE_DOPPLER_HEATMAP: "RANGE_DOPPLER_HEATMAP",
    TLV_STATS: "STATS",
    TLV_DETECTED_POINTS_SIDE_INFO: "SIDE_INFO",
    TLV_AZIMUTH_ELEVATION_STATIC_HEATMAP: "AZIMUTH_ELEVATION_HEATMAP",
    TLV_TEMPERATURE_STATS: "TEMPERATURE_STATS",
    TLV_COMPLEX_RANGE_FFT: "COMPLEX_RANGE_FFT",
}

HEADER_CANDIDATES = [
    ("standard_40", 40, "<8sIIIIIIII"),
    ("short_36", 36, "<8sIIIIIII"),
]


# =============================================================================
# Data Classes
# =============================================================================

@dataclass
class FrameHeader:
    magic: bytes
    version: int
    total_packet_len: int
    platform: int
    frame_number: int
    time_cpu_cycles: int
    num_detected_obj: int
    num_tlvs: int
    subframe_number: int
    header_len: int
    variant: str


@dataclass
class TLV:
    type: int
    type_name: str
    length: int
    data: bytes


@dataclass
class ComplexRangeFFT:
    num_range_bins: int
    chirp_index: int
    rx_antenna: int
    reserved: int
    real: Sequence[int]
    imag: Sequence[int]
    iq_order: str


@dataclass
class ParsedFrame:
    header: FrameHeader
    tlvs: list = field(default_factory=list)
    complex_range_fft: Optional[ComplexRangeFFT] = None
    range_profile: Optional[Sequence[int]] = None


# =============================================================================
# Parsing Helpers
# =============================================================================


def _is_plausible_header(total_len: int, num_tlvs: int, header_len: int) -> bool:
    if total_len < header_len:
        return False
    if total_len > MAX_FRAME_LEN:
        return False
    if num_tlvs < 0 or num_tlvs > MAX_TLVS:
        return False
    if total_len < header_len + (num_tlvs * 8):
        return False
    return True


def parse_header_candidates(buffer: bytes) -> Optional[FrameHeader]:
    """Try known header layouts and validate lengths."""
    for name, size, fmt in HEADER_CANDIDATES:
        if len(buffer) < size:
            continue
        try:
            fields = struct.unpack(fmt, buffer[:size])
        except struct.error:
            continue

        if fields[0] != MAGIC_WORD:
            continue

        version = fields[1]
        total_len = fields[2]
        platform = fields[3]
        frame_number = fields[4]
        time_cpu_cycles = fields[5]
        num_detected_obj = fields[6]
        num_tlvs = fields[7]
        subframe_number = fields[8] if size == 40 else 0

        if not _is_plausible_header(total_len, num_tlvs, size):
            continue

        return FrameHeader(
            magic=fields[0],
            version=version,
            total_packet_len=total_len,
            platform=platform,
            frame_number=frame_number,
            time_cpu_cycles=time_cpu_cycles,
            num_detected_obj=num_detected_obj,
            num_tlvs=num_tlvs,
            subframe_number=subframe_number,
            header_len=size,
            variant=name,
        )

    return None


def parse_tlv_header(data: bytes) -> tuple[int, int]:
    if len(data) < 8:
        raise ValueError("TLV header too short")
    return struct.unpack("<II", data[:8])


def _normalize_tlv_length(reported_len: int, remaining: int) -> Optional[int]:
    if reported_len <= remaining:
        return reported_len
    if reported_len >= 8 and (reported_len - 8) <= remaining:
        return reported_len - 8
    return None


def _to_numpy_array(values: Sequence[int], dtype) -> Sequence[int]:
    if not HAS_NUMPY:
        return list(values)
    return np.array(values, dtype=dtype)


def parse_complex_range_fft(data: bytes, iq_order: str) -> Optional[ComplexRangeFFT]:
    if len(data) < 8:
        return None

    num_bins, chirp_idx, rx_ant, reserved = struct.unpack("<HHHH", data[:8])
    expected_size = 8 + num_bins * 4
    if len(data) < expected_size:
        print(
            f"Warning: ComplexRangeFFT data too short. Expected {expected_size}, got {len(data)}"
        )
        return None

    real_vals = []
    imag_vals = []
    for i in range(num_bins):
        offset = 8 + i * 4
        first, second = struct.unpack_from("<hh", data, offset)
        if iq_order == "imre":
            imag_vals.append(first)
            real_vals.append(second)
        else:
            real_vals.append(first)
            imag_vals.append(second)

    real = _to_numpy_array(real_vals, np.int16 if HAS_NUMPY else None)
    imag = _to_numpy_array(imag_vals, np.int16 if HAS_NUMPY else None)

    return ComplexRangeFFT(
        num_range_bins=num_bins,
        chirp_index=chirp_idx,
        rx_antenna=rx_ant,
        reserved=reserved,
        real=real,
        imag=imag,
        iq_order=iq_order,
    )


def parse_range_profile(data: bytes) -> Optional[Sequence[int]]:
    if len(data) < 2:
        return None
    num_bins = len(data) // 2
    values = struct.unpack(f"<{num_bins}H", data[: num_bins * 2])
    return _to_numpy_array(values, np.uint16 if HAS_NUMPY else None)


def parse_tlvs(data: bytes, num_tlvs: int) -> list[TLV]:
    tlvs = []
    offset = 0

    for _ in range(num_tlvs):
        if offset + 8 > len(data):
            break

        tlv_type, tlv_len_reported = parse_tlv_header(data[offset : offset + 8])
        remaining = len(data) - (offset + 8)
        payload_len = _normalize_tlv_length(tlv_len_reported, remaining)
        if payload_len is None:
            print(
                f"Warning: TLV length invalid (type {tlv_type:#x}, len {tlv_len_reported})"
            )
            break

        tlv_data = data[offset + 8 : offset + 8 + payload_len]
        type_name = TLV_NAMES.get(tlv_type, f"UNKNOWN_{tlv_type:#x}")
        tlvs.append(
            TLV(
                type=tlv_type,
                type_name=type_name,
                length=payload_len,
                data=tlv_data,
            )
        )

        offset += 8 + payload_len

    return tlvs


def parse_frame(data: bytes, iq_order: str) -> Optional[ParsedFrame]:
    header = parse_header_candidates(data)
    if header is None:
        return None

    tlv_start = header.header_len
    tlv_end = header.total_packet_len
    tlv_data = data[tlv_start:tlv_end]
    tlvs = parse_tlvs(tlv_data, header.num_tlvs)

    frame = ParsedFrame(header=header, tlvs=tlvs)

    for tlv in tlvs:
        if tlv.type == TLV_COMPLEX_RANGE_FFT:
            frame.complex_range_fft = parse_complex_range_fft(tlv.data, iq_order)
        elif tlv.type == TLV_RANGE_PROFILE:
            frame.range_profile = parse_range_profile(tlv.data)

    return frame


# =============================================================================
# Numeric Utilities
# =============================================================================


def _magnitude(real: Sequence[int], imag: Sequence[int]) -> Sequence[float]:
    if HAS_NUMPY and isinstance(real, np.ndarray):
        real_f = real.astype(np.float32)
        imag_f = imag.astype(np.float32)
        return np.sqrt(real_f * real_f + imag_f * imag_f)
    return [math.hypot(r, i) for r, i in zip(real, imag)]


def _phase(real: Sequence[int], imag: Sequence[int]) -> Sequence[float]:
    if HAS_NUMPY and isinstance(real, np.ndarray):
        return np.arctan2(imag, real)
    return [math.atan2(i, r) for r, i in zip(real, imag)]


def _argmax(values: Sequence[float]) -> int:
    if values is None or len(values) == 0:
        return 0
    if HAS_NUMPY and isinstance(values, np.ndarray):
        return int(np.argmax(values))
    return max(range(len(values)), key=lambda idx: values[idx])


def _stats(values: Sequence[float]) -> tuple[float, float, float]:
    if HAS_NUMPY and isinstance(values, np.ndarray):
        return float(values.min()), float(values.max()), float(values.mean())
    if values is None or len(values) == 0:
        return 0.0, 0.0, 0.0
    total = sum(values)
    return min(values), max(values), total / len(values)


# =============================================================================
# Frame Reader
# =============================================================================


class FrameReader:
    """Read frames from serial port or file."""

    def __init__(self, source: str, baud: int = 921600, is_file: bool = False):
        self.is_file = is_file
        self.buffer = bytearray()

        if is_file:
            self.file = open(source, "rb")
            self.source = self.file
        else:
            if not HAS_SERIAL:
                raise ImportError("pyserial required for serial port mode")
            self.serial = serial.Serial(source, baud, timeout=0.1)
            self.serial.reset_input_buffer()
            self.source = self.serial

    def close(self):
        if self.is_file:
            self.file.close()
        else:
            self.serial.close()

    def read_frame(self, timeout: float = 1.0, iq_order: str = "reim") -> Optional[ParsedFrame]:
        start_time = time.time()

        while time.time() - start_time < timeout:
            if self.is_file:
                chunk = self.source.read(4096)
                if not chunk:
                    return None
            else:
                available = self.serial.in_waiting
                if available:
                    chunk = self.serial.read(available)
                else:
                    chunk = b""
                    time.sleep(0.01)

            if chunk:
                self.buffer.extend(chunk)

            idx = self.buffer.find(MAGIC_WORD)
            if idx == -1:
                if len(self.buffer) > len(MAGIC_WORD):
                    self.buffer = self.buffer[-len(MAGIC_WORD) :]
                continue

            if idx > 0:
                self.buffer = self.buffer[idx:]

            if len(self.buffer) < MIN_HEADER_LEN:
                continue

            header = parse_header_candidates(self.buffer)
            if header is None:
                self.buffer = self.buffer[1:]
                continue

            if len(self.buffer) < header.total_packet_len:
                continue

            frame_data = bytes(self.buffer[: header.total_packet_len])
            self.buffer = self.buffer[header.total_packet_len :]

            return parse_frame(frame_data, iq_order)

        return None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()


# =============================================================================
# Output Formatting
# =============================================================================


def format_frame_summary(frame: ParsedFrame, verbose: bool = False) -> str:
    lines = []
    header = frame.header
    lines.append(f"Frame {header.frame_number} ({header.variant})")
    lines.append(f"  Packet len: {header.total_packet_len} bytes")
    lines.append(f"  Objects: {header.num_detected_obj}, TLVs: {header.num_tlvs}")

    for tlv in frame.tlvs:
        lines.append(f"  TLV {tlv.type:#06x} ({tlv.type_name}): {tlv.length} bytes")

        if tlv.type == TLV_COMPLEX_RANGE_FFT and frame.complex_range_fft:
            fft = frame.complex_range_fft
            mag = _magnitude(fft.real, fft.imag)
            phase = _phase(fft.real, fft.imag)
            peak_bin = _argmax(mag)
            phase_min, phase_max, phase_mean = _stats(phase)
            lines.append(
                f"    Bins: {fft.num_range_bins}, Chirp: {fft.chirp_index}, Antenna: {fft.rx_antenna}"
            )
            lines.append(f"    Peak: bin {peak_bin}, magnitude {mag[peak_bin]:.1f}")
            lines.append(
                f"    Phase stats: min {phase_min:.3f}, max {phase_max:.3f}, mean {phase_mean:.3f} rad"
            )
            lines.append(f"    Phase at peak: {phase[peak_bin]:.3f} rad")

            if verbose:
                pairs = list(zip(fft.real[:5], fft.imag[:5]))
                lines.append(f"    First 5 I/Q: {pairs}")
                lines.append(f"    I/Q order: {fft.iq_order}")

        elif tlv.type == TLV_RANGE_PROFILE and frame.range_profile is not None:
            rp = frame.range_profile
            if HAS_NUMPY and isinstance(rp, np.ndarray):
                peak_bin = int(np.argmax(rp))
                peak_val = int(rp[peak_bin])
            else:
                peak_bin = max(range(len(rp)), key=lambda idx: rp[idx]) if rp else 0
                peak_val = rp[peak_bin] if rp else 0
            lines.append(f"    Bins: {len(rp)}, Peak: bin {peak_bin} = {peak_val}")

    return "\n".join(lines)


def _dump_iq_to_csv(path: Path, fft: ComplexRangeFFT) -> None:
    with path.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["index", "real", "imag"])
        for idx, (real, imag) in enumerate(zip(fft.real, fft.imag)):
            writer.writerow([idx, int(real), int(imag)])


def _dump_iq_to_npz(path: Path, fft: ComplexRangeFFT) -> None:
    if not HAS_NUMPY:
        raise RuntimeError("NumPy not available for npz output")
    np.savez(
        path,
        real=np.array(fft.real, dtype=np.int16),
        imag=np.array(fft.imag, dtype=np.int16),
        iq_order=fft.iq_order,
    )


def dump_iq(frame: ParsedFrame, output_path: Path) -> Path:
    fft = frame.complex_range_fft
    if fft is None:
        raise RuntimeError("No complex range FFT data to dump")

    base = output_path
    suffix = base.suffix

    if not suffix:
        suffix = ".npz" if HAS_NUMPY else ".csv"

    if suffix.lower() == ".npz" and not HAS_NUMPY:
        suffix = ".csv"

    if base.suffix:
        base = base.with_suffix("")

    out_path = base.with_name(f"{base.name}_frame{frame.header.frame_number:06d}{suffix}")

    if suffix.lower() == ".npz":
        _dump_iq_to_npz(out_path, fft)
    else:
        _dump_iq_to_csv(out_path, fft)

    return out_path


# =============================================================================
# Self-Test
# =============================================================================


def _build_self_test_frame(num_bins: int = 16, iq_order: str = "reim") -> bytes:
    header = bytearray()
    header.extend(MAGIC_WORD)

    version = 0x03060200
    platform = 0x00680443

    tlv_payload = bytearray()
    tlv_payload.extend(struct.pack("<HHHH", num_bins, 0, 0, 0))
    for i in range(num_bins):
        real = i
        imag = -i
        if iq_order == "imre":
            tlv_payload.extend(struct.pack("<hh", imag, real))
        else:
            tlv_payload.extend(struct.pack("<hh", real, imag))

    tlv_header = struct.pack("<II", TLV_COMPLEX_RANGE_FFT, len(tlv_payload))
    tlv_blob = tlv_header + tlv_payload

    total_len = MAX_HEADER_LEN + len(tlv_blob)
    header.extend(struct.pack("<I", version))
    header.extend(struct.pack("<I", total_len))
    header.extend(struct.pack("<I", platform))
    header.extend(struct.pack("<I", 1))
    header.extend(struct.pack("<I", 123456))
    header.extend(struct.pack("<I", 0))
    header.extend(struct.pack("<I", 1))
    header.extend(struct.pack("<I", 0))

    return bytes(header + tlv_blob)


def run_self_test(iq_order: str) -> int:
    print("Running self-test with synthetic data...")
    test_frame = _build_self_test_frame(num_bins=16, iq_order=iq_order)
    parsed = parse_frame(test_frame, iq_order)

    if parsed is None:
        print("FAIL: Could not parse synthetic frame")
        return 1

    fft = parsed.complex_range_fft
    if fft is None:
        print("FAIL: ComplexRangeFFT TLV not parsed")
        return 1

    if fft.num_range_bins != 16:
        print("FAIL: Unexpected num_range_bins")
        return 1

    expected_real = [0, 1, 2]
    expected_imag = [0, -1, -2]
    if list(fft.real[:3]) != expected_real or list(fft.imag[:3]) != expected_imag:
        print("FAIL: I/Q samples do not match expected values")
        return 1

    print("Self-test PASSED")
    return 0


# =============================================================================
# Main
# =============================================================================


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Parse IWR6843 UART output with custom TLV support"
    )
    parser.add_argument("--port", default="/dev/ttyACM1", help="Serial port")
    parser.add_argument("--baud", type=int, default=921600, help="Baud rate")
    parser.add_argument("--file", type=str, help="Read from file")
    parser.add_argument("--filter", type=str, help="Filter TLV type (hex, e.g., 0x500)")
    parser.add_argument("--count", type=int, default=0, help="Number of frames to read")
    parser.add_argument("--verbose", "-v", action="store_true", help="Verbose output")
    parser.add_argument("--quiet", "-q", action="store_true", help="Suppress unfiltered output")
    parser.add_argument("--self-test", action="store_true", help="Run self-test")
    parser.add_argument("--dump", type=str, help="Dump complex I/Q to .csv or .npz")
    parser.add_argument(
        "--iq-order",
        choices=["reim", "imre"],
        default="imre",
        help="I/Q sample order in payload (imre=SDK default, reim=alternative)",
    )

    args = parser.parse_args()

    if args.self_test:
        return run_self_test(args.iq_order)

    tlv_filter = None
    if args.filter:
        tlv_filter = int(args.filter, 16) if args.filter.startswith("0x") else int(args.filter)

    if args.file:
        source = args.file
        is_file = True
        print(f"Reading from file: {args.file}")
    else:
        if not HAS_SERIAL:
            print("Error: pyserial not installed. Use --file mode or install pyserial.")
            return 1
        source = args.port
        is_file = False
        print(f"Connecting to {args.port} at {args.baud} baud...")

    dump_path = Path(args.dump) if args.dump else None

    frame_count = 0
    try:
        with FrameReader(source, args.baud, is_file) as reader:
            while True:
                frame = reader.read_frame(iq_order=args.iq_order)
                if frame is None:
                    if is_file:
                        print("End of file")
                        break
                    continue

                frame_count += 1

                if tlv_filter is not None:
                    filtered_tlvs = [t for t in frame.tlvs if t.type == tlv_filter]
                    if not filtered_tlvs:
                        continue
                    frame.tlvs = filtered_tlvs

                if not args.quiet or tlv_filter:
                    print(f"\n{'=' * 60}")
                    print(format_frame_summary(frame, verbose=args.verbose))

                if dump_path and frame.complex_range_fft:
                    out_path = dump_iq(frame, dump_path)
                    print(f"Dumped I/Q to {out_path}")

                if args.count > 0 and frame_count >= args.count:
                    break

    except KeyboardInterrupt:
        print(f"\n\nReceived {frame_count} frames")
    except Exception as exc:
        print(f"Error: {exc}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
