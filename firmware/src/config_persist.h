/**
 * @file config_persist.h
 * @brief Configuration persistence for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Provides save/load/reset functionality for chirp configurations
 * using the device's flash memory.
 */

#ifndef CHIRP_CONFIG_PERSIST_H
#define CHIRP_CONFIG_PERSIST_H

#include <stdint.h>

#include "chirp.h"
#include "error_codes.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * Configuration Persistence Constants
     ******************************************************************************/

/** Magic number to identify valid configuration */
#define CHIRP_CONFIG_MAGIC 0x43485250 /* "CHRP" */

/** Current configuration version */
#define CHIRP_CONFIG_VERSION 0x0100 /* v1.0 */

/** Default flash offset for configuration storage */
#define CHIRP_CONFIG_FLASH_OFFSET 0x00080000

/** Configuration size (must be multiple of flash page size) */
#define CHIRP_CONFIG_SIZE 4096

    /*******************************************************************************
     * Persisted Configuration Structure
     ******************************************************************************/

    /**
     * @brief Persisted configuration header
     */
    typedef struct Chirp_ConfigHeader_t
    {
        /** Magic number for validation */
        uint32_t magic;

        /** Configuration version */
        uint16_t version;

        /** Configuration size (excluding header) */
        uint16_t size;

        /** CRC32 of configuration data */
        uint32_t crc32;

        /** Reserved for future use */
        uint32_t reserved[2];

    } Chirp_ConfigHeader;

    /**
     * @brief Persisted chirp configuration
     */
    typedef struct Chirp_PersistedConfig_t
    {
        /** Configuration header */
        Chirp_ConfigHeader header;

        /** Output mode configuration */
        Chirp_OutputConfig outputConfig;

        /** Target selection configuration */
        Chirp_TargetConfig targetConfig;

        /** Motion detection configuration */
        Chirp_MotionConfig motionConfig;

        /** Power management configuration */
        Chirp_PowerConfig powerConfig;

        /** Watchdog configuration */
        uint8_t watchdogEnabled;
        uint32_t watchdogTimeoutMs;
        uint8_t watchdogAction;

        /** Padding to ensure alignment */
        uint8_t padding[32];

    } Chirp_PersistedConfig;

    /*******************************************************************************
     * Function Prototypes
     ******************************************************************************/

    /**
     * @brief Save current configuration to flash
     * @param flashOffset Flash offset to write to
     * @return CHIRP_OK on success, error code on failure
     */
    Chirp_ErrorCode Chirp_Config_save(uint32_t flashOffset);

    /**
     * @brief Load configuration from flash
     * @param flashOffset Flash offset to read from
     * @return CHIRP_OK on success, error code on failure
     */
    Chirp_ErrorCode Chirp_Config_load(uint32_t flashOffset);

    /**
     * @brief Check if valid configuration exists in flash
     * @param flashOffset Flash offset to check
     * @return 1 if valid config exists, 0 otherwise
     */
    uint8_t Chirp_Config_exists(uint32_t flashOffset);

    /**
     * @brief Reset configuration to factory defaults
     * @return CHIRP_OK on success
     */
    Chirp_ErrorCode Chirp_Config_factoryReset(void);

    /**
     * @brief Erase saved configuration from flash
     * @param flashOffset Flash offset to erase
     * @return CHIRP_OK on success, error code on failure
     */
    Chirp_ErrorCode Chirp_Config_erase(uint32_t flashOffset);

    /**
     * @brief Get saved configuration info without loading
     * @param flashOffset Flash offset to read from
     * @param header Output header (version, size, crc)
     * @return CHIRP_OK on success, error code on failure
     */
    Chirp_ErrorCode Chirp_Config_getInfo(uint32_t flashOffset, Chirp_ConfigHeader *header);

    /**
     * @brief Calculate CRC32 for configuration validation
     * @param data Data buffer
     * @param length Data length
     * @return CRC32 value
     */
    uint32_t Chirp_Config_crc32(const uint8_t *data, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_CONFIG_PERSIST_H */
