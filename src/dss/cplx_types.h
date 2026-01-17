/**
 *   @file  cplx_types.h
 *
 *   @brief
 *      Complex number type definitions for DSP processing.
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
#ifndef CPLX_TYPES_H
#define CPLX_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**************************************************************************
 *************************** Endianness Configuration *********************
 **************************************************************************/

/* C674x DSP and ARM R4F are both little-endian on IWR6843 */
#ifndef _LITTLE_ENDIAN
#define _LITTLE_ENDIAN
#endif

/**************************************************************************
 *************************** Complex Type Definitions *********************
 **************************************************************************/

#ifdef _LITTLE_ENDIAN

/**
 * @brief 8-bit complex number (little-endian: imag first)
 */
typedef struct cplx8_t
{
    int8_t imag;
    int8_t real;
} cplx8_t;

/**
 * @brief 16-bit complex number (little-endian: imag first)
 */
typedef struct cplx16_t
{
    int16_t imag;
    int16_t real;
} cplx16_t;

/**
 * @brief 32-bit complex number (little-endian: imag first)
 */
typedef struct cplx32_t
{
    int32_t imag;
    int32_t real;
} cplx32_t;

/**
 * @brief Single-precision floating point complex number
 *
 * Note: For floating point, we use real-first ordering which is
 * more natural for mathematical operations and compatible with
 * DSPLIB FFT functions.
 */
typedef struct cplxf_t
{
    float real;
    float imag;
} cplxf_t;

#else /* _BIG_ENDIAN */

typedef struct cplx8_t
{
    int8_t real;
    int8_t imag;
} cplx8_t;

typedef struct cplx16_t
{
    int16_t real;
    int16_t imag;
} cplx16_t;

typedef struct cplx32_t
{
    int32_t real;
    int32_t imag;
} cplx32_t;

typedef struct cplxf_t
{
    float real;
    float imag;
} cplxf_t;

#endif /* _LITTLE_ENDIAN / _BIG_ENDIAN */

/**************************************************************************
 *************************** Union Types **********************************
 **************************************************************************/

/**
 * @brief Union for 32-bit complex number access
 */
typedef union cplx32u_t
{
    cplx32_t cplx32;
    uint64_t realimag;
} cplx32u_t;

/**
 * @brief Union for 16-bit complex number access
 */
typedef union cplx16u_t
{
    cplx16_t cplx16;
    uint32_t realimag;
} cplx16u_t;

#ifdef __cplusplus
}
#endif

#endif /* CPLX_TYPES_H */
