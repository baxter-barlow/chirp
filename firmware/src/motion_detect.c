/**
 * @file motion_detect.c
 * @brief Motion detection implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "motion_detect.h"

#include <string.h>

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void Chirp_Motion_init(Chirp_MotionConfig *config, Chirp_MotionState *state)
{
    if (config != NULL)
    {
        config->enabled = 1;
        config->reserved = 0;
        config->threshold = CHIRP_MOTION_THRESHOLD_DEFAULT;
        config->minBin = 0;
        config->maxBin = CHIRP_MOTION_MAX_BINS - 1;
    }

    if (state != NULL)
    {
        Chirp_Motion_reset(state);
    }
}

int32_t Chirp_Motion_configure(Chirp_MotionConfig *config, uint8_t enabled, uint16_t threshold, uint16_t minBin,
                               uint16_t maxBin)
{
    if (config == NULL)
    {
        return -1;
    }

    if (minBin >= maxBin)
    {
        return -1;
    }

    config->enabled = enabled;
    config->threshold = threshold;
    config->minBin = minBin;
    config->maxBin = maxBin;

    return 0;
}

int32_t Chirp_Motion_process(const Chirp_MotionConfig *config, Chirp_MotionState *state, const uint16_t *magnitude,
                             uint16_t numBins, Chirp_MotionResult *result)
{
    uint16_t i;
    uint16_t startBin, endBin;
    int32_t delta;
    uint16_t absDelta;
    uint32_t motionSum = 0;
    uint16_t motionBinCount = 0;
    uint16_t peakDelta = 0;
    uint16_t peakBin = 0;

    if (config == NULL || state == NULL || magnitude == NULL || result == NULL)
    {
        return -1;
    }

    /* Initialize result */
    memset(result, 0, sizeof(Chirp_MotionResult));

    /* If disabled, just return no motion */
    if (!config->enabled)
    {
        return 0;
    }

    /* Determine bin range to process */
    startBin = config->minBin;
    endBin = config->maxBin;

    if (endBin >= numBins)
    {
        endBin = numBins - 1;
    }

    if (endBin >= CHIRP_MOTION_MAX_BINS)
    {
        endBin = CHIRP_MOTION_MAX_BINS - 1;
    }

    /* On first frame, just store magnitudes */
    if (state->firstFrame)
    {
        for (i = startBin; i <= endBin; i++)
        {
            state->prevMagnitude[i] = magnitude[i];
        }
        state->numBins = numBins;
        state->firstFrame = 0;
        state->frameCount = 1;
        return 0;
    }

    /* Compare current frame to previous */
    for (i = startBin; i <= endBin; i++)
    {
        delta = (int32_t)magnitude[i] - (int32_t)state->prevMagnitude[i];
        absDelta = (delta >= 0) ? (uint16_t)delta : (uint16_t)(-delta);

        if (absDelta > config->threshold)
        {
            motionBinCount++;
            motionSum += absDelta;

            if (absDelta > peakDelta)
            {
                peakDelta = absDelta;
                peakBin = i;
            }
        }

        /* Update previous magnitude */
        state->prevMagnitude[i] = magnitude[i];
    }

    /* Fill result */
    result->motionDetected = (motionBinCount > 0) ? 1 : 0;
    result->motionBinCount = motionBinCount;
    result->peakMotionBin = peakBin;
    result->peakMotionDelta = peakDelta;

    /* Calculate normalized motion level (0-255) */
    if (motionBinCount > 0)
    {
        uint32_t avgMotion = motionSum / motionBinCount;
        /* Scale to 0-255 based on threshold */
        uint32_t level = (avgMotion * 255) / (config->threshold * 4);
        if (level > 255)
            level = 255;
        result->motionLevel = (uint8_t)level;
    }

    state->frameCount++;

    return 0;
}

void Chirp_Motion_reset(Chirp_MotionState *state)
{
    if (state != NULL)
    {
        memset(state->prevMagnitude, 0, sizeof(state->prevMagnitude));
        state->numBins = 0;
        state->frameCount = 0;
        state->firstFrame = 1;
        memset(state->reserved, 0, sizeof(state->reserved));
    }
}
