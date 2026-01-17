/**
 *   @file  vitalsign_dsp.h
 *
 *   @brief
 *      DSS Vital Signs Processing Module Interface
 *
 *   This module implements vital signs (heart rate and breathing rate)
 *   detection using radar phase data. Adapted from TI IWRL6432 for
 *   IWR6843AOPEVM dual-core architecture.
 *
 *  Copyright (C) 2024 Texas Instruments Incorporated
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef VITALSIGN_DSP_H
#define VITALSIGN_DSP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "vitalsign_common.h"
#include <ti/common/sys_common.h>

/**************************************************************************
 *************************** Error Codes **********************************
 **************************************************************************/

/** @brief Success */
#define VITALSIGNS_EOK                          (0)

/** @brief Generic error */
#define VITALSIGNS_EINVAL                       (-1)

/** @brief Not initialized error */
#define VITALSIGNS_ENOTINIT                     (-2)

/** @brief Out of memory error */
#define VITALSIGNS_ENOMEM                       (-3)

/**************************************************************************
 *************************** Function Declarations ************************
 **************************************************************************/

/**
 *  @brief Initialize the vital signs processing module
 *
 *  @param[in] cfg    Pointer to configuration structure
 *
 *  @return    VITALSIGNS_EOK on success, error code otherwise
 *
 *  @note Must be called before any other VitalSigns functions.
 *        Generates twiddle factors and initializes internal buffers.
 */
int32_t VitalSigns_init(VitalSigns_Config *cfg);

/**
 *  @brief De-initialize the vital signs processing module
 *
 *  Releases all resources and resets state.
 */
void VitalSigns_deinit(void);

/**
 *  @brief Reset the vital signs processing state
 *
 *  Clears accumulated data and resets frame counters.
 *  Called when target is lost or configuration changes.
 */
void VitalSigns_reset(void);

/**
 *  @brief Update configuration at runtime
 *
 *  @param[in] cfg    Pointer to new configuration
 *
 *  @return    VITALSIGNS_EOK on success, error code otherwise
 */
int32_t VitalSigns_updateConfig(VitalSigns_Config *cfg);

/**
 *  @brief Process one frame of radar data for vital signs
 *
 *  @param[in] radarCube       Pointer to radar cube data (cmplx16ImRe_t format)
 *  @param[in] numRangeBins    Number of range bins in radar cube
 *  @param[in] numDopplerChirps Number of Doppler chirps (slow time samples)
 *  @param[in] numVirtualAnt   Number of virtual antennas
 *  @param[in] targetRangeBin  Center range bin for target (from tracker or config)
 *
 *  @return    VITALSIGNS_EOK on success, error code otherwise
 *
 *  @note This function should be called once per frame after range FFT.
 *        Vital signs output is updated every VS_REFRESH_RATE frames.
 */
int32_t VitalSigns_processFrame(
    cmplx16ImRe_t *radarCube,
    uint16_t numRangeBins,
    uint16_t numDopplerChirps,
    uint8_t  numVirtualAnt,
    uint16_t targetRangeBin
);

/**
 *  @brief Get the latest vital signs output
 *
 *  @param[out] output    Pointer to output structure to fill
 *
 *  @return    VITALSIGNS_EOK on success, error code otherwise
 *
 *  @note Output is valid after at least VS_MASK_LOOP_NO processing cycles.
 */
int32_t VitalSigns_getOutput(VitalSigns_Output *output);

/**
 *  @brief Check if vital signs output is ready
 *
 *  @return    1 if output is ready and valid, 0 otherwise
 */
int32_t VitalSigns_isOutputReady(void);

/**
 *  @brief Get range bin from tracker target position
 *
 *  Converts tracker's Cartesian position to a range bin index
 *  for vital signs processing.
 *
 *  @param[in] targetX         Target X position in meters
 *  @param[in] targetY         Target Y position in meters
 *  @param[in] rangeResolution Range resolution in meters per bin
 *
 *  @return    Range bin index
 */
uint16_t VitalSigns_getRangeBinFromPosition(
    float targetX,
    float targetY,
    float rangeResolution
);

/**
 *  @brief Handle target loss scenario
 *
 *  Called when tracker loses target. Implements persistence logic
 *  to continue VS processing briefly after target loss.
 *
 *  @param[in] targetLost    1 if target is lost, 0 if present
 *
 *  @return    1 if VS should continue processing, 0 if should stop
 */
int32_t VitalSigns_handleTargetLoss(int32_t targetLost);

/**
 *  @brief Get current processing state (for debug)
 *
 *  @param[out] state    Pointer to state structure to fill
 */
void VitalSigns_getState(VitalSigns_State *state);

#ifdef __cplusplus
}
#endif

#endif /* VITALSIGN_DSP_H */
