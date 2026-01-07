/**
 * @file target_select.c
 * @brief Target auto-selection algorithm implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "target_select.h"

#include <string.h>

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

/**
 * @brief Find local maximum (peak) in range profile
 */
static int32_t findPeak(const uint16_t *magnitude, uint16_t startBin, uint16_t endBin, uint16_t *peakBin,
                        uint16_t *peakValue)
{
    uint16_t i;
    uint16_t maxVal = 0;
    uint16_t maxBin = startBin;

    if (startBin >= endBin)
    {
        return -1;
    }

    for (i = startBin; i < endBin; i++)
    {
        if (magnitude[i] > maxVal)
        {
            maxVal = magnitude[i];
            maxBin = i;
        }
    }

    *peakBin = maxBin;
    *peakValue = maxVal;

    return 0;
}

/**
 * @brief Calculate SNR in dB (approximate, using integer math)
 * Signal is peak, noise is average of surrounding bins
 */
static uint8_t calculateSNR(const uint16_t *magnitude, uint16_t numBins, uint16_t peakBin, uint16_t peakValue)
{
    uint32_t noiseSum = 0;
    uint32_t noiseCount = 0;
    uint16_t i;
    uint32_t noiseAvg;
    uint32_t snrLinear;

    /* Average noise from bins away from peak */
    for (i = 0; i < numBins; i++)
    {
        /* Skip bins near peak (within 5 bins) */
        if (i < peakBin - 5 || i > peakBin + 5)
        {
            noiseSum += magnitude[i];
            noiseCount++;
        }
    }

    if (noiseCount == 0 || noiseSum == 0)
    {
        return 40; /* Max SNR if no noise */
    }

    noiseAvg = noiseSum / noiseCount;

    if (noiseAvg == 0)
    {
        return 40;
    }

    /* SNR linear = peak / noise */
    snrLinear = (uint32_t)peakValue / noiseAvg;

    /* Approximate dB: 10 log10(x) ≈ 3.3 * log2(x) */
    /* Simple approximation: count bits for log2 */
    if (snrLinear >= 1000)
        return 30;
    if (snrLinear >= 316)
        return 25;
    if (snrLinear >= 100)
        return 20;
    if (snrLinear >= 31)
        return 15;
    if (snrLinear >= 10)
        return 10;
    if (snrLinear >= 3)
        return 5;

    return 0;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void Chirp_TargetSelect_init(Chirp_TargetConfig *config, Chirp_TargetState *state)
{
    if (config != NULL)
    {
        config->minRange_m = CHIRP_TARGET_MIN_RANGE_DEFAULT;
        config->maxRange_m = CHIRP_TARGET_MAX_RANGE_DEFAULT;
        config->minSNR_dB = CHIRP_TARGET_MIN_SNR_DEFAULT;
        config->numTrackBins = 3;
        config->hysteresisBins = CHIRP_TARGET_HYSTERESIS_DEFAULT;
        config->reserved = 0;
    }

    if (state != NULL)
    {
        state->prevPrimaryBin = 0;
        state->framesSinceChange = 0;
        state->locked = 0;
        memset(state->reserved, 0, sizeof(state->reserved));
    }
}

int32_t Chirp_TargetSelect_configure(Chirp_TargetConfig *config, float minRange, float maxRange, uint8_t minSNR,
                                     uint8_t numBins)
{
    if (config == NULL)
    {
        return -1;
    }

    if (minRange < 0.0f || maxRange <= minRange)
    {
        return -1;
    }

    if (numBins == 0 || numBins > CHIRP_TARGET_MAX_TRACK_BINS)
    {
        return -1;
    }

    config->minRange_m = minRange;
    config->maxRange_m = maxRange;
    config->minSNR_dB = minSNR;
    config->numTrackBins = numBins;

    return 0;
}

int32_t Chirp_TargetSelect_process(const Chirp_TargetConfig *config, Chirp_TargetState *state,
                                   const uint16_t *rangeMagnitude, uint16_t numBins, float rangeResolution,
                                   Chirp_TargetResult *result)
{
    uint16_t minBin, maxBin;
    uint16_t peakBin, peakValue;
    uint8_t snr;
    int16_t i, halfTrack;
    int16_t startTrack, endTrack;

    if (config == NULL || state == NULL || rangeMagnitude == NULL || result == NULL)
    {
        return -1;
    }

    if (numBins == 0 || rangeResolution <= 0.0f)
    {
        return -1;
    }

    /* Initialize result */
    memset(result, 0, sizeof(Chirp_TargetResult));

    /* Convert range bounds to bins */
    minBin = Chirp_TargetSelect_rangeToBin(config->minRange_m, rangeResolution);
    maxBin = Chirp_TargetSelect_rangeToBin(config->maxRange_m, rangeResolution);

    /* Clamp to valid range */
    if (minBin >= numBins)
        minBin = 0;
    if (maxBin >= numBins)
        maxBin = numBins - 1;
    if (minBin >= maxBin)
    {
        result->valid = 0;
        return 0;
    }

    /* Find peak in search range */
    if (findPeak(rangeMagnitude, minBin, maxBin + 1, &peakBin, &peakValue) != 0)
    {
        result->valid = 0;
        return 0;
    }

    /* Check SNR threshold */
    snr = calculateSNR(rangeMagnitude, numBins, peakBin, peakValue);
    if (snr < config->minSNR_dB)
    {
        result->valid = 0;
        result->confidence = snr * 100 / config->minSNR_dB;
        return 0;
    }

    /* Apply hysteresis - don't switch target if within hysteresis range */
    if (state->locked && peakBin >= state->prevPrimaryBin - config->hysteresisBins &&
        peakBin <= state->prevPrimaryBin + config->hysteresisBins)
    {
        /* Keep previous target if new peak is close */
        if (rangeMagnitude[state->prevPrimaryBin] > peakValue / 2)
        {
            peakBin = state->prevPrimaryBin;
            peakValue = rangeMagnitude[peakBin];
        }
    }

    /* Update state */
    if (peakBin != state->prevPrimaryBin)
    {
        state->framesSinceChange = 0;
    }
    else
    {
        if (state->framesSinceChange < 65535)
        {
            state->framesSinceChange++;
        }
    }

    state->prevPrimaryBin = peakBin;
    state->locked = 1;

    /* Fill result */
    result->primaryBin = peakBin;
    result->primaryMagnitude = peakValue;
    result->primaryRange_Q8 = (uint16_t)(Chirp_TargetSelect_binToRange(peakBin, rangeResolution) * 256.0f);
    result->confidence = (snr > 40) ? 100 : (snr * 100 / 40);
    result->numTargets = 1;
    result->valid = 1;

    /* Calculate track bins centered on primary */
    halfTrack = (int16_t)(config->numTrackBins / 2);
    startTrack = (int16_t)peakBin - halfTrack;
    endTrack = startTrack + (int16_t)config->numTrackBins;

    /* Clamp to valid range */
    if (startTrack < 0)
        startTrack = 0;
    if (endTrack > (int16_t)numBins)
        endTrack = (int16_t)numBins;

    result->numTrackBinsUsed = 0;
    for (i = startTrack; i < endTrack && result->numTrackBinsUsed < CHIRP_TARGET_MAX_TRACK_BINS; i++)
    {
        result->trackBins[result->numTrackBinsUsed++] = (uint16_t)i;
    }

    return 0;
}

float Chirp_TargetSelect_binToRange(uint16_t bin, float rangeResolution)
{
    return (float)bin * rangeResolution;
}

uint16_t Chirp_TargetSelect_rangeToBin(float range, float rangeResolution)
{
    if (rangeResolution <= 0.0f)
    {
        return 0;
    }

    return (uint16_t)(range / rangeResolution);
}
