/**
 * @file motion_detect.h
 * @brief Motion detection for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Detects significant motion that would corrupt micro-motion measurements.
 * Uses frame-to-frame magnitude comparison.
 */

#ifndef CHIRP_MOTION_DETECT_H
#define CHIRP_MOTION_DETECT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Constants
 ******************************************************************************/

/** Maximum range bins to monitor for motion */
#define CHIRP_MOTION_MAX_BINS       64

/** Default motion threshold (magnitude delta) */
#define CHIRP_MOTION_THRESHOLD_DEFAULT  500

/** Number of frames for baseline averaging */
#define CHIRP_MOTION_BASELINE_FRAMES    10

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/**
 * @brief Motion detection configuration
 */
typedef struct Chirp_MotionConfig_t
{
    /** Enable motion detection */
    uint8_t enabled;

    /** Reserved for alignment */
    uint8_t reserved;

    /** Motion detection threshold (magnitude delta) */
    uint16_t threshold;

    /** Minimum bin to monitor */
    uint16_t minBin;

    /** Maximum bin to monitor */
    uint16_t maxBin;

} Chirp_MotionConfig;

/**
 * @brief Motion detection result (TLV 0x0550 payload)
 */
typedef struct Chirp_MotionResult_t
{
    /** Motion detected flag (0 or 1) */
    uint8_t motionDetected;

    /** Motion level (0-255, normalized) */
    uint8_t motionLevel;

    /** Number of bins with motion */
    uint16_t motionBinCount;

    /** Peak motion bin index */
    uint16_t peakMotionBin;

    /** Peak motion magnitude delta */
    uint16_t peakMotionDelta;

} Chirp_MotionResult;

/**
 * @brief Motion detection internal state
 */
typedef struct Chirp_MotionState_t
{
    /** Previous frame magnitude (for comparison) */
    uint16_t prevMagnitude[CHIRP_MOTION_MAX_BINS];

    /** Number of bins stored */
    uint16_t numBins;

    /** Frame counter */
    uint16_t frameCount;

    /** First frame flag */
    uint8_t firstFrame;

    /** Reserved */
    uint8_t reserved[3];

} Chirp_MotionState;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize motion detection
 * @param config Configuration structure to initialize
 * @param state State structure to initialize
 */
void Chirp_Motion_init(Chirp_MotionConfig *config, Chirp_MotionState *state);

/**
 * @brief Configure motion detection
 * @param config Configuration structure
 * @param enabled Enable/disable
 * @param threshold Motion threshold
 * @param minBin Minimum bin to monitor
 * @param maxBin Maximum bin to monitor
 * @return 0 on success, -1 on error
 */
int32_t Chirp_Motion_configure(Chirp_MotionConfig *config,
                                uint8_t enabled,
                                uint16_t threshold,
                                uint16_t minBin,
                                uint16_t maxBin);

/**
 * @brief Process frame for motion detection
 * @param config Configuration
 * @param state State (updated with current frame)
 * @param magnitude Array of range bin magnitudes
 * @param numBins Number of bins
 * @param result Output motion result
 * @return 0 on success, -1 on error
 */
int32_t Chirp_Motion_process(const Chirp_MotionConfig *config,
                              Chirp_MotionState *state,
                              const uint16_t *magnitude,
                              uint16_t numBins,
                              Chirp_MotionResult *result);

/**
 * @brief Reset motion detection state
 * @param state State to reset
 */
void Chirp_Motion_reset(Chirp_MotionState *state);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_MOTION_DETECT_H */
