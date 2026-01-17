/**
 *   @file  vitalsign_cli.c
 *
 *   @brief
 *      MSS Vital Signs CLI Module Implementation
 *
 *   This module provides CLI command handlers for configuring vital signs
 *   processing on the IWR6843AOPEVM.
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

/**************************************************************************
 *************************** Include Files ********************************
 **************************************************************************/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <ti/common/sys_common.h>
#include <ti/drivers/uart/UART.h>
#include <ti/utils/cli/cli.h>

#include "vitalsign_cli.h"
#include "vitalsign_common.h"

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/** @brief Maximum number of range bins */
#define VS_CLI_MAX_RANGE_BINS       256

/** @brief Maximum target ID value */
#define VS_CLI_MAX_TARGET_ID        255

/**************************************************************************
 *************************** Global Variables *****************************
 **************************************************************************/

/** @brief Vital signs configuration storage */
static VitalSigns_Config gVsCfg = {0};

/** @brief Configuration pending flag */
static uint8_t gVsCfgPending = 0;

/**************************************************************************
 *************************** Function Implementations *********************
 **************************************************************************/

/**
 *  @brief Initialize vital signs CLI commands
 */
int32_t VitalSignsCLI_init(void *cliCfgPtr, uint32_t startIdx)
{
    CLI_Cfg *cliCfg = (CLI_Cfg *)cliCfgPtr;
    uint32_t idx = startIdx;

    if (cliCfg == NULL)
    {
        return 0;
    }

    /* Register 'vitalsign' command */
    cliCfg->tableEntry[idx].cmd = "vitalsign";
    cliCfg->tableEntry[idx].helpString = "<enable> <trackerIntegration>";
    cliCfg->tableEntry[idx].cmdHandlerFxn = VitalSignsCLI_vitalSignCmd;
    idx++;

    /* Register 'VSRangeIdxCfg' command */
    cliCfg->tableEntry[idx].cmd = "VSRangeIdxCfg";
    cliCfg->tableEntry[idx].helpString = "<startBin> <numBins>";
    cliCfg->tableEntry[idx].cmdHandlerFxn = VitalSignsCLI_rangeIdxCfgCmd;
    idx++;

    /* Register 'VSTargetId' command */
    cliCfg->tableEntry[idx].cmd = "VSTargetId";
    cliCfg->tableEntry[idx].helpString = "<targetId>";
    cliCfg->tableEntry[idx].cmdHandlerFxn = VitalSignsCLI_targetIdCmd;
    idx++;

    /* Initialize default configuration */
    memset(&gVsCfg, 0, sizeof(VitalSigns_Config));
    gVsCfg.enabled = 0;
    gVsCfg.trackerIntegration = 0;
    gVsCfg.targetId = 255;  /* Default: nearest target */
    gVsCfg.rangeBinStart = 20;  /* Default: ~1.5m at typical resolution */
    gVsCfg.numRangeBins = 5;
    gVsCfg.rangeResolution = 0.0732f;  /* Default for typical VS chirp */

    return (int32_t)(idx - startIdx);
}

/**
 *  @brief CLI handler for 'vitalsign' command
 */
int32_t VitalSignsCLI_vitalSignCmd(int32_t argc, char* argv[])
{
    uint8_t enable;
    uint8_t trackerIntegration;

    /* Validate argument count */
    if (argc != 3)
    {
        CLI_write("Error: vitalsign <enable> <trackerIntegration>\n");
        CLI_write("  enable: 0=off, 1=on\n");
        CLI_write("  trackerIntegration: 0=fixed range, 1=use tracker\n");
        return -1;
    }

    /* Parse arguments */
    enable = (uint8_t)atoi(argv[1]);
    trackerIntegration = (uint8_t)atoi(argv[2]);

    /* Validate enable parameter */
    if (enable > 1)
    {
        CLI_write("Error: enable must be 0 or 1\n");
        return -1;
    }

    /* Validate tracker integration parameter */
    if (trackerIntegration > 1)
    {
        CLI_write("Error: trackerIntegration must be 0 or 1\n");
        return -1;
    }

    /* Store configuration */
    gVsCfg.enabled = enable;
    gVsCfg.trackerIntegration = trackerIntegration;
    gVsCfgPending = 1;

    if (enable)
    {
        CLI_write("Vital Signs enabled, tracker integration: %s\n",
                  trackerIntegration ? "ON" : "OFF");
    }
    else
    {
        CLI_write("Vital Signs disabled\n");
    }

    return 0;
}

/**
 *  @brief CLI handler for 'VSRangeIdxCfg' command
 */
int32_t VitalSignsCLI_rangeIdxCfgCmd(int32_t argc, char* argv[])
{
    uint16_t startBin;
    uint16_t numBins;

    /* Validate argument count */
    if (argc != 3)
    {
        CLI_write("Error: VSRangeIdxCfg <startBin> <numBins>\n");
        CLI_write("  startBin: Starting range bin (0-%d)\n", VS_CLI_MAX_RANGE_BINS - 1);
        CLI_write("  numBins: Number of bins (1-%d)\n", VS_NUM_RANGE_SEL_BIN);
        return -1;
    }

    /* Parse arguments */
    startBin = (uint16_t)atoi(argv[1]);
    numBins = (uint16_t)atoi(argv[2]);

    /* Validate start bin */
    if (startBin >= VS_CLI_MAX_RANGE_BINS)
    {
        CLI_write("Error: startBin must be 0-%d\n", VS_CLI_MAX_RANGE_BINS - 1);
        return -1;
    }

    /* Validate number of bins */
    if (numBins == 0 || numBins > VS_NUM_RANGE_SEL_BIN)
    {
        CLI_write("Error: numBins must be 1-%d\n", VS_NUM_RANGE_SEL_BIN);
        return -1;
    }

    /* Ensure we don't exceed array bounds */
    if (startBin + numBins > VS_CLI_MAX_RANGE_BINS)
    {
        CLI_write("Error: startBin + numBins exceeds %d\n", VS_CLI_MAX_RANGE_BINS);
        return -1;
    }

    /* Store configuration */
    gVsCfg.rangeBinStart = startBin;
    gVsCfg.numRangeBins = numBins;
    gVsCfgPending = 1;

    CLI_write("VS Range config: start=%d, numBins=%d\n", startBin, numBins);

    return 0;
}

/**
 *  @brief CLI handler for 'VSTargetId' command
 */
int32_t VitalSignsCLI_targetIdCmd(int32_t argc, char* argv[])
{
    uint16_t targetId;

    /* Validate argument count */
    if (argc != 2)
    {
        CLI_write("Error: VSTargetId <targetId>\n");
        CLI_write("  targetId: 0-249 for specific target, 255 for nearest\n");
        return -1;
    }

    /* Parse argument */
    targetId = (uint16_t)atoi(argv[1]);

    /* Validate target ID */
    if (targetId > VS_CLI_MAX_TARGET_ID)
    {
        CLI_write("Error: targetId must be 0-%d\n", VS_CLI_MAX_TARGET_ID);
        return -1;
    }

    /* Store configuration */
    gVsCfg.targetId = (uint8_t)targetId;
    gVsCfgPending = 1;

    if (targetId == 255)
    {
        CLI_write("VS Target: nearest\n");
    }
    else
    {
        CLI_write("VS Target ID: %d\n", targetId);
    }

    return 0;
}

/**
 *  @brief Get current vital signs configuration
 */
int32_t VitalSignsCLI_getConfig(VitalSigns_Config *cfg)
{
    if (cfg == NULL)
    {
        return -1;
    }

    memcpy(cfg, &gVsCfg, sizeof(VitalSigns_Config));

    return 0;
}

/**
 *  @brief Check if vital signs configuration is pending
 */
int32_t VitalSignsCLI_isConfigPending(void)
{
    return gVsCfgPending;
}

/**
 *  @brief Clear configuration pending flag
 */
void VitalSignsCLI_clearConfigPending(void)
{
    gVsCfgPending = 0;
}
