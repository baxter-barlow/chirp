/**
 * @file firmware_config.h
 * @brief Master configuration for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * This file contains all compile-time configuration options.
 * Edit this file to enable/disable features before building.
 */

#ifndef CHIRP_FIRMWARE_CONFIG_H
#define CHIRP_FIRMWARE_CONFIG_H

/* ==========================================================================
 * Output Mode Configuration
 * ========================================================================== */

/**
 * Enable Complex Range FFT output (TLV 0x0500)
 * Outputs full I/Q data for all range bins
 * Bandwidth: ~1040 bytes/frame at 256 bins
 */
#define MMWDEMO_OUTPUT_COMPLEX_RANGE_FFT_ENABLE 1

/**
 * Enable Target I/Q output (TLV 0x0510)
 * Outputs I/Q only for selected target bins
 * Bandwidth: ~40 bytes/frame for 4 bins
 */
#define MMWDEMO_OUTPUT_TARGET_IQ_ENABLE 0 /* Phase 2 */

/**
 * Enable Phase output (TLV 0x0520)
 * Outputs computed phase + magnitude for target bins
 * Bandwidth: ~20 bytes/frame for 4 bins
 */
#define MMWDEMO_OUTPUT_PHASE_ENABLE 0 /* Phase 2 */

/**
 * Enable Presence detection (TLV 0x0540)
 * Simple presence/absence flag
 * Bandwidth: ~8 bytes/frame
 */
#define MMWDEMO_OUTPUT_PRESENCE_ENABLE 0 /* Phase 2 */

/**
 * Enable Motion detection (TLV 0x0550)
 * Motion status and magnitude
 * Bandwidth: ~12 bytes/frame
 */
#define MMWDEMO_OUTPUT_MOTION_ENABLE 0 /* Phase 2 */

/**
 * Enable Target info output (TLV 0x0560)
 * Target selection metadata
 * Bandwidth: ~24 bytes/frame
 */
#define MMWDEMO_OUTPUT_TARGET_INFO_ENABLE 0 /* Phase 2 */

/* ==========================================================================
 * Target Selection Configuration
 * ========================================================================== */

/** Default minimum range for target search (meters) */
#define TARGET_RANGE_MIN_DEFAULT 0.5f

/** Default maximum range for target search (meters) */
#define TARGET_RANGE_MAX_DEFAULT 2.5f

/** Hysteresis threshold for target switching (dB) */
#define TARGET_HYSTERESIS_DB 3.0f

/** Number of bins to track around primary target */
#define TARGET_TRACK_BINS 4

/* ==========================================================================
 * Motion Detection Configuration
 * ========================================================================== */

/** Default motion detection threshold (dB) */
#define MOTION_THRESHOLD_DEFAULT 6.0f

/** Number of frames to average for motion baseline */
#define MOTION_BASELINE_FRAMES 10

/* ==========================================================================
 * Power Management Configuration
 * ========================================================================== */

/** Enable duty cycling (Phase 3) */
#define POWER_MGMT_ENABLE 0

/** Default active frames before sleep */
#define POWER_ACTIVE_FRAMES 100

/** Default sleep frames */
#define POWER_SLEEP_FRAMES 100

/* ==========================================================================
 * UART Configuration
 * ========================================================================== */

/**
 * Enable DMA for UART data output
 * When enabled, UART transfers use DMA to free CPU during transmission.
 * The CPU is free to handle interrupts and other tasks during DMA transfer.
 * Disable this if hardware testing reveals DMA issues.
 */
#define CHIRP_UART_DMA_ENABLE 0

/** UART TX DMA channel (1-31, must not conflict with other DMA users) */
#define CHIRP_UART_TX_DMA_CHANNEL 1U

/** UART RX DMA channel (1-31, must not conflict with other DMA users) */
#define CHIRP_UART_RX_DMA_CHANNEL 2U

/* ==========================================================================
 * Debug Configuration
 * ========================================================================== */

/** Enable debug output via UART */
#define DEBUG_OUTPUT_ENABLE 0

/** Enable timing measurements */
#define DEBUG_TIMING_ENABLE 0

#endif /* CHIRP_FIRMWARE_CONFIG_H */
