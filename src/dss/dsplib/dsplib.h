/* ======================================================================== *
 * Local DSPlib - Project-integrated DSP functions for C674x               *
 *                                                                         *
 * This unified header provides access to DSP functions from TI DSPlib,    *
 * integrated locally for project visibility and customization.            *
 *                                                                         *
 * Included functions:                                                     *
 *   - DSPF_sp_fftSPxSP:  Single-precision complex FFT (mixed radix)       *
 *   - DSPF_sp_ifftSPxSP: Single-precision complex IFFT (mixed radix)      *
 *   - fft_gen_twiddle:   Twiddle factor generation utility                *
 *                                                                         *
 * Note: This is a natural C implementation. For maximum performance,      *
 * use USE_LOCAL_DSPLIB=0 to link against the assembly-optimized vendor    *
 * library.                                                                *
 *                                                                         *
 * Copyright (C) 2011 Texas Instruments Incorporated - http://www.ti.com/  *
 * ======================================================================= */

#ifndef _LOCAL_DSPLIB_H_
#define _LOCAL_DSPLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "fft_sp.h"
#include "ifft_sp.h"

/**
 * @brief Generate twiddle factors for FFT
 *
 * @param[out] w   Pointer to twiddle factor array (allocate 2*n floats)
 * @param[in]  n   FFT size (power of 2 or 4)
 * @return Number of elements written
 */
int fft_gen_twiddle(float *w, int n);

/**
 * @brief Generate twiddle factors using single-precision math
 *
 * @param[out] w   Pointer to twiddle factor array
 * @param[in]  n   FFT size
 * @return Number of elements written
 */
int fft_gen_twiddle_sp(float *w, int n);

#ifdef __cplusplus
}
#endif

#endif /* _LOCAL_DSPLIB_H_ */

/* ======================================================================== */
/*  End of file: dsplib.h                                                   */
/* ======================================================================== */
