"""
Chirp frame parser for UART output.

Parses mmWave radar frames with support for chirp custom TLVs.
"""

import struct
from dataclasses import dataclass, field
from typing import Iterator, Optional

from .tlv import (
    ComplexRangeFFT,
    MotionStatus,
    PhaseOutput,
    Presence,
    TargetInfo,
    TargetIQ,
    parse_tlv_payload,
    tlv_name,
)

# =============================================================================
# Constants
# =============================================================================

MAGIC_WORD = bytes([0x02, 0x01, 0x04, 0x03, 0x06, 0x05, 0x08, 0x07])
MIN_HEADER_LEN = 36
MAX_HEADER_LEN = 40
MAX_FRAME_LEN = 2 * 1024 * 1024
MAX_TLVS = 64

HEADER_FORMATS = [
    ("standard_40", 40, "<8sIIIIIIII"),
    ("short_36", 36, "<8sIIIIIII"),
]


# =============================================================================
# Data Classes
# =============================================================================


@dataclass
class FrameHeader:
    """Parsed frame header."""

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
class RawTLV:
    """Raw TLV with unparsed payload."""

    type: int
    type_name: str
    length: int
    data: bytes


@dataclass
class ParsedFrame:
    """Complete parsed frame with all TLVs."""

    header: FrameHeader
    raw_tlvs: list[RawTLV] = field(default_factory=list)

    # Parsed chirp custom TLVs
    complex_range_fft: Optional[ComplexRangeFFT] = None
    target_iq: Optional[TargetIQ] = None
    phase_output: Optional[PhaseOutput] = None
    presence: Optional[Presence] = None
    motion_status: Optional[MotionStatus] = None
    target_info: Optional[TargetInfo] = None

    # Standard TLVs (raw data)
    range_profile: Optional[bytes] = None
    detected_points: Optional[bytes] = None


# =============================================================================
# Parser Implementation
# =============================================================================


def _is_valid_header(total_len: int, num_tlvs: int, header_len: int) -> bool:
    """Validate header field values."""
    if total_len < header_len:
        return False
    if total_len > MAX_FRAME_LEN:
        return False
    if num_tlvs < 0 or num_tlvs > MAX_TLVS:
        return False
    if total_len < header_len + (num_tlvs * 8):
        return False
    return True


def parse_header(buffer: bytes) -> Optional[FrameHeader]:
    """Try to parse frame header from buffer."""
    for name, size, fmt in HEADER_FORMATS:
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

        if not _is_valid_header(total_len, num_tlvs, size):
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


def find_magic_word(data: bytes, start: int = 0) -> int:
    """Find magic word in data, return offset or -1 if not found."""
    return data.find(MAGIC_WORD, start)


def parse_frame(data: bytes) -> Optional[ParsedFrame]:
    """Parse a complete frame from byte buffer.

    Args:
        data: Buffer containing frame data starting with magic word

    Returns:
        ParsedFrame if successful, None if invalid
    """
    header = parse_header(data)
    if header is None:
        return None

    if len(data) < header.total_packet_len:
        return None

    frame = ParsedFrame(header=header)

    # Parse TLVs
    offset = header.header_len
    for _ in range(header.num_tlvs):
        if offset + 8 > len(data):
            break

        tlv_type, tlv_len = struct.unpack("<II", data[offset : offset + 8])
        offset += 8

        if offset + tlv_len > len(data):
            break

        tlv_data = data[offset : offset + tlv_len]
        offset += tlv_len

        # Store raw TLV
        raw_tlv = RawTLV(
            type=tlv_type,
            type_name=tlv_name(tlv_type),
            length=tlv_len,
            data=tlv_data,
        )
        frame.raw_tlvs.append(raw_tlv)

        # Parse chirp custom TLVs
        parsed = parse_tlv_payload(tlv_type, tlv_data)
        if parsed is not None:
            if isinstance(parsed, ComplexRangeFFT):
                frame.complex_range_fft = parsed
            elif isinstance(parsed, TargetIQ):
                frame.target_iq = parsed
            elif isinstance(parsed, PhaseOutput):
                frame.phase_output = parsed
            elif isinstance(parsed, Presence):
                frame.presence = parsed
            elif isinstance(parsed, MotionStatus):
                frame.motion_status = parsed
            elif isinstance(parsed, TargetInfo):
                frame.target_info = parsed

        # Store raw data for standard TLVs
        from .tlv import TLV_DETECTED_POINTS, TLV_RANGE_PROFILE

        if tlv_type == TLV_RANGE_PROFILE:
            frame.range_profile = tlv_data
        elif tlv_type == TLV_DETECTED_POINTS:
            frame.detected_points = tlv_data

    return frame


class FrameParser:
    """Streaming frame parser with automatic sync."""

    def __init__(self, buffer_size: int = 1024 * 1024):
        self._buffer = bytearray()
        self._buffer_size = buffer_size

    def feed(self, data: bytes) -> None:
        """Add data to internal buffer."""
        self._buffer.extend(data)

        # Trim buffer if too large
        if len(self._buffer) > self._buffer_size:
            # Find last magic word and keep from there
            last_magic = self._buffer.rfind(MAGIC_WORD)
            if last_magic > 0:
                self._buffer = self._buffer[last_magic:]
            else:
                self._buffer = self._buffer[-MAX_FRAME_LEN:]

    def parse_frames(self) -> Iterator[ParsedFrame]:
        """Parse and yield all complete frames in buffer."""
        while True:
            # Find magic word
            magic_idx = find_magic_word(bytes(self._buffer))
            if magic_idx < 0:
                break

            # Discard data before magic word
            if magic_idx > 0:
                del self._buffer[:magic_idx]

            # Need enough data for header
            if len(self._buffer) < MAX_HEADER_LEN:
                break

            # Try to parse header
            header = parse_header(bytes(self._buffer))
            if header is None:
                # Invalid header, skip magic word and try again
                del self._buffer[: len(MAGIC_WORD)]
                continue

            # Check if we have complete frame
            if len(self._buffer) < header.total_packet_len:
                break

            # Parse complete frame
            frame = parse_frame(bytes(self._buffer[: header.total_packet_len]))
            if frame is not None:
                yield frame

            # Remove parsed frame from buffer
            del self._buffer[: header.total_packet_len]

    def clear(self) -> None:
        """Clear internal buffer."""
        self._buffer.clear()
