/**
 *   @file  vitalsign_common.h
 *
 *   @brief
 *      Common definitions and structures for Vital Signs processing
 *      shared between MSS (ARM R4F) and DSS (C674x DSP).
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
#ifndef VITALSIGN_COMMON_H
#define VITALSIGN_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**************************************************************************
 *************************** Constants ************************************
 **************************************************************************/

/** @brief TLV type for vital signs output message */
#define MMWDEMO_OUTPUT_MSG_VS                   0x410

/** @brief Mathematical constant PI */
#define VS_PI                                   3.1415926535897f

/** @brief Total number of frames to accumulate for VS processing */
#define VS_TOTAL_FRAME                          128

/** @brief Number of frames between VS output updates */
#define VS_REFRESH_RATE                         32

/** @brief Number of range bins to process for vital signs */
#define VS_NUM_RANGE_SEL_BIN                    5

/** @brief Number of angle bins to select after angle FFT */
#define VS_NUM_ANGLE_SEL_BIN                    9

/** @brief Size of angle FFT (azimuth and elevation) */
#define VS_NUM_ANGLE_FFT                        16

/** @brief Number of virtual antennas on IWR6843 (3TX * 4RX = 12) */
#define VS_NUM_VIRTUAL_CHANNEL                  12

/** @brief FFT size for phase spectrum analysis */
#define VS_PHASE_FFT_SIZE                       512

/** @brief Start index for heart rate detection in spectrum (68 BPM / 60 * 512 / 9Hz) */
#define VS_HEART_INDEX_START                    68

/** @brief End index for heart rate detection in spectrum */
#define VS_HEART_INDEX_END                      128

/** @brief Start index for breathing rate detection in spectrum */
#define VS_BREATH_INDEX_START                   3

/** @brief End index for breathing rate detection in spectrum */
#define VS_BREATH_INDEX_END                     50

/** @brief Threshold for heart rate decision based on correlation */
#define VS_HEART_RATE_DECISION_THRESH           3

/** @brief Maximum allowed jump in heart rate between frames */
#define VS_HEART_RATE_JUMP_LIMIT                12

/** @brief Number of VS loops to wait before outputting valid data */
#define VS_MASK_LOOP_NO                         7

/** @brief Multiplication factor for spectrum to BPM conversion */
#define VS_SPECTRUM_MULTIPLICATION_FACTOR       0.882f

/** @brief Number of frames to persist range bin after target loss */
#define VS_TARGET_PERSIST_FRAMES                50

/**************************************************************************
 *************************** Data Structures ******************************
 **************************************************************************/

/**
 * @brief
 *  Vital Signs Configuration structure
 *
 * @details
 *  This structure contains the configuration parameters for vital signs
 *  processing. It is sent from MSS to DSS via mailbox/HSRAM.
 */
typedef struct VitalSigns_Config_t
{
    /** @brief Enable/disable vital signs processing (0=off, 1=on) */
    uint8_t  enabled;

    /** @brief Enable tracker integration (0=fixed range, 1=use tracker) */
    uint8_t  trackerIntegration;

    /** @brief Target ID to track (0-249, 255=nearest target) */
    uint8_t  targetId;

    /** @brief Reserved for alignment */
    uint8_t  reserved;

    /** @brief Starting range bin for VS processing */
    uint16_t rangeBinStart;

    /** @brief Number of range bins to process (1-5) */
    uint16_t numRangeBins;

    /** @brief Range resolution in meters per bin */
    float    rangeResolution;

} VitalSigns_Config;

/**
 * @brief
 *  Vital Signs Output structure
 *
 * @details
 *  This structure contains the vital signs measurement results.
 *  It is stored in HSRAM for MSS to read and transmit via UART.
 *  Total size: 20 bytes (aligned to 4 bytes)
 */
typedef struct VitalSigns_Output_t
{
    /** @brief Tracker target ID being monitored */
    uint16_t id;

    /** @brief Active range bin being processed */
    uint16_t rangeBin;

    /** @brief Heart rate in BPM (0 if invalid) */
    float    heartRate;

    /** @brief Breathing rate in BPM (0 if invalid) */
    float    breathingRate;

    /** @brief Breathing deviation (presence indicator) */
    float    breathingDeviation;

    /** @brief Validity flag (1=valid measurement, 0=invalid) */
    uint8_t  valid;

    /** @brief Alignment padding */
    uint8_t  reserved[3];

} VitalSigns_Output;

/**
 * @brief
 *  UART TLV output structure for vital signs
 *
 * @details
 *  This structure is used for TLV output over UART (type 0x410).
 *  Matches VitalSigns_Output for direct memcpy.
 */
typedef struct MmwDemo_output_message_vitalsigns_t
{
    /** @brief Tracker target ID */
    uint16_t targetId;

    /** @brief Active range bin */
    uint16_t rangeBin;

    /** @brief Heart rate in BPM (0 if invalid) */
    float    heartRate;

    /** @brief Breathing rate in BPM (0 if invalid) */
    float    breathingRate;

    /** @brief Breathing waveform deviation (presence indicator) */
    float    breathingDeviation;

    /** @brief 1=valid measurement, 0=invalid */
    uint8_t  valid;

    /** @brief Alignment padding */
    uint8_t  reserved[3];

} MmwDemo_output_message_vitalsigns;

/**
 * @brief
 *  Antenna geometry definition for a single antenna
 *
 * @details
 *  Defines the position of a virtual antenna in the array
 *  in steps of lambda/2.
 */
typedef struct VitalSigns_AntennaGeometryAnt_t
{
    /** @brief Row index in steps of lambda/2 */
    int8_t row;

    /** @brief Column index in steps of lambda/2 */
    int8_t col;

} VitalSigns_AntennaGeometryAnt;

/**
 * @brief
 *  Antenna geometry configuration
 *
 * @details
 *  Defines the virtual antenna array geometry for IWR6843.
 *  Used for 2D angle FFT processing.
 */
typedef struct VitalSigns_AntennaGeometry_t
{
    /** @brief Virtual antenna positions (12 antennas for 3TX*4RX) */
    VitalSigns_AntennaGeometryAnt  antennaPos[VS_NUM_VIRTUAL_CHANNEL];

    /** @brief Number of antenna rows */
    uint16_t numAntRow;

    /** @brief Number of antenna columns */
    uint16_t numAntCol;

    /** @brief Number of TX antennas */
    uint16_t numTxAntennas;

    /** @brief Number of RX antennas */
    uint16_t numRxAntennas;

    /** @brief Number of range bins in radar cube */
    uint32_t numRangeBins;

} VitalSigns_AntennaGeometry;

/**
 * @brief
 *  Internal state for vital signs processing
 *
 * @details
 *  Maintains processing state across frames.
 *  This is internal to DSS and not shared with MSS.
 */
typedef struct VitalSigns_State_t
{
    /** @brief Current frame count in VS cycle (0-127) */
    uint32_t vsDataCount;

    /** @brief VS loop iteration counter */
    uint32_t vsLoop;

    /** @brief Current target range bin */
    uint16_t vsRangeBin;

    /** @brief Flag indicating no target present */
    uint16_t indicateNoTarget;

    /** @brief Last frame peak index I (azimuth) */
    uint16_t lastFramePeakIdxI;

    /** @brief Last frame peak index J (elevation) */
    uint16_t lastFramePeakIdxJ;

    /** @brief Frames since last valid target */
    uint16_t targetLostFrames;

    /** @brief Heart rate histogram peak index */
    uint16_t heartHistIndex;

    /** @brief Breath rate histogram peak index */
    uint16_t breathHistIndex;

    /** @brief Previous heart rate peaks for tracking */
    uint16_t previousHeartPeak[4];

    /** @brief Mean buffer offset (ping-pong) */
    uint16_t vsMeanCntOffset0;

    /** @brief Mean buffer offset (ping-pong) */
    uint16_t vsMeanCntOffset1;

    /** @brief Twiddle factors generated flag */
    uint8_t  twiddleGenerated;

    /** @brief Module initialized flag */
    uint8_t  initialized;

} VitalSigns_State;

#ifdef __cplusplus
}
#endif

#endif /* VITALSIGN_COMMON_H */
