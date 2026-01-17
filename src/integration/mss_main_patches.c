/**
 * @file mss_main_patches.c
 *
 * @brief Code patches to integrate vital signs into mss_main.c
 *
 * This file documents the specific code changes needed in mss_main.c
 * for vital signs integration.
 */

/*****************************************************************************
 * PATCH 1: Add includes at top of file (after existing demo includes)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
#include "vitalsign_common.h"
#include "vitalsign_cli.h"
#endif


/*****************************************************************************
 * PATCH 2: Add VS TLV output in MmwDemo_transmitProcessedOutput()
 *
 * Add this code block after the stats TLV section (around line 1485)
 * but before header.numTLVs is set.
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
    /* Vital Signs TLV */
    if (gMmwMssMCB.subFrameCfg[result->subFrameIdx].vitalSignsCfg.enabled)
    {
        tl[tlvIdx].type = MMWDEMO_OUTPUT_MSG_VITALSIGNS;
        tl[tlvIdx].length = sizeof(MmwDemo_output_message_vitalsigns);
        packetLen += sizeof(MmwDemo_output_message_tl) + tl[tlvIdx].length;
        tlvIdx++;
    }
#endif


/*****************************************************************************
 * PATCH 3: Add VS data transmission in MmwDemo_transmitProcessedOutput()
 *
 * Add after the temperature stats transmission block (around line 1590)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
    /* Send Vital Signs data */
    if (gMmwMssMCB.subFrameCfg[result->subFrameIdx].vitalSignsCfg.enabled)
    {
        MmwDemo_output_message_vitalsigns vsOutput;
        VitalSigns_Output *pVsOut;

        /* Get VS output from HSRAM (DSS writes it there) */
        pVsOut = (VitalSigns_Output *)SOC_translateAddress(
            (uint32_t)&gHSRAM.vitalSignsOutput,
            SOC_TranslateAddr_Dir_FROM_OTHER_CPU,
            &errCode);

        if (pVsOut != NULL)
        {
            vsOutput.targetId = pVsOut->id;
            vsOutput.rangeBin = pVsOut->rangeBin;
            vsOutput.heartRate = pVsOut->heartRate;
            vsOutput.breathingRate = pVsOut->breathingRate;
            vsOutput.breathingDeviation = pVsOut->breathingDeviation;
            vsOutput.valid = pVsOut->valid;
            vsOutput.reserved[0] = 0;
            vsOutput.reserved[1] = 0;
            vsOutput.reserved[2] = 0;

            UART_writePolling(uartHandle,
                              (uint8_t*)&tl[tlvIdx],
                              sizeof(MmwDemo_output_message_tl));

            UART_writePolling(uartHandle,
                              (uint8_t*)&vsOutput,
                              sizeof(MmwDemo_output_message_vitalsigns));
            tlvIdx++;
        }
    }
#endif


/*****************************************************************************
 * PATCH 4: Add VS config to preStartCommonCfg in MmwDemo_configSensor()
 *
 * Add in the function where DPC configuration is sent (around line 2100)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
    /* Send vital signs configuration to DSS */
    if (VitalSignsCLI_isConfigPending())
    {
        VitalSigns_Config vsCfg;
        VitalSignsCLI_getConfig(&vsCfg);

        /* Store in subframe config */
        memcpy(&gMmwMssMCB.subFrameCfg[subFrameIdx].vitalSignsCfg,
               &vsCfg, sizeof(VitalSigns_Config));

        VitalSignsCLI_clearConfigPending();
    }
#endif


/*****************************************************************************
 * PATCH 5: Modify HSRAM structure in mmw_output.h
 *
 * Add VS output storage to the HSRAM structure.
 *****************************************************************************/

/*
 * In mmw_output.h, modify MmwDemo_HSRAM_t structure:
 */

#ifdef VITAL_SIGNS_ENABLED
#define MMWDEMO_HSRAM_PAYLOAD_SIZE_VS (SOC_HSRAM_SIZE - sizeof(DPC_ObjectDetection_ExecuteResult) - \
                                       sizeof(MmwDemo_output_message_stats) - \
                                       sizeof(VitalSigns_Output))
#else
#define MMWDEMO_HSRAM_PAYLOAD_SIZE_VS MMWDEMO_HSRAM_PAYLOAD_SIZE
#endif

typedef struct MmwDemo_HSRAM_t
{
    /*! @brief   DPC execution result */
    DPC_ObjectDetection_ExecuteResult result;

    /*! @brief   Output message stats reported by DSS */
    MmwDemo_output_message_stats   outStats;

#ifdef VITAL_SIGNS_ENABLED
    /*! @brief   Vital signs output from DSS */
    VitalSigns_Output              vitalSignsOutput;
#endif

    /*! @brief   Payload data of result */
    uint8_t                        payload[MMWDEMO_HSRAM_PAYLOAD_SIZE_VS];
} MmwDemo_HSRAM;
