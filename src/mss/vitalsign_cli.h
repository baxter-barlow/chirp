/**
 *   @file  vitalsign_cli.h
 *
 *   @brief
 *      MSS Vital Signs CLI Module Interface
 *
 *   This module provides CLI command handlers for configuring vital signs
 *   processing on the IWR6843AOPEVM.
 *
 *   CLI Commands:
 *   - vitalsign <enable> <trackerIntegration>
 *   - VSRangeIdxCfg <startBin> <numBins>
 *   - VSTargetId <targetId>
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
#ifndef VITALSIGN_CLI_H
#define VITALSIGN_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "vitalsign_common.h"

/**************************************************************************
 *************************** Function Declarations ************************
 **************************************************************************/

/**
 *  @brief Initialize vital signs CLI commands
 *
 *  Registers vital signs CLI commands with the CLI framework.
 *  Should be called during MmwDemo_CLIInit().
 *
 *  @param[in] cliCfg    Pointer to CLI configuration (CLI_Cfg)
 *  @param[in] startIdx  Starting index in cliCfg.tableEntry array
 *
 *  @return    Number of commands registered
 */
int32_t VitalSignsCLI_init(void *cliCfg, uint32_t startIdx);

/**
 *  @brief CLI handler for 'vitalsign' command
 *
 *  Command syntax: vitalsign <enable> <trackerIntegration>
 *    enable: 0=off, 1=on
 *    trackerIntegration: 0=fixed range, 1=use tracker
 *
 *  Example: vitalsign 1 1
 *
 *  @param[in] argc    Number of arguments
 *  @param[in] argv    Arguments array
 *
 *  @return    0 on success, -1 on error
 */
int32_t VitalSignsCLI_vitalSignCmd(int32_t argc, char* argv[]);

/**
 *  @brief CLI handler for 'VSRangeIdxCfg' command
 *
 *  Command syntax: VSRangeIdxCfg <startBin> <numBins>
 *    startBin: Starting range bin (0-255)
 *    numBins: Number of bins to process (1-5)
 *
 *  Example: VSRangeIdxCfg 20 5
 *
 *  @param[in] argc    Number of arguments
 *  @param[in] argv    Arguments array
 *
 *  @return    0 on success, -1 on error
 */
int32_t VitalSignsCLI_rangeIdxCfgCmd(int32_t argc, char* argv[]);

/**
 *  @brief CLI handler for 'VSTargetId' command
 *
 *  Command syntax: VSTargetId <targetId>
 *    targetId: Tracker target ID (0-249, 255=nearest)
 *
 *  Example: VSTargetId 0
 *
 *  @param[in] argc    Number of arguments
 *  @param[in] argv    Arguments array
 *
 *  @return    0 on success, -1 on error
 */
int32_t VitalSignsCLI_targetIdCmd(int32_t argc, char* argv[]);

/**
 *  @brief Get current vital signs configuration
 *
 *  @param[out] cfg    Pointer to configuration structure to fill
 *
 *  @return    0 on success, -1 if cfg is NULL
 */
int32_t VitalSignsCLI_getConfig(VitalSigns_Config *cfg);

/**
 *  @brief Check if vital signs configuration is pending
 *
 *  @return    1 if configuration is pending, 0 otherwise
 */
int32_t VitalSignsCLI_isConfigPending(void);

/**
 *  @brief Clear configuration pending flag
 *
 *  Called after configuration has been sent to DSS.
 */
void VitalSignsCLI_clearConfigPending(void);

#ifdef __cplusplus
}
#endif

#endif /* VITALSIGN_CLI_H */
