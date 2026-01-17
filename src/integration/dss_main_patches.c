/**
 * @file dss_main_patches.c
 *
 * @brief Code patches to integrate vital signs into dss_main.c
 *
 * This file documents the specific code changes needed in dss_main.c
 * for vital signs integration.
 */

/*****************************************************************************
 * PATCH 1: Add includes at top of file (after existing demo includes)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
#include "vitalsign_common.h"
#include "vitalsign_dsp.h"
#endif


/*****************************************************************************
 * PATCH 2: Add VS globals
 *
 * Add near other global variables (around line 120)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
/* Vital signs configuration received from MSS */
static VitalSigns_Config gVitalSignsConfig = {0};

/* Flag indicating VS has been initialized */
static uint8_t gVitalSignsInitialized = 0;
#endif


/*****************************************************************************
 * PATCH 3: Add VS initialization in MmwDemo_dssInitTask()
 *
 * Add after DPM_synch() completes successfully (around line 810)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
    /* Initialize vital signs module with default config */
    {
        VitalSigns_Config vsInitCfg;
        memset(&vsInitCfg, 0, sizeof(VitalSigns_Config));
        vsInitCfg.enabled = 0;
        vsInitCfg.trackerIntegration = 0;
        vsInitCfg.targetId = 255;
        vsInitCfg.rangeBinStart = 20;
        vsInitCfg.numRangeBins = 5;
        vsInitCfg.rangeResolution = 0.0732f;

        if (VitalSigns_init(&vsInitCfg) == VITALSIGNS_EOK)
        {
            gVitalSignsInitialized = 1;
            System_printf("Debug: Vital Signs module initialized\n");
        }
    }
#endif


/*****************************************************************************
 * PATCH 4: Add VS processing in MmwDemo_DPC_ObjectDetection_dpmTask()
 *
 * Add inside the DPM_execute result handling block, after MmwDemo_copyResultToHSRAM
 * but before DPM_sendResult (around line 690)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
    /* Process vital signs if enabled */
    if (gVitalSignsInitialized && gVitalSignsConfig.enabled)
    {
        cmplx16ImRe_t *radarCubeData;
        uint16_t targetRangeBin;
        int32_t vsRetVal;

        /* Get radar cube data pointer */
        radarCubeData = (cmplx16ImRe_t *)result->radarCube.data;

        /* Determine target range bin */
        if (gVitalSignsConfig.trackerIntegration)
        {
            /* TODO: Get range bin from tracker if available */
            /* For now, use configured start bin */
            targetRangeBin = gVitalSignsConfig.rangeBinStart;
        }
        else
        {
            targetRangeBin = gVitalSignsConfig.rangeBinStart;
        }

        /* Process frame for vital signs */
        vsRetVal = VitalSigns_processFrame(
            radarCubeData,
            result->radarCube.dataProperty.numRangeBins,
            result->radarCube.dataProperty.numDopplerChirps,
            (uint8_t)(result->radarCube.dataProperty.numTxAntennas *
                      result->radarCube.dataProperty.numRxAntennas),
            targetRangeBin
        );

        if (vsRetVal == VITALSIGNS_EOK)
        {
            /* Copy VS output to HSRAM for MSS to read */
            VitalSigns_getOutput(&gHSRAM.vitalSignsOutput);
        }
    }
#endif


/*****************************************************************************
 * PATCH 5: Handle VS config update from MSS
 *
 * Add a new IOCTL case in MmwDemo_DPC_ObjectDetection_reportFxn()
 * for handling VS configuration updates (around line 370)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
/* Define VS config IOCTL command - add to header file */
#define MMWDEMO_IOCTL_VS_CONFIG    0x100

/* In the DPM_Report_IOCTL case, add handling: */
case DPM_Report_IOCTL:
{
    if (arg0 == MMWDEMO_IOCTL_VS_CONFIG)
    {
        /* VS configuration update received */
        VitalSigns_Config *pVsCfg = (VitalSigns_Config *)arg1;
        if (pVsCfg != NULL)
        {
            memcpy(&gVitalSignsConfig, pVsCfg, sizeof(VitalSigns_Config));

            /* Update module configuration */
            if (gVitalSignsInitialized)
            {
                VitalSigns_updateConfig(&gVitalSignsConfig);
            }
        }
    }
    break;
}
#endif


/*****************************************************************************
 * PATCH 6: Add VS cleanup in sensor stop
 *
 * Add in MmwDemo_sensorStopEpilog() (around line 325)
 *****************************************************************************/

#ifdef VITAL_SIGNS_ENABLED
    /* Reset vital signs processing on sensor stop */
    if (gVitalSignsInitialized)
    {
        VitalSigns_reset();
    }
#endif


/*****************************************************************************
 * PATCH 7: Alternative - Simpler integration using inter-frame processing
 *
 * If the above DPM task integration is complex, an alternative is to
 * process VS in the inter-frame callback. Add to
 * MmwDemo_DPC_ObjectDetection_processInterFrameBeginCallBackFxn()
 *****************************************************************************/

static void MmwDemo_DPC_ObjectDetection_processInterFrameBeginCallBackFxn(uint8_t subFrameIndx)
{
    Load_update();
    gMmwDssMCB.dataPathObj.subFrameStats[subFrameIndx].interFrameCPULoad = Load_getCPULoad();

#ifdef VITAL_SIGNS_ENABLED
    /* Note: This is called at the START of inter-frame processing,
     * before the radar cube from the previous frame is available.
     * For VS processing, we need to use the DPM task approach instead,
     * or add a new callback that fires after DPC processing completes.
     */
#endif
}


/*****************************************************************************
 * MEMORY CONSIDERATIONS
 *
 * The VS module uses ~52KB of L2 RAM. Ensure the linker command file
 * allocates this space in the .dss_l2 section.
 *
 * In mmw_dss_linker.cmd, verify L2SRAM has sufficient space:
 *
 *     L2SRAM  : origin = 0x00800000, length = 0x00020000  // 128KB
 *
 * And add the section mapping:
 *
 *     SECTIONS
 *     {
 *         ...
 *         .dss_l2 > L2SRAM
 *         ...
 *     }
 *
 *****************************************************************************/
