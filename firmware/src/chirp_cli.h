/**
 * @file chirp_cli.h
 * @brief CLI command handlers for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#ifndef CHIRP_CLI_H
#define CHIRP_CLI_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * CLI Command Handlers
     * These can be registered with the TI CLI framework in mmw_cli.c
     ******************************************************************************/

    /**
     * @brief Set chirp output mode
     * Usage: chirpOutputMode <mode> [enableMotion] [enableTargetInfo]
     * mode: 0=RAW_IQ, 1=RANGE_FFT, 2=TARGET_IQ, 3=PHASE, 4=PRESENCE
     */
    int32_t Chirp_CLI_outputMode(int32_t argc, char *argv[]);

    /**
     * @brief Configure target selection
     * Usage: chirpTargetCfg <minRange_m> <maxRange_m> <minSNR_dB> <numTrackBins>
     */
    int32_t Chirp_CLI_targetCfg(int32_t argc, char *argv[]);

    /**
     * @brief Configure motion detection
     * Usage: chirpMotionCfg <enabled> <threshold> <minBin> <maxBin>
     */
    int32_t Chirp_CLI_motionCfg(int32_t argc, char *argv[]);

    /**
     * @brief Display chirp status
     * Usage: chirpStatus
     */
    int32_t Chirp_CLI_status(int32_t argc, char *argv[]);

    /**
     * @brief Reset chirp state
     * Usage: chirpReset
     */
    int32_t Chirp_CLI_reset(int32_t argc, char *argv[]);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_CLI_H */
