/**
 * @file phase_extract.c
 * @brief Phase extraction implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "phase_extract.h"
#include <string.h>

/*******************************************************************************
 * Private Constants
 ******************************************************************************/

/* atan2 lookup table for fast approximation */
/* Index by (|y|/|x|) * 64, value is angle * 10430 / (pi/4) */
static const int16_t atan_lut[65] = {
        0,   163,   326,   488,   651,   813,   975,  1135,
     1295,  1454,  1612,  1768,  1923,  2076,  2228,  2378,
     2526,  2672,  2815,  2957,  3096,  3233,  3368,  3500,
     3630,  3757,  3882,  4004,  4123,  4240,  4354,  4466,
     4575,  4682,  4786,  4888,  4987,  5083,  5178,  5270,
     5360,  5448,  5533,  5616,  5698,  5777,  5854,  5929,
     6003,  6074,  6144,  6212,  6278,  6343,  6406,  6467,
     6527,  6585,  6642,  6698,  6752,  6805,  6856,  6907,
     6956  /* pi/4 * 10430 / (pi/4) = 10430... but scaled */
};

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

int16_t Chirp_Phase_atan2(int16_t y, int16_t x)
{
    int32_t absX, absY;
    int32_t ratio;
    int16_t angle;
    uint8_t index;

    absX = (x >= 0) ? x : -x;
    absY = (y >= 0) ? y : -y;

    /* Handle edge cases */
    if (absX == 0 && absY == 0)
    {
        return 0;
    }

    if (absX == 0)
    {
        /* On Y axis */
        return (y > 0) ? 16384 : -16384;  /* pi/2 or -pi/2 */
    }

    if (absY == 0)
    {
        /* On X axis */
        return (x > 0) ? 0 : -32768;  /* 0 or pi/-pi */
    }

    /* Calculate ratio and lookup */
    if (absY <= absX)
    {
        /* |y/x| <= 1, use direct lookup */
        ratio = (absY << 6) / absX;  /* 0 to 64 */
        if (ratio > 64) ratio = 64;
        index = (uint8_t)ratio;
        angle = atan_lut[index];
    }
    else
    {
        /* |y/x| > 1, use pi/2 - atan(x/y) */
        ratio = (absX << 6) / absY;
        if (ratio > 64) ratio = 64;
        index = (uint8_t)ratio;
        angle = 16384 - atan_lut[index];  /* pi/2 - angle */
    }

    /* Map to correct quadrant */
    if (x < 0)
    {
        if (y >= 0)
        {
            angle = 32768 - angle;  /* Q2: pi - angle */
        }
        else
        {
            angle = -32768 + angle;  /* Q3: -pi + angle */
        }
    }
    else
    {
        if (y < 0)
        {
            angle = -angle;  /* Q4: -angle */
        }
        /* Q1: angle stays positive */
    }

    return angle;
}

uint16_t Chirp_Phase_sqrt(uint32_t val)
{
    uint32_t result = 0;
    uint32_t bit = 1UL << 30;

    /* Find highest bit */
    while (bit > val)
    {
        bit >>= 2;
    }

    /* Binary search for sqrt */
    while (bit != 0)
    {
        if (val >= result + bit)
        {
            val -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }

    return (uint16_t)result;
}

void Chirp_Phase_extract(int16_t real, int16_t imag, int16_t *phase, uint16_t *magnitude)
{
    int32_t magSquared;

    if (phase != NULL)
    {
        *phase = Chirp_Phase_atan2(imag, real);
    }

    if (magnitude != NULL)
    {
        /* Magnitude = sqrt(real^2 + imag^2) */
        magSquared = (int32_t)real * real + (int32_t)imag * imag;
        *magnitude = Chirp_Phase_sqrt((uint32_t)magSquared);
    }
}

int32_t Chirp_Phase_extractBins(const int16_t *radarData,
                                 const uint16_t *binIndices,
                                 uint8_t numBins,
                                 uint16_t centerBin,
                                 uint32_t timestamp_us,
                                 Chirp_PhaseOutput *output)
{
    uint8_t i;
    const int16_t *binData;
    int16_t imag, real;

    if (radarData == NULL || binIndices == NULL || output == NULL)
    {
        return -1;
    }

    if (numBins == 0 || numBins > CHIRP_PHASE_MAX_BINS)
    {
        return -1;
    }

    /* Fill header */
    output->numBins = numBins;
    output->centerBin = centerBin;
    output->timestamp_us = timestamp_us;

    /* Extract phase for each bin */
    for (i = 0; i < numBins; i++)
    {
        /* SDK uses cmplx16ImRe_t: imag first, then real */
        binData = &radarData[binIndices[i] * 2];
        imag = binData[0];
        real = binData[1];

        output->bins[i].binIndex = binIndices[i];

        Chirp_Phase_extract(real, imag,
                           &output->bins[i].phase,
                           &output->bins[i].magnitude);

        /* Mark as valid */
        output->bins[i].flags = CHIRP_PHASE_FLAG_VALID;
    }

    return 0;
}

float Chirp_Phase_toRadians(int16_t phaseFixed)
{
    /* phase_rad = phaseFixed / 10430 * pi
     * But we use 32768/pi scaling, so:
     * phase_rad = phaseFixed * pi / 32768
     */
    return (float)phaseFixed * 3.14159265f / 32768.0f;
}

int16_t Chirp_Phase_fromRadians(float phaseRad)
{
    return (int16_t)(phaseRad * 32768.0f / 3.14159265f);
}
