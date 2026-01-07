"""
Chirp TLV definitions and data structures.

Custom TLV types (0x0500-0x05FF) for chirp firmware.
"""

import struct
from dataclasses import dataclass
from typing import Optional, Sequence

# =============================================================================
# TLV Type Constants
# =============================================================================

# Standard SDK TLV types
TLV_DETECTED_POINTS = 1
TLV_RANGE_PROFILE = 2
TLV_NOISE_PROFILE = 3
TLV_AZIMUTH_STATIC_HEATMAP = 4
TLV_RANGE_DOPPLER_HEATMAP = 5
TLV_STATS = 6
TLV_DETECTED_POINTS_SIDE_INFO = 7
TLV_AZIMUTH_ELEVATION_STATIC_HEATMAP = 8
TLV_TEMPERATURE_STATS = 9

# Chirp custom TLV types (0x0500-0x05FF)
TLV_COMPLEX_RANGE_FFT = 0x0500  # Full I/Q for all range bins
TLV_TARGET_IQ = 0x0510  # I/Q for selected target bins only
TLV_PHASE_OUTPUT = 0x0520  # Phase + magnitude for selected bins
TLV_PRESENCE = 0x0540  # Presence detection result
TLV_MOTION_STATUS = 0x0550  # Motion detection result
TLV_TARGET_INFO = 0x0560  # Target selection metadata

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
    TLV_TARGET_IQ: "TARGET_IQ",
    TLV_PHASE_OUTPUT: "PHASE_OUTPUT",
    TLV_PRESENCE: "PRESENCE",
    TLV_MOTION_STATUS: "MOTION_STATUS",
    TLV_TARGET_INFO: "TARGET_INFO",
}


def tlv_name(tlv_type: int) -> str:
    """Get human-readable name for TLV type."""
    return TLV_NAMES.get(tlv_type, f"UNKNOWN_0x{tlv_type:04X}")


# =============================================================================
# Data Classes for TLV Payloads
# =============================================================================


@dataclass
class ComplexRangeFFT:
    """TLV 0x0500: Complex Range FFT - Full I/Q for all range bins."""

    num_range_bins: int
    chirp_index: int
    rx_antenna: int
    real: Sequence[int]  # int16 values
    imag: Sequence[int]  # int16 values

    @classmethod
    def from_bytes(cls, data: bytes) -> "ComplexRangeFFT":
        """Parse from TLV payload bytes."""
        if len(data) < 8:
            raise ValueError("ComplexRangeFFT header too short")

        num_bins, chirp_idx, rx_ant, reserved = struct.unpack("<HHHH", data[:8])
        iq_data = data[8:]

        if len(iq_data) < num_bins * 4:
            raise ValueError(
                f"Not enough I/Q data: expected {num_bins * 4}, got {len(iq_data)}"
            )

        # cmplx16ImRe_t format: imag first, then real
        imag = []
        real = []
        for i in range(num_bins):
            im, re = struct.unpack("<hh", iq_data[i * 4 : i * 4 + 4])
            imag.append(im)
            real.append(re)

        return cls(
            num_range_bins=num_bins,
            chirp_index=chirp_idx,
            rx_antenna=rx_ant,
            real=real,
            imag=imag,
        )


@dataclass
class TargetIQBin:
    """Single bin data for Target I/Q TLV."""

    bin_index: int
    imag: int
    real: int


@dataclass
class TargetIQ:
    """TLV 0x0510: Target I/Q - I/Q for selected bins only."""

    num_bins: int
    center_bin: int
    timestamp_us: int
    bins: list[TargetIQBin]

    @classmethod
    def from_bytes(cls, data: bytes) -> "TargetIQ":
        """Parse from TLV payload bytes."""
        if len(data) < 8:
            raise ValueError("TargetIQ header too short")

        num_bins, center_bin, timestamp = struct.unpack("<HHI", data[:8])
        bin_data = data[8:]

        bins = []
        for i in range(num_bins):
            offset = i * 8
            if offset + 8 > len(bin_data):
                break
            bin_idx, im, re, reserved = struct.unpack(
                "<Hhhh", bin_data[offset : offset + 8]
            )
            bins.append(TargetIQBin(bin_index=bin_idx, imag=im, real=re))

        return cls(
            num_bins=num_bins,
            center_bin=center_bin,
            timestamp_us=timestamp,
            bins=bins,
        )


@dataclass
class PhaseBin:
    """Single bin data for Phase Output TLV."""

    bin_index: int
    phase: int  # Fixed-point: -32768 to +32767 = -pi to +pi
    magnitude: int
    flags: int  # bit0=motion, bit1=valid

    @property
    def phase_radians(self) -> float:
        """Convert phase to radians."""
        import math

        return self.phase * math.pi / 32768.0

    @property
    def has_motion(self) -> bool:
        return bool(self.flags & 0x01)

    @property
    def is_valid(self) -> bool:
        return bool(self.flags & 0x02)


@dataclass
class PhaseOutput:
    """TLV 0x0520: Phase Output - Phase + magnitude for selected bins."""

    num_bins: int
    center_bin: int
    timestamp_us: int
    bins: list[PhaseBin]

    @classmethod
    def from_bytes(cls, data: bytes) -> "PhaseOutput":
        """Parse from TLV payload bytes."""
        if len(data) < 8:
            raise ValueError("PhaseOutput header too short")

        num_bins, center_bin, timestamp = struct.unpack("<HHI", data[:8])
        bin_data = data[8:]

        bins = []
        for i in range(num_bins):
            offset = i * 8
            if offset + 8 > len(bin_data):
                break
            bin_idx, phase, mag, flags = struct.unpack(
                "<HhHH", bin_data[offset : offset + 8]
            )
            bins.append(
                PhaseBin(bin_index=bin_idx, phase=phase, magnitude=mag, flags=flags)
            )

        return cls(
            num_bins=num_bins,
            center_bin=center_bin,
            timestamp_us=timestamp,
            bins=bins,
        )


@dataclass
class Presence:
    """TLV 0x0540: Presence detection result."""

    presence: int  # 0=absent, 1=present, 2=motion
    confidence: int  # 0-100
    range_q8: int  # Range in meters (Q8 fixed point)
    target_bin: int

    @property
    def range_meters(self) -> float:
        """Get range in meters."""
        return self.range_q8 / 256.0

    @property
    def presence_str(self) -> str:
        """Human-readable presence state."""
        states = {0: "absent", 1: "present", 2: "motion"}
        return states.get(self.presence, f"unknown({self.presence})")

    @classmethod
    def from_bytes(cls, data: bytes) -> "Presence":
        """Parse from TLV payload bytes."""
        if len(data) < 8:
            raise ValueError("Presence payload too short")

        presence, confidence, range_q8, target_bin, reserved = struct.unpack(
            "<BBHHH", data[:8]
        )

        return cls(
            presence=presence,
            confidence=confidence,
            range_q8=range_q8,
            target_bin=target_bin,
        )


@dataclass
class MotionStatus:
    """TLV 0x0550: Motion detection result."""

    motion_detected: bool
    motion_level: int  # 0-255
    motion_bin_count: int
    peak_motion_bin: int
    peak_motion_delta: int

    @classmethod
    def from_bytes(cls, data: bytes) -> "MotionStatus":
        """Parse from TLV payload bytes."""
        if len(data) < 8:
            raise ValueError("MotionStatus payload too short")

        detected, level, bin_count, peak_bin, peak_delta = struct.unpack(
            "<BBHHH", data[:8]
        )

        return cls(
            motion_detected=bool(detected),
            motion_level=level,
            motion_bin_count=bin_count,
            peak_motion_bin=peak_bin,
            peak_motion_delta=peak_delta,
        )


@dataclass
class TargetInfo:
    """TLV 0x0560: Target selection metadata."""

    primary_bin: int
    primary_magnitude: int
    primary_range_q8: int  # Range in meters (Q8 fixed point)
    confidence: int  # 0-100
    num_targets: int
    secondary_bin: int

    @property
    def range_meters(self) -> float:
        """Get primary target range in meters."""
        return self.primary_range_q8 / 256.0

    @classmethod
    def from_bytes(cls, data: bytes) -> "TargetInfo":
        """Parse from TLV payload bytes."""
        if len(data) < 12:
            raise ValueError("TargetInfo payload too short")

        (
            primary_bin,
            primary_mag,
            primary_range,
            confidence,
            num_targets,
            secondary_bin,
            reserved,
        ) = struct.unpack("<HHHBBHH", data[:12])

        return cls(
            primary_bin=primary_bin,
            primary_magnitude=primary_mag,
            primary_range_q8=primary_range,
            confidence=confidence,
            num_targets=num_targets,
            secondary_bin=secondary_bin,
        )


# =============================================================================
# TLV Parser Dispatch
# =============================================================================

TLV_PARSERS = {
    TLV_COMPLEX_RANGE_FFT: ComplexRangeFFT.from_bytes,
    TLV_TARGET_IQ: TargetIQ.from_bytes,
    TLV_PHASE_OUTPUT: PhaseOutput.from_bytes,
    TLV_PRESENCE: Presence.from_bytes,
    TLV_MOTION_STATUS: MotionStatus.from_bytes,
    TLV_TARGET_INFO: TargetInfo.from_bytes,
}


def parse_tlv_payload(tlv_type: int, data: bytes) -> Optional[object]:
    """Parse TLV payload using appropriate parser."""
    parser = TLV_PARSERS.get(tlv_type)
    if parser:
        try:
            return parser(data)
        except Exception as e:
            print(f"Warning: Failed to parse {tlv_name(tlv_type)}: {e}")
            return None
    return None
