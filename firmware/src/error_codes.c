/**
 * @file error_codes.c
 * @brief Error code message implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "error_codes.h"

/*******************************************************************************
 * Error Messages
 ******************************************************************************/

const char *Chirp_Error_getMessage(Chirp_ErrorCode code)
{
    switch (code)
    {
        /* Success */
        case CHIRP_OK:
            return "Success";

        /* General errors */
        case CHIRP_ERR_NULL_PTR:
            return "Null pointer";
        case CHIRP_ERR_NOT_INITIALIZED:
            return "Not initialized";
        case CHIRP_ERR_ALREADY_INITIALIZED:
            return "Already initialized";
        case CHIRP_ERR_INVALID_ARG:
            return "Invalid argument";
        case CHIRP_ERR_OUT_OF_RANGE:
            return "Value out of range";
        case CHIRP_ERR_BUFFER_TOO_SMALL:
            return "Buffer too small";
        case CHIRP_ERR_NOT_SUPPORTED:
            return "Not supported";
        case CHIRP_ERR_BUSY:
            return "Resource busy";
        case CHIRP_ERR_TIMEOUT:
            return "Timeout";
        case CHIRP_ERR_INTERNAL:
            return "Internal error";

        /* Configuration errors */
        case CHIRP_ERR_CFG_INVALID_MODE:
            return "Invalid output mode";
        case CHIRP_ERR_CFG_INVALID_RANGE:
            return "Invalid range configuration";
        case CHIRP_ERR_CFG_INVALID_THRESHOLD:
            return "Invalid threshold value";
        case CHIRP_ERR_CFG_INVALID_BIN:
            return "Invalid bin index";
        case CHIRP_ERR_CFG_INVALID_PROFILE:
            return "Invalid profile name";
        case CHIRP_ERR_CFG_SENSOR_RUNNING:
            return "Cannot configure while sensor running";
        case CHIRP_ERR_CFG_CONFLICT:
            return "Configuration conflict";

        /* Target selection errors */
        case CHIRP_ERR_TGT_NO_TARGET:
            return "No target detected";
        case CHIRP_ERR_TGT_RANGE_INVALID:
            return "Target range invalid";
        case CHIRP_ERR_TGT_SNR_LOW:
            return "Target SNR too low";
        case CHIRP_ERR_TGT_BIN_COUNT:
            return "Invalid track bin count";

        /* Motion detection errors */
        case CHIRP_ERR_MOT_DISABLED:
            return "Motion detection disabled";
        case CHIRP_ERR_MOT_BIN_RANGE:
            return "Motion bin range invalid";

        /* Power management errors */
        case CHIRP_ERR_PWR_INVALID_MODE:
            return "Invalid power mode";
        case CHIRP_ERR_PWR_INVALID_TIMING:
            return "Invalid duty cycle timing";
        case CHIRP_ERR_PWR_STATE_INVALID:
            return "Invalid sensor state";
        case CHIRP_ERR_PWR_TRANSITION:
            return "State transition not allowed";

        /* Phase extraction errors */
        case CHIRP_ERR_PHS_NO_DATA:
            return "No phase data available";
        case CHIRP_ERR_PHS_OVERFLOW:
            return "Phase buffer overflow";

        /* Output mode errors */
        case CHIRP_ERR_OUT_INVALID_MODE:
            return "Invalid output mode";
        case CHIRP_ERR_OUT_BUFFER_FULL:
            return "Output buffer full";

        /* Persistence errors */
        case CHIRP_ERR_FLASH_WRITE:
            return "Flash write failed";
        case CHIRP_ERR_FLASH_READ:
            return "Flash read failed";
        case CHIRP_ERR_FLASH_ERASE:
            return "Flash erase failed";
        case CHIRP_ERR_FLASH_VERIFY:
            return "Flash verify failed";
        case CHIRP_ERR_FLASH_NO_CONFIG:
            return "No saved configuration";
        case CHIRP_ERR_FLASH_CORRUPT:
            return "Configuration corrupt";

        /* Watchdog errors */
        case CHIRP_ERR_WDG_TIMEOUT:
            return "Watchdog timeout";
        case CHIRP_ERR_WDG_NOT_STARTED:
            return "Watchdog not started";

        default:
            return "Unknown error";
    }
}

const char *Chirp_Error_getModule(Chirp_ErrorCode code)
{
    uint16_t module = (uint16_t)(code & 0xFF00);

    switch (module)
    {
        case 0x0000:
            return "General";
        case 0x0100:
            return "Config";
        case 0x0200:
            return "Target";
        case 0x0300:
            return "Motion";
        case 0x0400:
            return "Power";
        case 0x0500:
            return "Phase";
        case 0x0600:
            return "Output";
        case 0x0700:
            return "Flash";
        case 0x0800:
            return "Watchdog";
        default:
            return "Unknown";
    }
}
