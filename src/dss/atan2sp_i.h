/**
 *   @file  atan2sp_i.h
 *
 *   @brief
 *      Single precision floating point arctangent two-argument
 *      optimized inline C implementation for C674x DSP.
 *
 *   Adapted from TI IWRL6432 Vital Signs implementation for IWR6843AOPEVM.
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
#ifndef ATAN2SP_I_H
#define ATAN2SP_I_H

#ifdef __cplusplus
extern "C" {
#endif

/**************************************************************************
 *************************** Inline Functions *****************************
 **************************************************************************/

/**
 *  @brief Internal helper for atan2sp_i polynomial evaluation
 *
 *  @param[in] g1    Input value (ratio of y/x or x/y)
 *  @param[in] pih   Pi/2 constant
 *  @param[in] s     Swap flag
 *  @param[in] bn    b negative flag
 *  @param[in] an    a negative flag
 *
 *  @return    atan2 result in radians
 */
static inline float atan2f_sr1i_atan2spi(float g1, float pih, int s, int bn, int an)
{
    float coef;
    float g2, g4, g6, g8, g10, g12;
    float pol;
    float tmp1, tmp2;
    int   ns_nbn;

    /* Polynomial coefficients for atan approximation */
    const float c1 = 0.00230158202f;
    const float c2 = -0.01394551000f;
    const float c3 = 0.03937087815f;
    const float c4 = -0.07235669163f;
    const float c5 = 0.10521499322f;
    const float c6 = -0.14175076797f;
    const float c7 = 0.19989300877f;
    const float c8 = -0.33332930041f;

    /* Get coefficient based on the flags */
    coef = pih;
    if (!s)
    {
        coef = 3.1415927f;
    }

    ns_nbn = s | bn;

    if (!ns_nbn)
    {
        coef = 0;
    }
    if (an)
    {
        coef = -coef;
    }

    /* Calculate polynomial using Horner's method variant */
    g2  = g1 * g1;
    g4  = g2 * g2;
    g6  = g2 * g4;
    g8  = g4 * g4;
    g10 = g6 * g4;
    g12 = g8 * g4;

    tmp1 = ((c5 * g8) + (c6 * g6)) + ((c7 * g4) + (c8 * g2));
    tmp2 = (((c1 * g4 + c2 * g2) + c3) * g12) + (c4 * g10);

    pol = tmp1 + tmp2;
    pol = pol * g1 + g1;

    return (s ? (coef - pol) : (coef + pol));
}

/**
 *  @brief Single precision floating point atan2(y, x)
 *
 *  Computes the arc tangent of y/x, using the signs of both arguments
 *  to determine the quadrant of the return value.
 *
 *  @param[in] a    Y coordinate (numerator)
 *  @param[in] b    X coordinate (denominator)
 *
 *  @return    atan2(a, b) in radians, range [-pi, pi]
 *
 *  @note This implementation is optimized for C674x DSP and provides
 *        approximately 23 bits of precision.
 */
static inline float atan2sp_i(float a, float b)
{
    float g, x, y;
    float res;
    float temp;
    float a_tmp, b_tmp;
    int   an, bn, s;

    const float pih = 1.570796327f;   /* pi/2 */
    const float pi  = 3.141592741f;   /* pi */
    const float Max = 3.402823466E+38F;  /* FLT_MAX */

    x = a;
    y = b;
    s = 0;

    /* Determine sign flags */
    an = (a < 0);  /* flag for a negative */
    bn = (b < 0);  /* flag for b negative */

    /* Get absolute values for comparison */
    a_tmp = (a < 0) ? -a : a;
    b_tmp = (b < 0) ? -b : b;

    /* Swap a and b if |a| > |b| to keep ratio in [0, 1] */
    if (a_tmp > b_tmp)
    {
        temp = b;
        b = a;
        a = temp;
        s = 1;  /* swap flag */
    }

    /* Compute ratio */
    g = a / b;

    /* Polynomial approximation */
    res = atan2f_sr1i_atan2spi(g, pih, s, bn, an);

    /* Handle special cases */
    if (x == 0.0f)
    {
        res = (y >= 0.0f) ? 0 : pi;
    }
    if (g > Max)
    {
        res = pih;
    }
    if (g < -Max)
    {
        res = -pih;
    }

    return res;
}

#ifdef __cplusplus
}
#endif

#endif /* ATAN2SP_I_H */
