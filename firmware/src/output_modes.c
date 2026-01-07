/**
 * @file output_modes.c
 * @brief Output mode management implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "output_modes.h"

#include <string.h>

/*******************************************************************************
 * Private Data
 ******************************************************************************/

static const char *outputModeNames[CHIRP_OUTPUT_MODE_COUNT] = {"RAW_IQ", "RANGE_FFT", "TARGET_IQ", "PHASE", "PRESENCE"};

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void Chirp_OutputMode_init(Chirp_OutputConfig *config)
{
    if (config == NULL)
    {
        return;
    }

    /* Default to RANGE_FFT mode (backward compatible with TLV 0x0500) */
    config->mode = CHIRP_OUTPUT_MODE_RANGE_FFT;
    config->enableMotionOutput = 0;
    config->enableTargetInfo = 0;
    config->reserved = 0;
}

int32_t Chirp_OutputMode_set(Chirp_OutputConfig *config, Chirp_OutputMode mode)
{
    if (config == NULL)
    {
        return -1;
    }

    if (mode >= CHIRP_OUTPUT_MODE_COUNT)
    {
        return -1;
    }

    config->mode = mode;
    return 0;
}

Chirp_OutputMode Chirp_OutputMode_get(const Chirp_OutputConfig *config)
{
    if (config == NULL)
    {
        return CHIRP_OUTPUT_MODE_RANGE_FFT;
    }

    return config->mode;
}

const char *Chirp_OutputMode_getName(Chirp_OutputMode mode)
{
    if (mode >= CHIRP_OUTPUT_MODE_COUNT)
    {
        return "UNKNOWN";
    }

    return outputModeNames[mode];
}

int32_t Chirp_OutputMode_parse(const char *str, Chirp_OutputMode *mode)
{
    uint32_t i;

    if (str == NULL || mode == NULL)
    {
        return -1;
    }

    /* Check for numeric input */
    if (str[0] >= '0' && str[0] <= '9')
    {
        uint32_t val = (uint32_t)(str[0] - '0');
        if (val < CHIRP_OUTPUT_MODE_COUNT)
        {
            *mode = (Chirp_OutputMode)val;
            return 0;
        }
        return -1;
    }

    /* Check for string input */
    for (i = 0; i < CHIRP_OUTPUT_MODE_COUNT; i++)
    {
        if (strcmp(str, outputModeNames[i]) == 0)
        {
            *mode = (Chirp_OutputMode)i;
            return 0;
        }
    }

    return -1;
}
