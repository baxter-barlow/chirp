/**
 * @file error_codes.h
 * @brief Error codes and messages for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Provides standardized error codes and human-readable messages
 * for all chirp modules.
 */

#ifndef CHIRP_ERROR_CODES_H
#define CHIRP_ERROR_CODES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * Error Code Definitions
     *
     * Error codes are organized by module:
     * - 0x0000:       Success
     * - 0x0001-0x00FF: General errors
     * - 0x0100-0x01FF: Configuration errors
     * - 0x0200-0x02FF: Target selection errors
     * - 0x0300-0x03FF: Motion detection errors
     * - 0x0400-0x04FF: Power management errors
     * - 0x0500-0x05FF: Phase extraction errors
     * - 0x0600-0x06FF: Output mode errors
     * - 0x0700-0x07FF: Persistence errors
     * - 0x0800-0x08FF: Watchdog errors
     ******************************************************************************/

    typedef enum Chirp_ErrorCode_e
    {
        /* Success */
        CHIRP_OK = 0x0000,

        /* General errors (0x0001-0x00FF) */
        CHIRP_ERR_NULL_PTR = 0x0001,
        CHIRP_ERR_NOT_INITIALIZED = 0x0002,
        CHIRP_ERR_ALREADY_INITIALIZED = 0x0003,
        CHIRP_ERR_INVALID_ARG = 0x0004,
        CHIRP_ERR_OUT_OF_RANGE = 0x0005,
        CHIRP_ERR_BUFFER_TOO_SMALL = 0x0006,
        CHIRP_ERR_NOT_SUPPORTED = 0x0007,
        CHIRP_ERR_BUSY = 0x0008,
        CHIRP_ERR_TIMEOUT = 0x0009,
        CHIRP_ERR_INTERNAL = 0x000A,

        /* Configuration errors (0x0100-0x01FF) */
        CHIRP_ERR_CFG_INVALID_MODE = 0x0100,
        CHIRP_ERR_CFG_INVALID_RANGE = 0x0101,
        CHIRP_ERR_CFG_INVALID_THRESHOLD = 0x0102,
        CHIRP_ERR_CFG_INVALID_BIN = 0x0103,
        CHIRP_ERR_CFG_INVALID_PROFILE = 0x0104,
        CHIRP_ERR_CFG_SENSOR_RUNNING = 0x0105,
        CHIRP_ERR_CFG_CONFLICT = 0x0106,

        /* Target selection errors (0x0200-0x02FF) */
        CHIRP_ERR_TGT_NO_TARGET = 0x0200,
        CHIRP_ERR_TGT_RANGE_INVALID = 0x0201,
        CHIRP_ERR_TGT_SNR_LOW = 0x0202,
        CHIRP_ERR_TGT_BIN_COUNT = 0x0203,

        /* Motion detection errors (0x0300-0x03FF) */
        CHIRP_ERR_MOT_DISABLED = 0x0300,
        CHIRP_ERR_MOT_BIN_RANGE = 0x0301,

        /* Power management errors (0x0400-0x04FF) */
        CHIRP_ERR_PWR_INVALID_MODE = 0x0400,
        CHIRP_ERR_PWR_INVALID_TIMING = 0x0401,
        CHIRP_ERR_PWR_STATE_INVALID = 0x0402,
        CHIRP_ERR_PWR_TRANSITION = 0x0403,

        /* Phase extraction errors (0x0500-0x05FF) */
        CHIRP_ERR_PHS_NO_DATA = 0x0500,
        CHIRP_ERR_PHS_OVERFLOW = 0x0501,

        /* Output mode errors (0x0600-0x06FF) */
        CHIRP_ERR_OUT_INVALID_MODE = 0x0600,
        CHIRP_ERR_OUT_BUFFER_FULL = 0x0601,

        /* Persistence errors (0x0700-0x07FF) */
        CHIRP_ERR_FLASH_WRITE = 0x0700,
        CHIRP_ERR_FLASH_READ = 0x0701,
        CHIRP_ERR_FLASH_ERASE = 0x0702,
        CHIRP_ERR_FLASH_VERIFY = 0x0703,
        CHIRP_ERR_FLASH_NO_CONFIG = 0x0704,
        CHIRP_ERR_FLASH_CORRUPT = 0x0705,

        /* Watchdog errors (0x0800-0x08FF) */
        CHIRP_ERR_WDG_TIMEOUT = 0x0800,
        CHIRP_ERR_WDG_NOT_STARTED = 0x0801

    } Chirp_ErrorCode;

    /*******************************************************************************
     * Error Message Functions
     ******************************************************************************/

    /**
     * @brief Get human-readable error message
     * @param code Error code
     * @return Error message string (never NULL)
     */
    const char *Chirp_Error_getMessage(Chirp_ErrorCode code);

    /**
     * @brief Get error module name from code
     * @param code Error code
     * @return Module name string
     */
    const char *Chirp_Error_getModule(Chirp_ErrorCode code);

    /**
     * @brief Check if error code indicates success
     * @param code Error code
     * @return 1 if success, 0 if error
     */
    static inline uint8_t Chirp_Error_isSuccess(Chirp_ErrorCode code)
    {
        return (code == CHIRP_OK) ? 1 : 0;
    }

    /*******************************************************************************
     * Validation Macros
     ******************************************************************************/

/** Validate pointer is not NULL */
#define CHIRP_VALIDATE_PTR(ptr)   \
    do                            \
    {                             \
        if ((ptr) == NULL)        \
            return CHIRP_ERR_NULL_PTR; \
    } while (0)

/** Validate value is within range [min, max] */
#define CHIRP_VALIDATE_RANGE(val, min, max) \
    do                                      \
    {                                       \
        if ((val) < (min) || (val) > (max)) \
            return CHIRP_ERR_OUT_OF_RANGE;  \
    } while (0)

/** Validate state is initialized */
#define CHIRP_VALIDATE_INIT(state)          \
    do                                      \
    {                                       \
        if (!(state)->initialized)          \
            return CHIRP_ERR_NOT_INITIALIZED; \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_ERROR_CODES_H */
