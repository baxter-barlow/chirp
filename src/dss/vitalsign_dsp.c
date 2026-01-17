/**
 *   @file  vitalsign_dsp.c
 *
 *   @brief
 *      DSS Vital Signs Processing Module Implementation
 *
 *   This module implements vital signs (heart rate and breathing rate)
 *   detection using radar phase data. Adapted from TI IWRL6432 for
 *   IWR6843AOPEVM dual-core architecture (C674x DSP).
 *
 *   Algorithm Pipeline:
 *   1. Extract phase data from radar cube for selected range/antenna bins
 *   2. Perform 2D angle FFT (azimuth x elevation) to track strongest angle
 *   3. Accumulate 128 frames of phase data
 *   4. Apply phase unwrapping to get continuous displacement signal
 *   5. Perform 512-pt spectrum FFT on phase data
 *   6. Detect breathing peak (3-50 bins) and heart rate peak (68-128 bins)
 *   7. Use harmonic product spectrum for robust heart rate detection
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
#include <string.h>
#include <math.h>

#include "vitalsign_dsp.h"
#include "vitalsign_common.h"
#include "cplx_types.h"

/*
 * Math library includes - atan2sp_i inline function
 * USE_LOCAL_MATHLIB: Use project-integrated headers (src/dss/mathlib/)
 * Otherwise: Use existing local atan2sp_i.h or vendor mathlib
 */
#ifdef USE_LOCAL_MATHLIB
#include "mathlib/mathlib.h"
#else
#include "atan2sp_i.h"
#endif

/*
 * DSP library includes - FFT functions
 * USE_LOCAL_DSPLIB: Use project-integrated source (src/dss/dsplib/)
 * Otherwise: Use vendor DSPlib (pre-built assembly-optimized library)
 */
#ifdef USE_LOCAL_DSPLIB
#include "dsplib/dsplib.h"
#else
#include <ti/dsplib/src/DSPF_sp_fftSPxSP/DSPF_sp_fftSPxSP.h>
#endif

/**************************************************************************
 *************************** Local Definitions ****************************
 **************************************************************************/

/** @brief Size of mean buffer for DC offset removal (ping-pong) */
#define VS_MEAN_BUF_SIZE    (VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL * 2)

/**************************************************************************
 *************************** Global Variables *****************************
 **************************************************************************/

/* Configuration */
static VitalSigns_Config gVsConfig;

/* Processing state */
static VitalSigns_State gVsState;

/* Output */
static VitalSigns_Output gVsOutput;

/* Antenna geometry for IWR6843 AOP (3TX x 4RX = 12 virtual antennas) */
static VitalSigns_AntennaGeometry gVsAntenna;

/**************************************************************************
 *************************** L2 RAM Buffers *******************************
 **************************************************************************/

/* Per-frame data extraction buffer: range bins x virtual antennas */
#pragma DATA_SECTION(gVsDataPerFrame, ".dss_l2")
#pragma DATA_ALIGN(gVsDataPerFrame, 8)
static cplxf_t gVsDataPerFrame[VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL];

/* Angle FFT output buffer: accumulated over VS_TOTAL_FRAME frames */
#pragma DATA_SECTION(gVsAngleFftBuf, ".dss_l2")
#pragma DATA_ALIGN(gVsAngleFftBuf, 8)
static cplxf_t gVsAngleFftBuf[VS_TOTAL_FRAME * VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN];

/* DC offset mean buffer (ping-pong for running average) */
#pragma DATA_SECTION(gVsMeanBuf, ".dss_l2")
#pragma DATA_ALIGN(gVsMeanBuf, 8)
static cplxf_t gVsMeanBuf[VS_MEAN_BUF_SIZE];

/* Spectrum computation buffer */
#pragma DATA_SECTION(gVsSpectrumBuf, ".dss_l2")
#pragma DATA_ALIGN(gVsSpectrumBuf, 8)
static cplxf_t gVsSpectrumBuf[VS_PHASE_FFT_SIZE];

/* Twiddle factors for angle FFT (16-pt) */
#pragma DATA_SECTION(gVsTwiddleAngle, ".dss_l2")
#pragma DATA_ALIGN(gVsTwiddleAngle, 8)
static float gVsTwiddleAngle[2 * VS_NUM_ANGLE_FFT];

/* Twiddle factors for spectrum FFT (512-pt) */
#pragma DATA_SECTION(gVsTwiddleSpectrum, ".dss_l2")
#pragma DATA_ALIGN(gVsTwiddleSpectrum, 8)
static float gVsTwiddleSpectrum[2 * VS_PHASE_FFT_SIZE];

/* Angle FFT magnitude sum for peak detection */
#pragma DATA_SECTION(gVsAngleFftMagSum, ".dss_l2")
#pragma DATA_ALIGN(gVsAngleFftMagSum, 8)
static float gVsAngleFftMagSum[VS_NUM_ANGLE_FFT * VS_NUM_ANGLE_FFT];

/* Bit-reversed index table for FFT (up to 64 points) */
static const unsigned char gVsBrevFft[64] = {
    0x0,  0x20, 0x10, 0x30, 0x8,  0x28, 0x18, 0x38,
    0x4,  0x24, 0x14, 0x34, 0xc,  0x2c, 0x1c, 0x3c,
    0x2,  0x22, 0x12, 0x32, 0xa,  0x2a, 0x1a, 0x3a,
    0x6,  0x26, 0x16, 0x36, 0xe,  0x2e, 0x1e, 0x3e,
    0x1,  0x21, 0x11, 0x31, 0x9,  0x29, 0x19, 0x39,
    0x5,  0x25, 0x15, 0x35, 0xd,  0x2d, 0x1d, 0x3d,
    0x3,  0x23, 0x13, 0x33, 0xb,  0x2b, 0x1b, 0x3b,
    0x7,  0x27, 0x17, 0x37, 0xf,  0x2f, 0x1f, 0x3f
};

/**************************************************************************
 *************************** Local Function Prototypes ********************
 **************************************************************************/

static int32_t VitalSigns_genTwiddle(float *w, int32_t n);
static void VitalSigns_computeMagnitudeSquared(cplxf_t *inpBuff, float *magSqrdBuff, uint32_t numSamples);
static float VitalSigns_computePhaseUnwrap(float phase, float phasePrev, float *diffPhaseCorrectionCum);
static float VitalSigns_computeDeviation(float *a, int32_t n);
static void VitalSigns_extractRadarData(cmplx16ImRe_t *radarCube, uint16_t rangeBin, uint16_t numRangeBins, uint8_t numVirtualAnt);
static void VitalSigns_runPreProcess(uint32_t vsDataCount);
static void VitalSigns_computeVitalSignProcessing(uint16_t indicateNoTarget);
static uint32_t VitalSigns_runCopyTranspose64b(uint64_t *src, uint64_t *dest, uint32_t size, int32_t offset, uint32_t stride, uint32_t pairs);
static void VitalSigns_initAntennaGeometry(void);

/**************************************************************************
 *************************** Function Implementations *********************
 **************************************************************************/

/**
 *  @brief Generate twiddle factors for FFT
 */
static int32_t VitalSigns_genTwiddle(float *w, int32_t n)
{
    int32_t i, j, k;

    for (j = 1, k = 0; j < n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            w[k + 5] = sinf(6.0f * VS_PI * i / n);
            w[k + 4] = cosf(6.0f * VS_PI * i / n);
            w[k + 3] = sinf(4.0f * VS_PI * i / n);
            w[k + 2] = cosf(4.0f * VS_PI * i / n);
            w[k + 1] = sinf(2.0f * VS_PI * i / n);
            w[k + 0] = cosf(2.0f * VS_PI * i / n);
            k += 6;
        }
    }
    return k;
}

/**
 *  @brief Compute magnitude squared of complex array
 */
static void VitalSigns_computeMagnitudeSquared(cplxf_t *inpBuff, float *magSqrdBuff, uint32_t numSamples)
{
    uint32_t i;
    for (i = 0; i < numSamples; i++)
    {
        magSqrdBuff[i] = inpBuff[i].real * inpBuff[i].real +
                         inpBuff[i].imag * inpBuff[i].imag;
    }
}

/**
 *  @brief Phase unwrapping to handle 2*PI discontinuities
 */
static float VitalSigns_computePhaseUnwrap(float phase, float phasePrev, float *diffPhaseCorrectionCum)
{
    float modFactorF;
    float diffPhase;
    float diffPhaseMod;
    float diffPhaseCorrection;
    float phaseOut;

    /* Incremental phase variation */
    diffPhase = phase - phasePrev;

    if (diffPhase > VS_PI)
        modFactorF = 1.0f;
    else if (diffPhase < -VS_PI)
        modFactorF = -1.0f;
    else
        modFactorF = 0.0f;

    diffPhaseMod = diffPhase - modFactorF * 2.0f * VS_PI;

    /* Preserve variation sign for +pi vs. -pi */
    if ((diffPhaseMod == -VS_PI) && (diffPhase > 0))
        diffPhaseMod = VS_PI;

    /* Incremental phase correction */
    diffPhaseCorrection = diffPhaseMod - diffPhase;

    /* Ignore correction when incremental variation is smaller than cutoff */
    if (((diffPhaseCorrection < VS_PI) && (diffPhaseCorrection > 0)) ||
        ((diffPhaseCorrection > -VS_PI) && (diffPhaseCorrection < 0)))
    {
        diffPhaseCorrection = 0;
    }

    /* Find cumulative sum of deltas */
    *diffPhaseCorrectionCum = *diffPhaseCorrectionCum + diffPhaseCorrection;
    phaseOut = phase + *diffPhaseCorrectionCum;

    return phaseOut;
}

/**
 *  @brief Compute standard deviation (variance) of an array
 */
static float VitalSigns_computeDeviation(float *a, int32_t n)
{
    if (a == NULL || n < 1)
        return -1.0f;

    float sumX = 0.0f;
    float sumX2 = 0.0f;
    int32_t i;

    for (i = 0; i < n; i++)
    {
        sumX += a[i];
        sumX2 += a[i] * a[i];
    }

    return sumX2 / n - (sumX / n) * (sumX / n);
}

/**
 *  @brief Copy with transpose for 2D FFT
 */
static uint32_t VitalSigns_runCopyTranspose64b(uint64_t *src, uint64_t *dest, uint32_t size, int32_t offset, uint32_t stride, uint32_t pairs)
{
    int32_t i, j, k;
    j = 0;
    for (i = 0; i < (int32_t)size; i++)
    {
        for (k = 0; k < (int32_t)pairs; k++)
        {
            dest[j + k + i * offset] = src[pairs * i + k];
        }
        j += (int32_t)stride;
    }
    return 1;
}

/**
 *  @brief Initialize antenna geometry for IWR6843 AOP
 *
 *  The IWR6843 AOP has 3 TX and 4 RX antennas forming a virtual array.
 *  Virtual antenna positions (in lambda/2 units):
 *  TX0-RX0: (0,0), TX0-RX1: (0,1), TX0-RX2: (0,2), TX0-RX3: (0,3)
 *  TX1-RX0: (1,0), TX1-RX1: (1,1), TX1-RX2: (1,2), TX1-RX3: (1,3)
 *  TX2-RX0: (2,0), TX2-RX1: (2,1), TX2-RX2: (2,2), TX2-RX3: (2,3)
 */
static void VitalSigns_initAntennaGeometry(void)
{
    int32_t txIdx, rxIdx, virtIdx;

    gVsAntenna.numTxAntennas = 3;
    gVsAntenna.numRxAntennas = 4;
    gVsAntenna.numAntRow = 3;     /* TX dimension */
    gVsAntenna.numAntCol = 4;     /* RX dimension */
    gVsAntenna.numRangeBins = 256; /* Default, updated in processFrame */

    virtIdx = 0;
    for (txIdx = 0; txIdx < 3; txIdx++)
    {
        for (rxIdx = 0; rxIdx < 4; rxIdx++)
        {
            gVsAntenna.antennaPos[virtIdx].row = (int8_t)txIdx;
            gVsAntenna.antennaPos[virtIdx].col = (int8_t)rxIdx;
            virtIdx++;
        }
    }
}

/**
 *  @brief Extract radar data from radar cube for VS processing
 */
static void VitalSigns_extractRadarData(cmplx16ImRe_t *radarCube, uint16_t rangeBin, uint16_t numRangeBins, uint8_t numVirtualAnt)
{
    uint32_t dataIdx = 0;
    uint32_t antIdx;
    uint32_t binIdx;
    uint16_t startBin;
    uint32_t antennaOffset;

    /* Calculate starting range bin (center - 2) */
    if (rangeBin < 2)
        startBin = 0;
    else
        startBin = rangeBin - 2;

    /* Ensure we don't exceed radar cube bounds */
    if (startBin + VS_NUM_RANGE_SEL_BIN > numRangeBins)
        startBin = numRangeBins - VS_NUM_RANGE_SEL_BIN;

    antennaOffset = numRangeBins * 4; /* Bytes per antenna (16-bit I/Q per sample) */
    (void)antennaOffset; /* Currently unused - indexing calculated directly */

    /* Extract data for each range bin and virtual antenna */
    for (binIdx = 0; binIdx < VS_NUM_RANGE_SEL_BIN; binIdx++)
    {
        for (antIdx = 0; antIdx < (uint32_t)numVirtualAnt && antIdx < VS_NUM_VIRTUAL_CHANNEL; antIdx++)
        {
            uint32_t radarCubeIdx = (startBin + binIdx) + antIdx * numRangeBins;

            /* Convert from 16-bit fixed point to float */
            gVsDataPerFrame[dataIdx].real = (float)radarCube[radarCubeIdx].real;
            gVsDataPerFrame[dataIdx].imag = (float)radarCube[radarCubeIdx].imag;
            dataIdx++;
        }
    }
}

/**
 *  @brief Pre-processing: DC removal and 2D angle FFT
 */
static void VitalSigns_runPreProcess(uint32_t vsDataCount)
{
    cplxf_t vsDataAngleFftOutBufTemp[VS_NUM_ANGLE_FFT * VS_NUM_ANGLE_FFT];
    cplxf_t pDataTemp[64];
    cplxf_t pDataTempFftout[64];
    float VS_log2abs_buf_temp[VS_NUM_ANGLE_FFT];

    uint16_t vsLastFramePeakIdxI1, vsLastFramePeakIdxI2, vsLastFramePeakIdxI3;
    uint16_t vsLastFramePeakIdxJ1, vsLastFramePeakIdxJ2, vsLastFramePeakIdxJ3;

    uint16_t azimFftIdx, elevFftIdx, dataMeanIdx;
    uint32_t rangeBinIdx;
    float fftLogAbsPeakValue = 0;
    int32_t rad2D = 4;
    uint16_t dataIdx;
    uint32_t dataSetIdx;
    uint16_t fftDataIdx;
    uint16_t dataArrangeIdxCol;
    uint16_t virtualAntennaValid;
    uint16_t virtualAntennaIdx;
    uint16_t virtualAntennaPointer;
    uint16_t numberVirtualAntenna;

    /* ============================================
     * Step 1: Accumulate DC offset mean
     * ============================================ */
    dataIdx = 0;
    for (dataMeanIdx = 0; dataMeanIdx < VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL; dataMeanIdx++)
    {
        gVsMeanBuf[gVsState.vsMeanCntOffset0 + dataMeanIdx].real += gVsDataPerFrame[dataIdx].real;
        gVsMeanBuf[gVsState.vsMeanCntOffset0 + dataMeanIdx].imag += gVsDataPerFrame[dataIdx].imag;
        dataIdx++;
    }

    /* ============================================
     * Step 2: Remove DC offset from current frame
     * ============================================ */
    dataIdx = 0;
    for (dataMeanIdx = 0; dataMeanIdx < VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL; dataMeanIdx++)
    {
        gVsDataPerFrame[dataIdx].real -= gVsMeanBuf[dataMeanIdx + gVsState.vsMeanCntOffset1].real;
        gVsDataPerFrame[dataIdx].imag -= gVsMeanBuf[dataMeanIdx + gVsState.vsMeanCntOffset1].imag;
        dataIdx++;
    }

    /* ============================================
     * Step 3: Compute peak indices with wraparound
     * ============================================ */
    numberVirtualAntenna = gVsAntenna.numRxAntennas * gVsAntenna.numTxAntennas;

    switch (gVsState.lastFramePeakIdxI)
    {
        case 0:
            vsLastFramePeakIdxI1 = VS_NUM_ANGLE_FFT - 1;
            vsLastFramePeakIdxI2 = 0;
            vsLastFramePeakIdxI3 = 1;
            break;
        case VS_NUM_ANGLE_FFT - 1:
            vsLastFramePeakIdxI1 = VS_NUM_ANGLE_FFT - 2;
            vsLastFramePeakIdxI2 = VS_NUM_ANGLE_FFT - 1;
            vsLastFramePeakIdxI3 = 0;
            break;
        default:
            vsLastFramePeakIdxI1 = gVsState.lastFramePeakIdxI - 1;
            vsLastFramePeakIdxI2 = gVsState.lastFramePeakIdxI;
            vsLastFramePeakIdxI3 = gVsState.lastFramePeakIdxI + 1;
            break;
    }

    switch (gVsState.lastFramePeakIdxJ)
    {
        case 0:
            vsLastFramePeakIdxJ1 = VS_NUM_ANGLE_FFT - 1;
            vsLastFramePeakIdxJ2 = 0;
            vsLastFramePeakIdxJ3 = 1;
            break;
        case VS_NUM_ANGLE_FFT - 1:
            vsLastFramePeakIdxJ1 = VS_NUM_ANGLE_FFT - 2;
            vsLastFramePeakIdxJ2 = VS_NUM_ANGLE_FFT - 1;
            vsLastFramePeakIdxJ3 = 0;
            break;
        default:
            vsLastFramePeakIdxJ1 = gVsState.lastFramePeakIdxJ - 1;
            vsLastFramePeakIdxJ2 = gVsState.lastFramePeakIdxJ;
            vsLastFramePeakIdxJ3 = gVsState.lastFramePeakIdxJ + 1;
            break;
    }

    /* ============================================
     * Step 4: 2D Angle FFT for each range bin
     * ============================================ */
    dataSetIdx = vsDataCount * VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN;
    dataIdx = 0;

    for (rangeBinIdx = 0; rangeBinIdx < VS_NUM_RANGE_SEL_BIN; rangeBinIdx++)
    {
        /* --- First dimension FFT (azimuth) --- */
        for (azimFftIdx = 0; azimFftIdx < gVsAntenna.numAntRow; azimFftIdx++)
        {
            memset((uint8_t *)&pDataTemp[0], 0, VS_NUM_ANGLE_FFT * sizeof(cplxf_t));

            /* Arrange data according to antenna geometry */
            for (dataArrangeIdxCol = 0; dataArrangeIdxCol < VS_NUM_ANGLE_FFT; dataArrangeIdxCol++)
            {
                virtualAntennaPointer = 0;
                virtualAntennaValid = 0;

                for (virtualAntennaIdx = 0; virtualAntennaIdx < numberVirtualAntenna; virtualAntennaIdx++)
                {
                    if ((gVsAntenna.antennaPos[virtualAntennaIdx].row == azimFftIdx) &&
                        (gVsAntenna.antennaPos[virtualAntennaIdx].col == dataArrangeIdxCol))
                    {
                        virtualAntennaPointer = virtualAntennaIdx;
                        virtualAntennaValid = 1;
                    }
                }

                if (virtualAntennaValid == 1)
                {
                    pDataTemp[dataArrangeIdxCol].real = gVsDataPerFrame[virtualAntennaPointer + rangeBinIdx * numberVirtualAntenna].real;
                    pDataTemp[dataArrangeIdxCol].imag = gVsDataPerFrame[virtualAntennaPointer + rangeBinIdx * numberVirtualAntenna].imag;
                }
            }

            /* Perform azimuth FFT */
            DSPF_sp_fftSPxSP(
                VS_NUM_ANGLE_FFT,
                (float *)&pDataTemp[0],
                gVsTwiddleAngle,
                (float *)&pDataTemp[VS_NUM_ANGLE_FFT],
                (unsigned char *)gVsBrevFft,
                rad2D,
                0,
                VS_NUM_ANGLE_FFT);

            VitalSigns_runCopyTranspose64b((uint64_t *)&pDataTemp[VS_NUM_ANGLE_FFT],
                                           (uint64_t *)&pDataTempFftout[azimFftIdx],
                                           VS_NUM_ANGLE_FFT, 0, 2, 1);
        }

        fftDataIdx = 0;

        /* --- Second dimension FFT (elevation) --- */
        for (elevFftIdx = 0; elevFftIdx < VS_NUM_ANGLE_FFT; elevFftIdx++)
        {
            memset((uint8_t *)&pDataTemp[gVsAntenna.numAntRow], 0,
                   (VS_NUM_ANGLE_FFT - gVsAntenna.numAntRow) * sizeof(cplxf_t));

            memcpy((uint8_t *)&pDataTemp[0], (uint8_t *)&pDataTempFftout[fftDataIdx],
                   gVsAntenna.numAntRow * sizeof(cplxf_t));
            fftDataIdx += gVsAntenna.numAntRow;

            /* Perform elevation FFT */
            DSPF_sp_fftSPxSP(
                VS_NUM_ANGLE_FFT,
                (float *)&pDataTemp[0],
                gVsTwiddleAngle,
                (float *)&pDataTemp[VS_NUM_ANGLE_FFT],
                (unsigned char *)gVsBrevFft,
                rad2D,
                0,
                VS_NUM_ANGLE_FFT);

            memcpy((uint8_t *)&vsDataAngleFftOutBufTemp[elevFftIdx * VS_NUM_ANGLE_FFT],
                   (uint8_t *)&pDataTemp[VS_NUM_ANGLE_FFT],
                   VS_NUM_ANGLE_FFT * sizeof(cplxf_t));

            /* Accumulate magnitude for peak detection */
            VitalSigns_computeMagnitudeSquared(&pDataTemp[VS_NUM_ANGLE_FFT], VS_log2abs_buf_temp, VS_NUM_ANGLE_FFT);

            for (azimFftIdx = 0; azimFftIdx < VS_NUM_ANGLE_FFT; azimFftIdx++)
            {
                gVsAngleFftMagSum[elevFftIdx * VS_NUM_ANGLE_FFT + azimFftIdx] += VS_log2abs_buf_temp[azimFftIdx];
            }
        }

        /* --- Save 9 angle bins (3x3 around peak) to accumulation buffer --- */
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ1 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI1];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ1 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI2];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ1 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI3];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ2 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI1];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ2 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI2];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ2 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI3];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ3 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI1];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ3 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI2];
        gVsAngleFftBuf[dataSetIdx++] = vsDataAngleFftOutBufTemp[vsLastFramePeakIdxJ3 * VS_NUM_ANGLE_FFT + vsLastFramePeakIdxI3];
    }

    /* ============================================
     * Step 5: Update peak index at end of cycle
     * ============================================ */
    if ((vsDataCount == (VS_TOTAL_FRAME - 1)) || (gVsState.vsLoop == 0 && vsDataCount == 1))
    {
        fftLogAbsPeakValue = 0;

        for (azimFftIdx = 0; azimFftIdx < VS_NUM_ANGLE_FFT; azimFftIdx++)
        {
            for (elevFftIdx = 0; elevFftIdx < VS_NUM_ANGLE_FFT; elevFftIdx++)
            {
                if (gVsAngleFftMagSum[azimFftIdx * VS_NUM_ANGLE_FFT + elevFftIdx] > fftLogAbsPeakValue)
                {
                    fftLogAbsPeakValue = gVsAngleFftMagSum[azimFftIdx * VS_NUM_ANGLE_FFT + elevFftIdx];
                    gVsState.lastFramePeakIdxI = elevFftIdx;
                    gVsState.lastFramePeakIdxJ = azimFftIdx;
                }
            }
        }
        memset(gVsAngleFftMagSum, 0, sizeof(gVsAngleFftMagSum));
    }

    /* ============================================
     * Step 6: Finalize DC mean at end of cycle
     * ============================================ */
    if (vsDataCount == (VS_TOTAL_FRAME - 1))
    {
        for (dataMeanIdx = 0; dataMeanIdx < VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL; dataMeanIdx++)
        {
            gVsMeanBuf[dataMeanIdx + gVsState.vsMeanCntOffset0].real /= VS_TOTAL_FRAME;
            gVsMeanBuf[dataMeanIdx + gVsState.vsMeanCntOffset0].imag /= VS_TOTAL_FRAME;
        }

        memset(&gVsMeanBuf[gVsState.vsMeanCntOffset1], 0,
               VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL * sizeof(cplxf_t));

        /* Swap ping-pong buffers */
        if (gVsState.vsMeanCntOffset0 == 0)
        {
            gVsState.vsMeanCntOffset0 = VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL;
            gVsState.vsMeanCntOffset1 = 0;
        }
        else
        {
            gVsState.vsMeanCntOffset0 = 0;
            gVsState.vsMeanCntOffset1 = VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL;
        }
    }
}

/**
 *  @brief Main vital signs processing: phase unwrap, spectrum FFT, peak detection
 */
static void VitalSigns_computeVitalSignProcessing(uint16_t indicateNoTarget)
{
    uint16_t rangeBinIdx, angleBinIdx, peakCmputeIdx;
    uint32_t frameSizeIdx;
    uint32_t spectrumBinIdx;

    float breathCircularBufferFull[100];

    uint16_t comparePreviousPeak = 0;
    uint32_t addressRearrangeOffset = 0;
    uint16_t compareIndex = 0;
    uint16_t presentPeak[5] = {0};
    uint16_t peakDifferenceArray[5] = {0};
    float compareValue = 0;

    uint16_t breathPeakIdx;
    float breathPeakValue = 0;
    uint16_t heartPeakIdx;
    float heartPeakValue = 0;

    uint16_t breathRateArray[45] = {0};
    uint16_t heartRateArray[45] = {0};
    uint16_t heartRateSub1Array[45] = {0};
    uint16_t heartRateSub2Array[45] = {0};
    uint16_t heartPeakDiff1 = 0;

    cplxf_t pVitalSignsBreathCircularBuffer[VS_PHASE_FFT_SIZE];
    cplxf_t pVitalSignsHeartCircularBuffer[VS_PHASE_FFT_SIZE];
    cplxf_t pVitalSignsSpectrumCplx[VS_PHASE_FFT_SIZE];

    float pVitalSignsBreathAbsSpectrum[VS_PHASE_FFT_SIZE];
    float pVitalSignsBreathAbsSpectrumStorage[VS_PHASE_FFT_SIZE / 2];
    float pVitalSignsHeartAbsSpectrum[VS_PHASE_FFT_SIZE];
    float pVitalSignsHeartAbsSpectrumStorage[VS_PHASE_FFT_SIZE / 2];
    float pVitalSignsHeartAbsSpectrumTemp[VS_PHASE_FFT_SIZE / 2];
    float decimatedSpectrumProduct[VS_PHASE_FFT_SIZE / 2];

    float selPointPhase;
    float outputFilterBreathOut = 0;
    float outputFilterHeartOut = 0;
    float phaseUsedComputationPrev = 0;
    float breathWaveformDeviation;
    float phaseUsedComputation;
    float computePhaseUnwrapPhasePeak;
    float phasePrevFrame;
    float diffPhaseCorrectionCum;
    uint16_t cntbAddr;
    uint16_t selAddr;
    float aTanReal, aTanImag;
    int32_t rad2D = 2;

    memset(pVitalSignsHeartAbsSpectrumStorage, 0, sizeof(pVitalSignsHeartAbsSpectrumStorage));
    memset(pVitalSignsBreathAbsSpectrumStorage, 0, sizeof(pVitalSignsBreathAbsSpectrumStorage));

    /* ============================================
     * Main loop: Process all angle/range bin combinations
     * ============================================ */
    for (angleBinIdx = 0; angleBinIdx < VS_NUM_ANGLE_SEL_BIN; angleBinIdx++)
    {
        cntbAddr = angleBinIdx;

        for (rangeBinIdx = 0; rangeBinIdx < VS_NUM_RANGE_SEL_BIN; rangeBinIdx++)
        {
            memset(pVitalSignsBreathCircularBuffer, 0, sizeof(pVitalSignsBreathCircularBuffer));
            memset(pVitalSignsHeartCircularBuffer, 0, sizeof(pVitalSignsHeartCircularBuffer));

            diffPhaseCorrectionCum = 0;
            selAddr = cntbAddr;
            cntbAddr = VS_NUM_ANGLE_SEL_BIN + cntbAddr;

            /* Get initial phase */
            addressRearrangeOffset = selAddr + gVsState.vsDataCount * VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN;
            if (addressRearrangeOffset >= VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN * VS_TOTAL_FRAME)
            {
                addressRearrangeOffset -= (VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN * VS_TOTAL_FRAME);
            }

            aTanReal = gVsAngleFftBuf[addressRearrangeOffset].real;
            aTanImag = gVsAngleFftBuf[addressRearrangeOffset].imag;
            phasePrevFrame = atan2sp_i(aTanImag, aTanReal);
            phaseUsedComputationPrev = phasePrevFrame;
            selAddr = selAddr + VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN;

            /* Process all frames */
            for (frameSizeIdx = 0; frameSizeIdx < VS_TOTAL_FRAME - 1; frameSizeIdx++)
            {
                addressRearrangeOffset = selAddr + gVsState.vsDataCount * VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN;
                if (addressRearrangeOffset >= VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN * VS_TOTAL_FRAME)
                {
                    addressRearrangeOffset -= (VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN * VS_TOTAL_FRAME);
                }

                aTanReal = gVsAngleFftBuf[addressRearrangeOffset].real;
                aTanImag = gVsAngleFftBuf[addressRearrangeOffset].imag;
                selAddr = selAddr + VS_NUM_RANGE_SEL_BIN * VS_NUM_ANGLE_SEL_BIN;

                selPointPhase = atan2sp_i(aTanImag, aTanReal);
                computePhaseUnwrapPhasePeak = VitalSigns_computePhaseUnwrap(selPointPhase, phasePrevFrame, &diffPhaseCorrectionCum);
                phasePrevFrame = selPointPhase;

                phaseUsedComputation = computePhaseUnwrapPhasePeak - phaseUsedComputationPrev;
                phaseUsedComputationPrev = computePhaseUnwrapPhasePeak;

                outputFilterBreathOut = phaseUsedComputation;
                outputFilterHeartOut = phaseUsedComputation;

                pVitalSignsBreathCircularBuffer[frameSizeIdx].real = outputFilterBreathOut;
                pVitalSignsHeartCircularBuffer[frameSizeIdx].real = outputFilterHeartOut;
            }

            /* Store breath data for deviation calculation */
            if (angleBinIdx == 5 && rangeBinIdx == 3)
            {
                for (frameSizeIdx = 0; frameSizeIdx < 100; frameSizeIdx++)
                {
                    breathCircularBufferFull[frameSizeIdx] = pVitalSignsBreathCircularBuffer[frameSizeIdx].real;
                }
            }

            /* Spectrum FFT for breathing waveform */
            memset(pVitalSignsSpectrumCplx, 0, sizeof(pVitalSignsSpectrumCplx));

            DSPF_sp_fftSPxSP(
                VS_PHASE_FFT_SIZE,
                (float *)pVitalSignsBreathCircularBuffer,
                gVsTwiddleSpectrum,
                (float *)pVitalSignsSpectrumCplx,
                (unsigned char *)gVsBrevFft,
                rad2D,
                0,
                VS_PHASE_FFT_SIZE);

            VitalSigns_computeMagnitudeSquared(pVitalSignsSpectrumCplx, pVitalSignsBreathAbsSpectrum, VS_PHASE_FFT_SIZE);

            /* Find breathing peak */
            breathPeakValue = 0;
            for (spectrumBinIdx = VS_BREATH_INDEX_START; spectrumBinIdx < VS_BREATH_INDEX_END; spectrumBinIdx++)
            {
                compareValue = pVitalSignsBreathAbsSpectrum[spectrumBinIdx] +
                               pVitalSignsBreathAbsSpectrum[spectrumBinIdx + 1] +
                               pVitalSignsBreathAbsSpectrum[spectrumBinIdx - 1];
                if (compareValue > breathPeakValue)
                {
                    breathPeakValue = compareValue;
                    breathPeakIdx = spectrumBinIdx;
                }
            }

            /* Harmonic product spectrum for heart rate */
            for (spectrumBinIdx = 0; spectrumBinIdx < VS_PHASE_FFT_SIZE / 4; spectrumBinIdx++)
            {
                decimatedSpectrumProduct[spectrumBinIdx] =
                    pVitalSignsBreathAbsSpectrum[2 * spectrumBinIdx] * pVitalSignsBreathAbsSpectrum[spectrumBinIdx];
            }

            /* Accumulate spectra */
            for (spectrumBinIdx = VS_BREATH_INDEX_START; spectrumBinIdx < VS_BREATH_INDEX_END; spectrumBinIdx++)
            {
                pVitalSignsBreathAbsSpectrumStorage[spectrumBinIdx] += pVitalSignsBreathAbsSpectrum[spectrumBinIdx];
            }

            for (spectrumBinIdx = VS_HEART_INDEX_START; spectrumBinIdx < VS_HEART_INDEX_END; spectrumBinIdx++)
            {
                pVitalSignsHeartAbsSpectrumStorage[spectrumBinIdx] += decimatedSpectrumProduct[spectrumBinIdx];
            }

            /* Find heart rate peak */
            heartPeakValue = 0;
            for (spectrumBinIdx = VS_HEART_INDEX_START; spectrumBinIdx < VS_HEART_INDEX_END; spectrumBinIdx++)
            {
                compareValue = decimatedSpectrumProduct[spectrumBinIdx] +
                               decimatedSpectrumProduct[spectrumBinIdx + 1] +
                               decimatedSpectrumProduct[spectrumBinIdx - 1];
                if (compareValue > heartPeakValue)
                {
                    heartPeakValue = compareValue;
                    heartPeakIdx = spectrumBinIdx;
                }
            }

            breathRateArray[rangeBinIdx + angleBinIdx * 5] = breathPeakIdx;
            heartRateArray[rangeBinIdx + angleBinIdx * 5] = heartPeakIdx;

            /* Find secondary heart rate peaks */
            decimatedSpectrumProduct[heartPeakIdx] = 0;
            decimatedSpectrumProduct[heartPeakIdx + 1] = 0;
            decimatedSpectrumProduct[heartPeakIdx - 1] = 0;

            heartPeakValue = 0;
            for (spectrumBinIdx = VS_HEART_INDEX_START; spectrumBinIdx < VS_HEART_INDEX_END; spectrumBinIdx++)
            {
                compareValue = decimatedSpectrumProduct[spectrumBinIdx] +
                               decimatedSpectrumProduct[spectrumBinIdx + 1] +
                               decimatedSpectrumProduct[spectrumBinIdx - 1];
                if (compareValue > heartPeakValue)
                {
                    heartPeakValue = compareValue;
                    heartPeakIdx = spectrumBinIdx;
                }
            }
            heartRateSub1Array[rangeBinIdx + angleBinIdx * 5] = heartPeakIdx;

            decimatedSpectrumProduct[heartPeakIdx] = 0;
            decimatedSpectrumProduct[heartPeakIdx + 1] = 0;
            decimatedSpectrumProduct[heartPeakIdx - 1] = 0;

            heartPeakValue = 0;
            for (spectrumBinIdx = VS_HEART_INDEX_START; spectrumBinIdx < VS_HEART_INDEX_END; spectrumBinIdx++)
            {
                compareValue = decimatedSpectrumProduct[spectrumBinIdx] +
                               decimatedSpectrumProduct[spectrumBinIdx + 1] +
                               decimatedSpectrumProduct[spectrumBinIdx - 1];
                if (compareValue > heartPeakValue)
                {
                    heartPeakValue = compareValue;
                    heartPeakIdx = spectrumBinIdx;
                }
            }
            heartRateSub2Array[rangeBinIdx + angleBinIdx * 5] = heartPeakIdx;
        }
    }

    /* ============================================
     * Breathing rate: Histogram voting
     * ============================================ */
    breathPeakValue = 0;
    for (spectrumBinIdx = VS_BREATH_INDEX_START; spectrumBinIdx < VS_BREATH_INDEX_END; spectrumBinIdx++)
    {
        compareValue = pVitalSignsBreathAbsSpectrumStorage[spectrumBinIdx] +
                       pVitalSignsBreathAbsSpectrumStorage[spectrumBinIdx + 1] +
                       pVitalSignsBreathAbsSpectrumStorage[spectrumBinIdx - 1];
        if (compareValue > breathPeakValue)
        {
            breathPeakValue = compareValue;
            breathPeakIdx = spectrumBinIdx;
        }
    }

    memset(pVitalSignsBreathAbsSpectrum, 0, sizeof(pVitalSignsBreathAbsSpectrum));
    for (peakCmputeIdx = 0; peakCmputeIdx < 45; peakCmputeIdx++)
    {
        pVitalSignsBreathAbsSpectrum[breathRateArray[peakCmputeIdx]]++;
    }

    breathPeakValue = 0;
    for (spectrumBinIdx = VS_BREATH_INDEX_START; spectrumBinIdx < VS_BREATH_INDEX_END; spectrumBinIdx++)
    {
        compareValue = pVitalSignsBreathAbsSpectrum[spectrumBinIdx] +
                       pVitalSignsBreathAbsSpectrum[spectrumBinIdx - 1] +
                       pVitalSignsBreathAbsSpectrum[spectrumBinIdx + 1];
        if (compareValue > breathPeakValue)
        {
            breathPeakValue = compareValue;
            breathPeakIdx = spectrumBinIdx;
        }
    }
    gVsState.breathHistIndex = breathPeakIdx;

    /* ============================================
     * Heart rate: Only use center 3 range bins
     * ============================================ */
    for (peakCmputeIdx = 0; peakCmputeIdx < 9; peakCmputeIdx++)
    {
        heartRateArray[5 * peakCmputeIdx] = 0;
        heartRateArray[5 * peakCmputeIdx + 4] = 0;
        heartRateSub1Array[5 * peakCmputeIdx] = 0;
        heartRateSub1Array[5 * peakCmputeIdx + 4] = 0;
        heartRateSub2Array[5 * peakCmputeIdx] = 0;
        heartRateSub2Array[5 * peakCmputeIdx + 4] = 0;
    }
    (void)heartRateSub2Array; /* Reserved for future harmonic product extension */

    memset(pVitalSignsHeartAbsSpectrum, 0, sizeof(pVitalSignsHeartAbsSpectrum));
    for (peakCmputeIdx = 0; peakCmputeIdx < 45; peakCmputeIdx++)
    {
        pVitalSignsHeartAbsSpectrum[heartRateArray[peakCmputeIdx]]++;
        pVitalSignsHeartAbsSpectrum[heartRateSub1Array[peakCmputeIdx]]++;
    }

    heartPeakValue = 0;
    for (spectrumBinIdx = VS_HEART_INDEX_START; spectrumBinIdx < VS_HEART_INDEX_END; spectrumBinIdx++)
    {
        compareValue = pVitalSignsHeartAbsSpectrum[spectrumBinIdx] +
                       pVitalSignsHeartAbsSpectrum[spectrumBinIdx + 1] +
                       pVitalSignsHeartAbsSpectrum[spectrumBinIdx - 1] +
                       pVitalSignsHeartAbsSpectrum[spectrumBinIdx - 2] +
                       pVitalSignsHeartAbsSpectrum[spectrumBinIdx + 2];
        if (compareValue > heartPeakValue)
        {
            heartPeakValue = compareValue;
            heartPeakIdx = spectrumBinIdx;
        }
    }
    gVsState.heartHistIndex = heartPeakIdx;

    /* ============================================
     * Heart rate: Correlation with previous peaks
     * ============================================ */
    memcpy(pVitalSignsHeartAbsSpectrumTemp, pVitalSignsHeartAbsSpectrumStorage, sizeof(pVitalSignsHeartAbsSpectrumTemp));

    for (peakCmputeIdx = 0; peakCmputeIdx < 5; peakCmputeIdx++)
    {
        heartPeakValue = 0;
        for (spectrumBinIdx = VS_HEART_INDEX_START; spectrumBinIdx < VS_HEART_INDEX_END; spectrumBinIdx++)
        {
            compareValue = pVitalSignsHeartAbsSpectrumTemp[spectrumBinIdx] +
                           pVitalSignsHeartAbsSpectrumTemp[spectrumBinIdx - 1] +
                           pVitalSignsHeartAbsSpectrumTemp[spectrumBinIdx + 1];
            if (compareValue > heartPeakValue)
            {
                heartPeakValue = compareValue;
                heartPeakIdx = spectrumBinIdx;
            }
        }
        presentPeak[peakCmputeIdx] = heartPeakIdx;
        pVitalSignsHeartAbsSpectrumTemp[heartPeakIdx] = 0;
        pVitalSignsHeartAbsSpectrumTemp[heartPeakIdx + 1] = 0;
        pVitalSignsHeartAbsSpectrumTemp[heartPeakIdx - 1] = 0;
    }

    comparePreviousPeak = gVsState.previousHeartPeak[3];

    for (peakCmputeIdx = 0; peakCmputeIdx < 5; peakCmputeIdx++)
    {
        if (presentPeak[peakCmputeIdx] > comparePreviousPeak)
            peakDifferenceArray[peakCmputeIdx] = presentPeak[peakCmputeIdx] - comparePreviousPeak;
        else
            peakDifferenceArray[peakCmputeIdx] = comparePreviousPeak - presentPeak[peakCmputeIdx];
    }

    compareValue = 100;
    compareIndex = 0;
    for (peakCmputeIdx = 0; peakCmputeIdx < 5; peakCmputeIdx++)
    {
        if (peakDifferenceArray[peakCmputeIdx] < compareValue)
        {
            compareValue = peakDifferenceArray[peakCmputeIdx];
            compareIndex = peakCmputeIdx;
        }
    }

    if (compareValue < VS_HEART_RATE_DECISION_THRESH)
    {
        heartPeakIdx = presentPeak[compareIndex];
    }
    else
    {
        heartPeakIdx = gVsState.heartHistIndex;
    }

    /* Jump limiting */
    heartPeakDiff1 = 0;
    if (heartPeakIdx > gVsState.previousHeartPeak[0])
        heartPeakDiff1 = heartPeakIdx - gVsState.previousHeartPeak[0];
    else
        heartPeakDiff1 = gVsState.previousHeartPeak[0] - heartPeakIdx;

    if (heartPeakDiff1 > VS_HEART_RATE_JUMP_LIMIT && gVsState.vsLoop > VS_MASK_LOOP_NO)
    {
        if (heartPeakIdx > gVsState.previousHeartPeak[0])
            heartPeakIdx = gVsState.previousHeartPeak[0] + VS_HEART_RATE_JUMP_LIMIT;
        else
            heartPeakIdx = gVsState.previousHeartPeak[0] - VS_HEART_RATE_JUMP_LIMIT;
    }

    /* Update history */
    if (gVsState.vsLoop > 4)
    {
        gVsState.previousHeartPeak[3] = gVsState.previousHeartPeak[2];
        gVsState.previousHeartPeak[2] = gVsState.previousHeartPeak[1];
        gVsState.previousHeartPeak[1] = gVsState.previousHeartPeak[0];
        gVsState.previousHeartPeak[0] = heartPeakIdx;
    }
    else if (gVsState.vsLoop == 0)
    {
        memset(gVsState.previousHeartPeak, 0, sizeof(gVsState.previousHeartPeak));
    }

    /* ============================================
     * Compute final output
     * ============================================ */
    breathWaveformDeviation = VitalSigns_computeDeviation(&breathCircularBufferFull[59], 40);
    gVsOutput.breathingDeviation = breathWaveformDeviation;
    gVsOutput.heartRate = heartPeakIdx * VS_SPECTRUM_MULTIPLICATION_FACTOR;
    gVsOutput.breathingRate = gVsState.breathHistIndex * VS_SPECTRUM_MULTIPLICATION_FACTOR;
    gVsOutput.rangeBin = gVsState.vsRangeBin;
    gVsOutput.id = 0;

    if (indicateNoTarget == 1)
    {
        gVsOutput.id = 0;
        gVsOutput.rangeBin = 0;
        gVsOutput.breathingRate = 0;
        gVsOutput.heartRate = 0;
        gVsOutput.breathingDeviation = 0;
        gVsOutput.valid = 0;
    }
    else
    {
        gVsOutput.valid = (gVsState.vsLoop >= VS_MASK_LOOP_NO) ? 1 : 0;
    }

    if (gVsState.vsLoop < VS_MASK_LOOP_NO)
    {
        gVsOutput.breathingRate = 0;
        gVsOutput.heartRate = 0;
    }
}

/**************************************************************************
 *************************** Public API Implementation ********************
 **************************************************************************/

int32_t VitalSigns_init(VitalSigns_Config *cfg)
{
    if (cfg == NULL)
    {
        return VITALSIGNS_EINVAL;
    }

    /* Store configuration */
    memcpy(&gVsConfig, cfg, sizeof(VitalSigns_Config));

    /* Initialize state */
    memset(&gVsState, 0, sizeof(VitalSigns_State));
    gVsState.vsMeanCntOffset0 = 0;
    gVsState.vsMeanCntOffset1 = VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL;

    /* Initialize output */
    memset(&gVsOutput, 0, sizeof(VitalSigns_Output));

    /* Initialize antenna geometry */
    VitalSigns_initAntennaGeometry();

    /* Clear all buffers */
    memset(gVsDataPerFrame, 0, sizeof(gVsDataPerFrame));
    memset(gVsAngleFftBuf, 0, sizeof(gVsAngleFftBuf));
    memset(gVsMeanBuf, 0, sizeof(gVsMeanBuf));
    memset(gVsSpectrumBuf, 0, sizeof(gVsSpectrumBuf));
    memset(gVsAngleFftMagSum, 0, sizeof(gVsAngleFftMagSum));

    /* Generate twiddle factors */
    VitalSigns_genTwiddle(gVsTwiddleAngle, VS_NUM_ANGLE_FFT);
    VitalSigns_genTwiddle(gVsTwiddleSpectrum, VS_PHASE_FFT_SIZE);
    gVsState.twiddleGenerated = 1;

    gVsState.initialized = 1;

    return VITALSIGNS_EOK;
}

void VitalSigns_deinit(void)
{
    gVsState.initialized = 0;
}

void VitalSigns_reset(void)
{
    /* Reset state but keep configuration */
    gVsState.vsDataCount = 0;
    gVsState.vsLoop = 0;
    gVsState.indicateNoTarget = 0;
    gVsState.lastFramePeakIdxI = 0;
    gVsState.lastFramePeakIdxJ = 0;
    gVsState.targetLostFrames = 0;
    gVsState.heartHistIndex = 0;
    gVsState.breathHistIndex = 0;
    memset(gVsState.previousHeartPeak, 0, sizeof(gVsState.previousHeartPeak));
    gVsState.vsMeanCntOffset0 = 0;
    gVsState.vsMeanCntOffset1 = VS_NUM_RANGE_SEL_BIN * VS_NUM_VIRTUAL_CHANNEL;

    /* Clear buffers */
    memset(gVsAngleFftBuf, 0, sizeof(gVsAngleFftBuf));
    memset(gVsMeanBuf, 0, sizeof(gVsMeanBuf));
    memset(gVsAngleFftMagSum, 0, sizeof(gVsAngleFftMagSum));

    /* Reset output */
    memset(&gVsOutput, 0, sizeof(VitalSigns_Output));
}

int32_t VitalSigns_updateConfig(VitalSigns_Config *cfg)
{
    if (cfg == NULL)
    {
        return VITALSIGNS_EINVAL;
    }

    memcpy(&gVsConfig, cfg, sizeof(VitalSigns_Config));

    /* Reset processing state when config changes */
    VitalSigns_reset();

    return VITALSIGNS_EOK;
}

int32_t VitalSigns_processFrame(
    cmplx16ImRe_t *radarCube,
    uint16_t numRangeBins,
    uint16_t numDopplerChirps,
    uint8_t  numVirtualAnt,
    uint16_t targetRangeBin)
{
    if (!gVsState.initialized)
    {
        return VITALSIGNS_ENOTINIT;
    }

    if (radarCube == NULL)
    {
        return VITALSIGNS_EINVAL;
    }

    if (!gVsConfig.enabled)
    {
        return VITALSIGNS_EOK;
    }

    /* Update antenna geometry with actual values */
    gVsAntenna.numRangeBins = numRangeBins;

    /* Store target range bin */
    gVsState.vsRangeBin = targetRangeBin;

    /* Extract radar data for VS processing */
    if (gVsState.vsDataCount < VS_TOTAL_FRAME)
    {
        VitalSigns_extractRadarData(radarCube, targetRangeBin, numRangeBins, numVirtualAnt);
        VitalSigns_runPreProcess(gVsState.vsDataCount);
        gVsState.vsDataCount++;
    }

    /* Wrap frame counter */
    if (gVsState.vsDataCount >= VS_TOTAL_FRAME)
    {
        gVsState.vsDataCount = 0;
    }

    /* Run VS algorithm every VS_REFRESH_RATE frames */
    if ((gVsState.vsDataCount % VS_REFRESH_RATE) == 0)
    {
        VitalSigns_computeVitalSignProcessing(gVsState.indicateNoTarget);
        gVsState.vsLoop++;
    }

    return VITALSIGNS_EOK;
}

int32_t VitalSigns_getOutput(VitalSigns_Output *output)
{
    if (output == NULL)
    {
        return VITALSIGNS_EINVAL;
    }

    if (!gVsState.initialized)
    {
        return VITALSIGNS_ENOTINIT;
    }

    memcpy(output, &gVsOutput, sizeof(VitalSigns_Output));

    return VITALSIGNS_EOK;
}

int32_t VitalSigns_isOutputReady(void)
{
    return (gVsState.initialized && gVsState.vsLoop >= VS_MASK_LOOP_NO && gVsOutput.valid);
}

uint16_t VitalSigns_getRangeBinFromPosition(float targetX, float targetY, float rangeResolution)
{
    float range;

    if (rangeResolution <= 0)
    {
        return 0;
    }

    range = sqrtf(targetX * targetX + targetY * targetY);

    return (uint16_t)(range / rangeResolution);
}

int32_t VitalSigns_handleTargetLoss(int32_t targetLost)
{
    if (targetLost)
    {
        gVsState.targetLostFrames++;

        if (gVsState.targetLostFrames >= VS_TARGET_PERSIST_FRAMES)
        {
            gVsState.indicateNoTarget = 1;
            return 0;  /* Stop VS processing */
        }

        /* Continue with last known range bin */
        return 1;
    }
    else
    {
        gVsState.targetLostFrames = 0;
        gVsState.indicateNoTarget = 0;
        return 1;  /* Continue VS processing */
    }
}

void VitalSigns_getState(VitalSigns_State *state)
{
    if (state != NULL)
    {
        memcpy(state, &gVsState, sizeof(VitalSigns_State));
    }
}
