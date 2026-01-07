/**
 * @file chirp.h
 * @brief Master header for chirp firmware modules
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Include this header to access all chirp functionality.
 */

#ifndef CHIRP_H
#define CHIRP_H

#include "../include/mmw_output.h"
#include "config_persist.h"
#include "error_codes.h"
#include "firmware_config.h"
#include "motion_detect.h"
#include "output_modes.h"
#include "phase_extract.h"
#include "power_mode.h"
#include "target_select.h"
#include "version.h"
#include "watchdog.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * Chirp Runtime State
     ******************************************************************************/

    /**
     * @brief Complete chirp runtime state
     */
    typedef struct Chirp_State_t
    {
        /** Output mode configuration */
        Chirp_OutputConfig outputConfig;

        /** Target selection configuration */
        Chirp_TargetConfig targetConfig;

        /** Target selection state */
        Chirp_TargetState targetState;

        /** Target selection result (updated each frame) */
        Chirp_TargetResult targetResult;

        /** Motion detection configuration */
        Chirp_MotionConfig motionConfig;

        /** Motion detection state */
        Chirp_MotionState motionState;

        /** Motion detection result (updated each frame) */
        Chirp_MotionResult motionResult;

        /** Phase output (updated each frame) */
        Chirp_PhaseOutput phaseOutput;

        /** Power management configuration */
        Chirp_PowerConfig powerConfig;

        /** Power management state */
        Chirp_PowerState powerState;

        /** Watchdog configuration */
        Chirp_WdgConfig watchdogConfig;

        /** Watchdog state */
        Chirp_WdgState watchdogState;

        /** Range resolution in meters (from radar config) */
        float rangeResolution;

        /** Number of range bins (from radar config) */
        uint16_t numRangeBins;

        /** Initialization flag */
        uint8_t initialized;

        /** Reserved */
        uint8_t reserved;

    } Chirp_State;

    /*******************************************************************************
     * Global State (defined in chirp.c)
     ******************************************************************************/

    extern Chirp_State gChirpState;

    /*******************************************************************************
     * Initialization and Configuration
     ******************************************************************************/

    /**
     * @brief Initialize chirp state with defaults
     */
    void Chirp_init(void);

    /**
     * @brief Configure chirp with radar parameters
     * @param rangeResolution Range resolution in meters
     * @param numRangeBins Number of range bins
     */
    void Chirp_configure(float rangeResolution, uint16_t numRangeBins);

    /*******************************************************************************
     * Frame Processing
     ******************************************************************************/

    /**
     * @brief Process a frame of radar data
     * @param radarCubeData Pointer to radar cube data (cmplx16ImRe_t)
     * @param numRangeBins Number of range bins
     * @param timestamp_us Current timestamp in microseconds
     * @return 0 on success
     */
    int32_t Chirp_processFrame(const int16_t *radarCubeData, uint16_t numRangeBins, uint32_t timestamp_us);

    /*******************************************************************************
     * TLV Output Generation
     ******************************************************************************/

    /**
     * @brief Get number of TLVs to output based on current mode
     * @return Number of TLVs that will be generated
     */
    uint32_t Chirp_getNumOutputTLVs(void);

    /**
     * @brief Get total size of TLV output data
     * @return Size in bytes
     */
    uint32_t Chirp_getOutputSize(void);

    /**
     * @brief Check if specific TLV type should be output
     * @param tlvType TLV type to check
     * @return 1 if should output, 0 otherwise
     */
    uint8_t Chirp_shouldOutputTLV(uint32_t tlvType);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_H */
