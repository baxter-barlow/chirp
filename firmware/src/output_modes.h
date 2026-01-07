/**
 * @file output_modes.h
 * @brief Output mode definitions and management for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#ifndef CHIRP_OUTPUT_MODES_H
#define CHIRP_OUTPUT_MODES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Output Mode Enumeration
 ******************************************************************************/

/**
 * @brief Output modes supported by chirp firmware
 */
typedef enum Chirp_OutputMode_e
{
    /** Full radar cube I/Q - for development/debugging (~800 KB/s) */
    CHIRP_OUTPUT_MODE_RAW_IQ = 0,

    /** Complex range profile - all bins (~10 KB/s at 10fps) */
    CHIRP_OUTPUT_MODE_RANGE_FFT = 1,

    /** I/Q for selected target bins only (~1 KB/s) */
    CHIRP_OUTPUT_MODE_TARGET_IQ = 2,

    /** Phase + magnitude for selected bins (~0.5 KB/s) */
    CHIRP_OUTPUT_MODE_PHASE = 3,

    /** Simple presence detection flag (~0.02 KB/s) */
    CHIRP_OUTPUT_MODE_PRESENCE = 4,

    /** Number of output modes */
    CHIRP_OUTPUT_MODE_COUNT
} Chirp_OutputMode;

/*******************************************************************************
 * Output Mode Configuration
 ******************************************************************************/

/**
 * @brief Configuration for output mode system
 */
typedef struct Chirp_OutputConfig_t
{
    /** Current output mode */
    Chirp_OutputMode mode;

    /** Enable motion detection output (TLV 0x0550) */
    uint8_t enableMotionOutput;

    /** Enable target info output (TLV 0x0560) */
    uint8_t enableTargetInfo;

    /** Reserved for alignment */
    uint16_t reserved;

} Chirp_OutputConfig;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Initialize output mode system with defaults
 * @param config Pointer to configuration structure to initialize
 */
void Chirp_OutputMode_init(Chirp_OutputConfig *config);

/**
 * @brief Set output mode
 * @param config Pointer to configuration structure
 * @param mode New output mode
 * @return 0 on success, -1 on invalid mode
 */
int32_t Chirp_OutputMode_set(Chirp_OutputConfig *config, Chirp_OutputMode mode);

/**
 * @brief Get current output mode
 * @param config Pointer to configuration structure
 * @return Current output mode
 */
Chirp_OutputMode Chirp_OutputMode_get(const Chirp_OutputConfig *config);

/**
 * @brief Get string name for output mode (for CLI)
 * @param mode Output mode
 * @return Static string name
 */
const char* Chirp_OutputMode_getName(Chirp_OutputMode mode);

/**
 * @brief Parse output mode from string (for CLI)
 * @param str String to parse (e.g., "PHASE", "TARGET_IQ")
 * @param mode Pointer to store parsed mode
 * @return 0 on success, -1 on parse error
 */
int32_t Chirp_OutputMode_parse(const char *str, Chirp_OutputMode *mode);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_OUTPUT_MODES_H */
