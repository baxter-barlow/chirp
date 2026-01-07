/**
 *   @file  mmw_output.h
 *
 *   @brief
 *      This is the interface/message header file for the Millimeter Wave Demo
 *
 *  \par
 *  NOTE:
 *      (C) Copyright 2016 Texas Instruments, Inc.
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
#ifndef MMW_OUTPUT_H
#define MMW_OUTPUT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <ti/common/sys_common.h>
#include <ti/datapath/dpc/objectdetection/objdetdsp/objectdetection.h>

/** @brief Output packet length is a multiple of this value, must be power of 2*/
#define MMWDEMO_OUTPUT_MSG_SEGMENT_LEN 32

    /*!
     * @brief
     *  Message types used in Millimeter Wave Demo for the communication between
     *  target and host, and also for Mailbox communication
     *  between MSS and DSS on the XWR18xx platform. Message types are used to indicate
     *  different type detection information sent out from the target.
     *
     */
    typedef enum MmwDemo_output_message_type_e
    {
        /*! @brief   List of detected points */
        MMWDEMO_OUTPUT_MSG_DETECTED_POINTS = 1,

        /*! @brief   Range profile */
        MMWDEMO_OUTPUT_MSG_RANGE_PROFILE,

        /*! @brief   Noise floor profile */
        MMWDEMO_OUTPUT_MSG_NOISE_PROFILE,

        /*! @brief   Samples to calculate static azimuth  heatmap */
        MMWDEMO_OUTPUT_MSG_AZIMUT_STATIC_HEAT_MAP,

        /*! @brief   Range/Doppler detection matrix */
        MMWDEMO_OUTPUT_MSG_RANGE_DOPPLER_HEAT_MAP,

        /*! @brief   Stats information */
        MMWDEMO_OUTPUT_MSG_STATS,

        /*! @brief   List of detected points */
        MMWDEMO_OUTPUT_MSG_DETECTED_POINTS_SIDE_INFO,

        /*! @brief   Samples to calculate static azimuth/elevation
                     heatmap, (all virtual antennas exported) - unused in this demo
         */
        MMWDEMO_OUTPUT_MSG_AZIMUT_ELEVATION_STATIC_HEAT_MAP,

        /*! @brief   temperature stats from Radar front end */
        MMWDEMO_OUTPUT_MSG_TEMPERATURE_STATS,

        MMWDEMO_OUTPUT_MSG_MAX
    } MmwDemo_output_message_type;

/*******************************************************************************
 * CHIRP CUSTOM TLV TYPES
 * Defined outside enum to avoid SDK version conflicts.
 * TLV types 0x0500-0x05FF reserved for chirp firmware.
 ******************************************************************************/

/** TLV 0x0500: Complex Range FFT - Full I/Q for all range bins */
#define MMWDEMO_OUTPUT_MSG_COMPLEX_RANGE_FFT 0x0500

/** TLV 0x0510: Target I/Q - I/Q for selected target bins only */
#define MMWDEMO_OUTPUT_MSG_TARGET_IQ 0x0510

/** TLV 0x0520: Phase Output - Phase + magnitude for selected bins */
#define MMWDEMO_OUTPUT_MSG_PHASE_OUTPUT 0x0520

/** TLV 0x0540: Presence - Presence detection result */
#define MMWDEMO_OUTPUT_MSG_PRESENCE 0x0540

/** TLV 0x0550: Motion Status - Motion detection result */
#define MMWDEMO_OUTPUT_MSG_MOTION_STATUS 0x0550

/** TLV 0x0560: Target Info - Target selection metadata */
#define MMWDEMO_OUTPUT_MSG_TARGET_INFO 0x0560

    /*******************************************************************************
     * TLV 0x0500: Complex Range FFT Header
     ******************************************************************************/
    /*!
     * @brief   Header for Complex Range FFT TLV payload (8 bytes, 4-byte aligned).
     * @details Data format: cmplx16ImRe_t (imag first, then real, each int16_t)
     */
    typedef struct MmwDemo_output_complexRangeFFT_header_t
    {
        uint16_t numRangeBins; /*!< Number of range bins in payload */
        uint16_t chirpIndex;   /*!< Chirp index (0-based) */
        uint16_t rxAntenna;    /*!< RX antenna index (0-based) */
        uint16_t reserved;     /*!< Padding to 8 bytes */
    } MmwDemo_output_complexRangeFFT_header;

    /*******************************************************************************
     * TLV 0x0510: Target I/Q Header
     ******************************************************************************/
    /*!
     * @brief   Header for Target I/Q TLV payload
     * @details Contains I/Q data for selected bins only (from target selection)
     */
    typedef struct Chirp_output_targetIQ_header_t
    {
        uint16_t numBins;      /*!< Number of bins in payload (1-8) */
        uint16_t centerBin;    /*!< Primary target bin index */
        uint32_t timestamp_us; /*!< Timestamp in microseconds */
    } Chirp_output_targetIQ_header;

    /*!
     * @brief   Per-bin data for Target I/Q TLV (follows header)
     */
    typedef struct Chirp_output_targetIQ_bin_t
    {
        uint16_t binIndex; /*!< Range bin index */
        int16_t imag;      /*!< Imaginary (Q) component */
        int16_t real;      /*!< Real (I) component */
        uint16_t reserved; /*!< Padding */
    } Chirp_output_targetIQ_bin;

    /*******************************************************************************
     * TLV 0x0520: Phase Output Header
     ******************************************************************************/
    /*!
     * @brief   Header for Phase Output TLV payload
     */
    typedef struct Chirp_output_phase_header_t
    {
        uint16_t numBins;      /*!< Number of bins (1-8) */
        uint16_t centerBin;    /*!< Primary target bin index */
        uint32_t timestamp_us; /*!< Timestamp in microseconds */
    } Chirp_output_phase_header;

    /*!
     * @brief   Per-bin data for Phase Output TLV (follows header)
     * @details Phase is fixed-point: -32768 to +32767 = -pi to +pi
     */
    typedef struct Chirp_output_phase_bin_t
    {
        uint16_t binIndex;  /*!< Range bin index */
        int16_t phase;      /*!< Phase (fixed-point, pi/32768 scale) */
        uint16_t magnitude; /*!< Magnitude (linear) */
        uint16_t flags;     /*!< Flags: bit0=motion, bit1=valid */
    } Chirp_output_phase_bin;

    /*******************************************************************************
     * TLV 0x0540: Presence Detection
     ******************************************************************************/
    /*!
     * @brief   Presence detection result payload
     */
    typedef struct Chirp_output_presence_t
    {
        uint8_t presence;   /*!< 0=absent, 1=present, 2=motion */
        uint8_t confidence; /*!< Confidence 0-100 */
        uint16_t range_Q8;  /*!< Range in meters (Q8 fixed point) */
        uint16_t targetBin; /*!< Target range bin */
        uint16_t reserved;  /*!< Padding */
    } Chirp_output_presence;

    /*******************************************************************************
     * TLV 0x0550: Motion Status
     ******************************************************************************/
    /*!
     * @brief   Motion detection result payload
     */
    typedef struct Chirp_output_motion_t
    {
        uint8_t motionDetected;   /*!< Motion detected flag */
        uint8_t motionLevel;      /*!< Motion level 0-255 */
        uint16_t motionBinCount;  /*!< Number of bins with motion */
        uint16_t peakMotionBin;   /*!< Bin with highest motion */
        uint16_t peakMotionDelta; /*!< Peak motion magnitude delta */
    } Chirp_output_motion;

    /*******************************************************************************
     * TLV 0x0560: Target Info
     ******************************************************************************/
    /*!
     * @brief   Target selection information payload
     */
    typedef struct Chirp_output_targetInfo_t
    {
        uint16_t primaryBin;       /*!< Primary target bin index */
        uint16_t primaryMagnitude; /*!< Primary target magnitude */
        uint16_t primaryRange_Q8;  /*!< Primary range (Q8 fixed point meters) */
        uint8_t confidence;        /*!< Confidence 0-100 */
        uint8_t numTargets;        /*!< Number of targets detected */
        uint16_t secondaryBin;     /*!< Secondary target bin (if present) */
        uint16_t reserved;         /*!< Padding */
    } Chirp_output_targetInfo;

    /*!
     * @brief
     *  Message header for reporting detection information from data path.
     *
     * @details
     *  The structure defines the message header.
     */
    typedef struct MmwDemo_output_message_header_t
    {
        /*! @brief   Output buffer magic word (sync word). It is initialized to  {0x0102,0x0304,0x0506,0x0708} */
        uint16_t magicWord[4];

        /*! brief   Version: : MajorNum * 2^24 + MinorNum * 2^16 + BugfixNum * 2^8 + BuildNum   */
        uint32_t version;

        /*! @brief   Total packet length including header in Bytes */
        uint32_t totalPacketLen;

        /*! @brief   platform type */
        uint32_t platform;

        /*! @brief   Frame number */
        uint32_t frameNumber;

        /*! @brief   Time in CPU cycles when the message was created. For XWR16xx/XWR18xx: DSP CPU cycles, for XWR14xx:
         * R4F CPU cycles */
        uint32_t timeCpuCycles;

        /*! @brief   Number of detected objects */
        uint32_t numDetectedObj;

        /*! @brief   Number of TLVs */
        uint32_t numTLVs;

        /*! @brief   For Advanced Frame config, this is the sub-frame number in the range
         * 0 to (number of subframes - 1). For frame config (not advanced), this is always
         * set to 0. */
        uint32_t subFrameNumber;
    } MmwDemo_output_message_header;

    /*!
     * @brief
     * Structure holds message stats information from data path.
     *
     * @details
     *  The structure holds stats information. This is a payload of the TLV message item
     *  that holds stats information.
     */
    typedef struct MmwDemo_output_message_stats_t
    {
        /*! @brief   Interframe processing time in usec */
        uint32_t interFrameProcessingTime;

        /*! @brief   Transmission time of output detection information in usec */
        uint32_t transmitOutputTime;

        /*! @brief   Interframe processing margin in usec */
        uint32_t interFrameProcessingMargin;

        /*! @brief   Interchirp processing margin in usec */
        uint32_t interChirpProcessingMargin;

        /*! @brief   CPU Load (%) during active frame duration */
        uint32_t activeFrameCPULoad;

        /*! @brief   CPU Load (%) during inter frame duration */
        uint32_t interFrameCPULoad;
    } MmwDemo_output_message_stats;

/**
 * @brief
 *  Size of HSRAM Payload data array.
 */
#define MMWDEMO_HSRAM_PAYLOAD_SIZE                                                                                     \
    (SOC_HSRAM_SIZE - sizeof(DPC_ObjectDetection_ExecuteResult) - sizeof(MmwDemo_output_message_stats))

    /**
     * @brief
     *  DSS stores demo output and stats in HSRAM.
     */
    typedef struct MmwDemo_HSRAM_t
    {
        /*! @brief   DPC execution result */
        DPC_ObjectDetection_ExecuteResult result;

        /*! @brief   Output message stats reported by DSS */
        MmwDemo_output_message_stats outStats;

        /*! @brief   Payload data of result */
        uint8_t payload[MMWDEMO_HSRAM_PAYLOAD_SIZE];
    } MmwDemo_HSRAM;

    /**
     * @brief
     *  Message for reporting detected objects from data path.
     *
     * @details
     *  The structure defines the message body for detected objects from from data path.
     */
    typedef struct MmwDemo_output_message_tl_t
    {
        /*! @brief   TLV type */
        uint32_t type;

        /*! @brief   Length in bytes */
        uint32_t length;

    } MmwDemo_output_message_tl;

#ifdef __cplusplus
}
#endif

#endif /* MMW_OUTPUT_H */
