/**
 * @file phase_extract.h
 * @brief Phase extraction from I/Q data for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Extracts phase and magnitude from complex I/Q radar data.
 * Phase is computed using atan2 and output in fixed-point format.
 */

#ifndef CHIRP_PHASE_EXTRACT_H
#define CHIRP_PHASE_EXTRACT_H

#include <stdint.h>
#include <ti/common/sys_common.h>

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 * Constants
 ******************************************************************************/

/** Maximum bins per phase output */
#define CHIRP_PHASE_MAX_BINS    8

/** Phase scaling: phase_int16 = phase_rad * CHIRP_PHASE_SCALE */
/** Range: -32768 to +32767 maps to -pi to +pi */
#define CHIRP_PHASE_SCALE       10430   /* 32768 / pi */

/*******************************************************************************
 * Data Types
 ******************************************************************************/

/**
 * @brief Phase and magnitude for a single bin
 */
typedef struct Chirp_PhaseBin_t
{
    /** Range bin index */
    uint16_t binIndex;

    /** Phase in fixed-point: value = phase_rad * 10430 (-pi to +pi maps to -32768 to +32767) */
    int16_t phase;

    /** Magnitude (linear, sqrt(I^2 + Q^2)) */
    uint16_t magnitude;

    /** Flags: bit 0 = motion detected, bit 1 = valid */
    uint16_t flags;

} Chirp_PhaseBin;

/** Flag definitions */
#define CHIRP_PHASE_FLAG_MOTION     0x0001
#define CHIRP_PHASE_FLAG_VALID      0x0002

/**
 * @brief Phase output for multiple bins (TLV 0x0520 payload)
 */
typedef struct Chirp_PhaseOutput_t
{
    /** Number of bins in output (1-8) */
    uint16_t numBins;

    /** Center bin index (primary target) */
    uint16_t centerBin;

    /** Timestamp in microseconds since boot */
    uint32_t timestamp_us;

    /** Per-bin phase data */
    Chirp_PhaseBin bins[CHIRP_PHASE_MAX_BINS];

} Chirp_PhaseOutput;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

/**
 * @brief Extract phase and magnitude from complex I/Q sample
 * @param real Real (I) component
 * @param imag Imaginary (Q) component
 * @param phase Output: phase in fixed-point (-32768 to +32767 = -pi to +pi)
 * @param magnitude Output: magnitude (linear)
 */
void Chirp_Phase_extract(int16_t real, int16_t imag, int16_t *phase, uint16_t *magnitude);

/**
 * @brief Extract phase for multiple bins from radar cube
 * @param radarData Pointer to complex I/Q data (cmplx16ImRe_t format: imag, real)
 * @param binIndices Array of bin indices to extract
 * @param numBins Number of bins to extract
 * @param centerBin Center bin (primary target)
 * @param timestamp_us Current timestamp
 * @param output Output structure
 * @return 0 on success, -1 on error
 */
int32_t Chirp_Phase_extractBins(const int16_t *radarData,
                                 const uint16_t *binIndices,
                                 uint8_t numBins,
                                 uint16_t centerBin,
                                 uint32_t timestamp_us,
                                 Chirp_PhaseOutput *output);

/**
 * @brief Convert fixed-point phase to radians (for host SDK reference)
 * @param phaseFixed Phase in fixed-point format
 * @return Phase in radians (-pi to +pi)
 */
float Chirp_Phase_toRadians(int16_t phaseFixed);

/**
 * @brief Convert radians to fixed-point phase
 * @param phaseRad Phase in radians
 * @return Phase in fixed-point format
 */
int16_t Chirp_Phase_fromRadians(float phaseRad);

/**
 * @brief Fast integer atan2 approximation
 * @param y Imaginary component
 * @param x Real component
 * @return Angle in fixed-point (-32768 to +32767 = -pi to +pi)
 */
int16_t Chirp_Phase_atan2(int16_t y, int16_t x);

/**
 * @brief Fast integer square root
 * @param val Input value
 * @return Integer square root
 */
uint16_t Chirp_Phase_sqrt(uint32_t val);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_PHASE_EXTRACT_H */
