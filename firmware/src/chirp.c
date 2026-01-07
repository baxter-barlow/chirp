/**
 * @file chirp.c
 * @brief Main chirp firmware implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "chirp.h"

#include <string.h>

/*******************************************************************************
 * Global State
 ******************************************************************************/

Chirp_State gChirpState;

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void Chirp_init(void)
{
    /* Clear entire state */
    memset(&gChirpState, 0, sizeof(Chirp_State));

    /* Initialize output mode */
    Chirp_OutputMode_init(&gChirpState.outputConfig);

    /* Initialize target selection */
    Chirp_TargetSelect_init(&gChirpState.targetConfig, &gChirpState.targetState);

    /* Initialize motion detection */
    Chirp_Motion_init(&gChirpState.motionConfig, &gChirpState.motionState);

    /* Initialize power management */
    Chirp_Power_init(&gChirpState.powerConfig, &gChirpState.powerState);

    /* Mark as initialized */
    gChirpState.initialized = 1;
}

void Chirp_configure(float rangeResolution, uint16_t numRangeBins)
{
    gChirpState.rangeResolution = rangeResolution;
    gChirpState.numRangeBins = numRangeBins;
}

int32_t Chirp_processFrame(const int16_t *radarCubeData, uint16_t numRangeBins, uint32_t timestamp_us)
{
    uint16_t i;
    uint16_t magnitude[CHIRP_MOTION_MAX_BINS];
    int16_t imag, real;
    Chirp_OutputMode mode;

    if (!gChirpState.initialized)
    {
        return -1;
    }

    if (radarCubeData == NULL)
    {
        return -1;
    }

    /* Get current output mode */
    mode = Chirp_OutputMode_get(&gChirpState.outputConfig);

    /* Compute magnitude for each range bin (for target selection & motion) */
    if (mode >= CHIRP_OUTPUT_MODE_TARGET_IQ)
    {
        uint16_t binsToProcess = (numRangeBins < CHIRP_MOTION_MAX_BINS) ? numRangeBins : CHIRP_MOTION_MAX_BINS;

        for (i = 0; i < binsToProcess; i++)
        {
            /* cmplx16ImRe_t: imag first, then real */
            imag = radarCubeData[i * 2];
            real = radarCubeData[i * 2 + 1];

            /* Compute magnitude using our integer sqrt */
            magnitude[i] = Chirp_Phase_sqrt((uint32_t)real * real + (uint32_t)imag * imag);
        }

        /* Run target selection */
        Chirp_TargetSelect_process(&gChirpState.targetConfig, &gChirpState.targetState, magnitude, binsToProcess,
                                   gChirpState.rangeResolution, &gChirpState.targetResult);

        /* Run motion detection */
        Chirp_Motion_process(&gChirpState.motionConfig, &gChirpState.motionState, magnitude, binsToProcess,
                             &gChirpState.motionResult);

        /* Extract phase for selected bins */
        if (gChirpState.targetResult.valid && gChirpState.targetResult.numTrackBinsUsed > 0)
        {
            Chirp_Phase_extractBins(radarCubeData, gChirpState.targetResult.trackBins,
                                    gChirpState.targetResult.numTrackBinsUsed, gChirpState.targetResult.primaryBin,
                                    timestamp_us, &gChirpState.phaseOutput);

            /* Mark motion flag in phase output */
            if (gChirpState.motionResult.motionDetected)
            {
                for (i = 0; i < gChirpState.phaseOutput.numBins; i++)
                {
                    gChirpState.phaseOutput.bins[i].flags |= CHIRP_PHASE_FLAG_MOTION;
                }
            }
        }
    }

    return 0;
}

uint32_t Chirp_getNumOutputTLVs(void)
{
    Chirp_OutputMode mode = Chirp_OutputMode_get(&gChirpState.outputConfig);
    uint32_t count = 0;

    switch (mode)
    {
        case CHIRP_OUTPUT_MODE_RAW_IQ:
            /* Full radar cube - handled by existing TLV 0x0500 */
            count = 1;
            break;

        case CHIRP_OUTPUT_MODE_RANGE_FFT:
            /* Range profile - handled by existing SDK TLVs */
            count = 0;
            break;

        case CHIRP_OUTPUT_MODE_TARGET_IQ:
            /* TLV 0x0510: Target I/Q */
            count = 1;
            break;

        case CHIRP_OUTPUT_MODE_PHASE:
            /* TLV 0x0520: Phase output */
            count = 1;
            break;

        case CHIRP_OUTPUT_MODE_PRESENCE:
            /* TLV 0x0540: Presence */
            count = 1;
            break;

        default:
            break;
    }

    /* Add motion TLV if enabled */
    if (gChirpState.outputConfig.enableMotionOutput)
    {
        count++;
    }

    /* Add target info TLV if enabled */
    if (gChirpState.outputConfig.enableTargetInfo)
    {
        count++;
    }

    return count;
}

uint32_t Chirp_getOutputSize(void)
{
    Chirp_OutputMode mode = Chirp_OutputMode_get(&gChirpState.outputConfig);
    uint32_t size = 0;

    switch (mode)
    {
        case CHIRP_OUTPUT_MODE_TARGET_IQ:
            /* Header + bins */
            size = sizeof(Chirp_output_targetIQ_header) +
                   gChirpState.targetResult.numTrackBinsUsed * sizeof(Chirp_output_targetIQ_bin);
            break;

        case CHIRP_OUTPUT_MODE_PHASE:
            /* Header + bins */
            size = sizeof(Chirp_output_phase_header) + gChirpState.phaseOutput.numBins * sizeof(Chirp_output_phase_bin);
            break;

        case CHIRP_OUTPUT_MODE_PRESENCE:
            size = sizeof(Chirp_output_presence);
            break;

        default:
            break;
    }

    /* Add motion output size if enabled */
    if (gChirpState.outputConfig.enableMotionOutput)
    {
        size += sizeof(Chirp_output_motion);
    }

    /* Add target info size if enabled */
    if (gChirpState.outputConfig.enableTargetInfo)
    {
        size += sizeof(Chirp_output_targetInfo);
    }

    return size;
}

uint8_t Chirp_shouldOutputTLV(uint32_t tlvType)
{
    Chirp_OutputMode mode = Chirp_OutputMode_get(&gChirpState.outputConfig);

    switch (tlvType)
    {
        case MMWDEMO_OUTPUT_MSG_COMPLEX_RANGE_FFT:
            return (mode == CHIRP_OUTPUT_MODE_RAW_IQ) ? 1 : 0;

        case MMWDEMO_OUTPUT_MSG_TARGET_IQ:
            return (mode == CHIRP_OUTPUT_MODE_TARGET_IQ) ? 1 : 0;

        case MMWDEMO_OUTPUT_MSG_PHASE_OUTPUT:
            return (mode == CHIRP_OUTPUT_MODE_PHASE) ? 1 : 0;

        case MMWDEMO_OUTPUT_MSG_PRESENCE:
            return (mode == CHIRP_OUTPUT_MODE_PRESENCE) ? 1 : 0;

        case MMWDEMO_OUTPUT_MSG_MOTION_STATUS:
            return gChirpState.outputConfig.enableMotionOutput;

        case MMWDEMO_OUTPUT_MSG_TARGET_INFO:
            return gChirpState.outputConfig.enableTargetInfo;

        default:
            return 0;
    }
}
