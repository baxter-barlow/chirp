/**
 * @file target_select.h
 * @brief Target auto-selection algorithm for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Automatically identifies the range bin containing the primary target
 * (strongest static reflector in configured range).
 */

#ifndef CHIRP_TARGET_SELECT_H
#define CHIRP_TARGET_SELECT_H

#include <stdint.h>
#include <ti/common/sys_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Constants
 ******************************************************************************/

/** Maximum number of bins to track around primary target */
#define CHIRP_TARGET_MAX_TRACK_BINS     8

/** Default minimum range (meters) */
#define CHIRP_TARGET_MIN_RANGE_DEFAULT  0.3f

/** Default maximum range (meters) */
#define CHIRP_TARGET_MAX_RANGE_DEFAULT  3.0f

/** Default minimum SNR threshold (dB) */
#define CHIRP_TARGET_MIN_SNR_DEFAULT    10

/** Default hysteresis for target switching (bins) */
#define CHIRP_TARGET_HYSTERESIS_DEFAULT 2

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/**
 * @brief Configuration for target selection
 */
typedef struct Chirp_TargetConfig_t
{
    /** Minimum range to search for target (meters) */
    float minRange_m;

    /** Maximum range to search for target (meters) */
    float maxRange_m;

    /** Minimum SNR threshold (dB) */
    uint8_t minSNR_dB;

    /** Number of bins to track around primary target (1-8) */
    uint8_t numTrackBins;

    /** Hysteresis bins before switching target */
    uint8_t hysteresisBins;

    /** Reserved for alignment */
    uint8_t reserved;

} Chirp_TargetConfig;

/**
 * @brief Result of target selection
 */
typedef struct Chirp_TargetResult_t
{
    /** Primary target range bin index */
    uint16_t primaryBin;

    /** Secondary target range bin index (if present) */
    uint16_t secondaryBin;

    /** Primary target magnitude (linear) */
    uint16_t primaryMagnitude;

    /** Secondary target magnitude (linear) */
    uint16_t secondaryMagnitude;

    /** Confidence score (0-100) */
    uint8_t confidence;

    /** Number of targets detected */
    uint8_t numTargets;

    /** Target valid flag */
    uint8_t valid;

    /** Reserved for alignment */
    uint8_t reserved;

    /** Estimated range to primary target (meters, Q8 fixed point) */
    uint16_t primaryRange_Q8;

    /** Bins to output for TARGET_IQ/PHASE modes */
    uint16_t trackBins[CHIRP_TARGET_MAX_TRACK_BINS];

    /** Number of track bins populated */
    uint8_t numTrackBinsUsed;

    /** Padding */
    uint8_t pad[3];

} Chirp_TargetResult;

/**
 * @brief Internal state for target selection algorithm
 */
typedef struct Chirp_TargetState_t
{
    /** Previous primary bin (for hysteresis) */
    uint16_t prevPrimaryBin;

    /** Frames since target change */
    uint16_t framesSinceChange;

    /** Target locked flag */
    uint8_t locked;

    /** Reserved */
    uint8_t reserved[3];

} Chirp_TargetState;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize target selection with default configuration
 * @param config Configuration structure to initialize
 * @param state State structure to initialize
 */
void Chirp_TargetSelect_init(Chirp_TargetConfig *config, Chirp_TargetState *state);

/**
 * @brief Configure target selection parameters
 * @param config Configuration structure
 * @param minRange Minimum range (meters)
 * @param maxRange Maximum range (meters)
 * @param minSNR Minimum SNR (dB)
 * @param numBins Number of bins to track
 * @return 0 on success, -1 on invalid parameters
 */
int32_t Chirp_TargetSelect_configure(Chirp_TargetConfig *config,
                                      float minRange,
                                      float maxRange,
                                      uint8_t minSNR,
                                      uint8_t numBins);

/**
 * @brief Process range profile to find target
 * @param config Configuration
 * @param state Algorithm state (updated)
 * @param rangeMagnitude Array of range bin magnitudes (linear)
 * @param numBins Number of bins in array
 * @param rangeResolution Range resolution in meters
 * @param result Output: target selection result
 * @return 0 on success, -1 on error
 */
int32_t Chirp_TargetSelect_process(const Chirp_TargetConfig *config,
                                    Chirp_TargetState *state,
                                    const uint16_t *rangeMagnitude,
                                    uint16_t numBins,
                                    float rangeResolution,
                                    Chirp_TargetResult *result);

/**
 * @brief Convert range bin to meters
 * @param bin Range bin index
 * @param rangeResolution Range resolution (meters/bin)
 * @return Range in meters
 */
float Chirp_TargetSelect_binToRange(uint16_t bin, float rangeResolution);

/**
 * @brief Convert range in meters to bin index
 * @param range Range in meters
 * @param rangeResolution Range resolution (meters/bin)
 * @return Range bin index
 */
uint16_t Chirp_TargetSelect_rangeToBin(float range, float rangeResolution);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_TARGET_SELECT_H */
