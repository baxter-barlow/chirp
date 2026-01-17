/* ======================================================================= */
/* fft_twiddle.c -- FFT Twiddle Factor Generation Utility                  */
/*                                                                         */
/* Generates twiddle (rotation) factors for DSPF_sp_fftSPxSP mixed-radix   */
/* FFT algorithm.                                                          */
/*                                                                         */
/* Copyright (C) 2024 Texas Instruments Incorporated                       */
/*                                                                         */
/*  Redistribution and use in source and binary forms, with or without     */
/*  modification, are permitted provided that the following conditions     */
/*  are met:                                                               */
/*                                                                         */
/*    Redistributions of source code must retain the above copyright       */
/*    notice, this list of conditions and the following disclaimer.        */
/*                                                                         */
/*    Redistributions in binary form must reproduce the above copyright    */
/*    notice, this list of conditions and the following disclaimer in the  */
/*    documentation and/or other materials provided with the               */
/*    distribution.                                                        */
/*                                                                         */
/*    Neither the name of Texas Instruments Incorporated nor the names of  */
/*    its contributors may be used to endorse or promote products derived  */
/*    from this software without specific prior written permission.        */
/*                                                                         */
/*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS    */
/*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR  */
/*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT   */
/*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,  */
/*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT       */
/*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,  */
/*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY  */
/*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT    */
/*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  */
/*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.   */
/*                                                                         */
/* ======================================================================= */

#include <math.h>
#include "dsplib.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Generate twiddle factors for mixed-radix FFT
 *
 * Generates twiddle factors (complex exponentials) used by the
 * DSPF_sp_fftSPxSP function. The factors are stored as interleaved
 * cos/sin pairs in the format expected by the FFT algorithm.
 *
 * @param[out] w   Pointer to output twiddle factor array
 *                 Must be at least 2*n floats in size
 * @param[in]  n   FFT size (must be power of 2 or 4)
 *
 * @return Number of twiddle factor elements written (in floats)
 *
 * @note For an N-point FFT, the twiddle array needs approximately
 *       1.5*N elements. Allocate 2*N to be safe.
 *
 * @code
 * // Example: Generate twiddles for 512-pt FFT
 * float twiddle[1024];
 * int count = fft_gen_twiddle(twiddle, 512);
 * @endcode
 */
int fft_gen_twiddle(float *w, int n)
{
    int i, j, k;
    double delta;

    for (j = 1, k = 0; j < n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            delta = 2.0 * M_PI * i / n;

            /* Twiddle factors for radix-4 butterflies */
            w[k + 0] = (float)cos(1.0 * delta);  /* W^1 real */
            w[k + 1] = (float)sin(1.0 * delta);  /* W^1 imag */
            w[k + 2] = (float)cos(2.0 * delta);  /* W^2 real */
            w[k + 3] = (float)sin(2.0 * delta);  /* W^2 imag */
            w[k + 4] = (float)cos(3.0 * delta);  /* W^3 real */
            w[k + 5] = (float)sin(3.0 * delta);  /* W^3 imag */

            k += 6;
        }
    }

    return k;
}

/**
 * @brief Generate twiddle factors using single-precision math
 *
 * Same as fft_gen_twiddle but uses sinf/cosf for platforms where
 * double precision is expensive or unavailable.
 *
 * @param[out] w   Pointer to output twiddle factor array
 * @param[in]  n   FFT size
 *
 * @return Number of twiddle factor elements written
 */
int fft_gen_twiddle_sp(float *w, int n)
{
    int i, j, k;
    float delta;
    const float PI = 3.14159265358979f;

    for (j = 1, k = 0; j < n >> 2; j = j << 2)
    {
        for (i = 0; i < n >> 2; i += j)
        {
            delta = 2.0f * PI * i / n;

            w[k + 0] = cosf(1.0f * delta);
            w[k + 1] = sinf(1.0f * delta);
            w[k + 2] = cosf(2.0f * delta);
            w[k + 3] = sinf(2.0f * delta);
            w[k + 4] = cosf(3.0f * delta);
            w[k + 5] = sinf(3.0f * delta);

            k += 6;
        }
    }

    return k;
}

/* ======================================================================= */
/*  End of file: fft_twiddle.c                                             */
/* ======================================================================= */
