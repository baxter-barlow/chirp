"""
Chirp Python SDK - Host-side tools for chirp mmWave radar firmware.

This package provides:
- TLV parsing for chirp custom TLV types (0x0500-0x05FF)
- Frame parser for UART output streams
- Data structures for all chirp output formats

Example usage:
    from chirp import FrameParser, PhaseOutput

    parser = FrameParser()
    parser.feed(uart_data)

    for frame in parser.parse_frames():
        if frame.phase_output:
            for bin in frame.phase_output.bins:
                print(f"Bin {bin.bin_index}: phase={bin.phase_radians:.3f} rad")
"""

__version__ = "1.1.1"

# TLV type constants
# Parser classes
from .parser import (
    FrameHeader,
    FrameParser,
    ParsedFrame,
    RawTLV,
    find_magic_word,
    parse_frame,
    parse_header,
)

# Data classes for parsed TLVs
from .tlv import (
    TLV_COMPLEX_RANGE_FFT,
    TLV_DETECTED_POINTS,
    TLV_MOTION_STATUS,
    TLV_PHASE_OUTPUT,
    TLV_PRESENCE,
    TLV_RANGE_PROFILE,
    TLV_TARGET_INFO,
    TLV_TARGET_IQ,
    ComplexRangeFFT,
    MotionStatus,
    PhaseBin,
    PhaseOutput,
    Presence,
    TargetInfo,
    TargetIQ,
    TargetIQBin,
    tlv_name,
)

__all__ = [
    # Version
    "__version__",
    # TLV constants
    "TLV_COMPLEX_RANGE_FFT",
    "TLV_TARGET_IQ",
    "TLV_PHASE_OUTPUT",
    "TLV_PRESENCE",
    "TLV_MOTION_STATUS",
    "TLV_TARGET_INFO",
    "TLV_RANGE_PROFILE",
    "TLV_DETECTED_POINTS",
    "tlv_name",
    # TLV data classes
    "ComplexRangeFFT",
    "TargetIQ",
    "TargetIQBin",
    "PhaseOutput",
    "PhaseBin",
    "Presence",
    "MotionStatus",
    "TargetInfo",
    # Parser
    "FrameHeader",
    "ParsedFrame",
    "RawTLV",
    "FrameParser",
    "parse_frame",
    "parse_header",
    "find_magic_word",
]
