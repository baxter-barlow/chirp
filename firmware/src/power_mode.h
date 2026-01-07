/**
 * @file power_mode.h
 * @brief Power management for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Provides power mode control and duty cycling for battery-powered
 * applications. Supports graceful sensor start/stop transitions.
 */

#ifndef CHIRP_POWER_MODE_H
#define CHIRP_POWER_MODE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * Power Mode Definitions
     ******************************************************************************/

    /**
     * @brief Power mode enumeration
     */
    typedef enum Chirp_PowerMode_e
    {
        /** Full power - continuous operation, maximum frame rate */
        CHIRP_POWER_MODE_FULL = 0,

        /** Balanced - moderate duty cycle for typical applications */
        CHIRP_POWER_MODE_BALANCED = 1,

        /** Low power - reduced frame rate, longer sleep periods */
        CHIRP_POWER_MODE_LOW_POWER = 2,

        /** Ultra low power - minimal operation, presence check only */
        CHIRP_POWER_MODE_ULTRA_LOW = 3,

        /** Custom duty cycle - user-defined active/sleep times */
        CHIRP_POWER_MODE_CUSTOM = 4,

        /** Number of power modes */
        CHIRP_POWER_MODE_COUNT

    } Chirp_PowerMode;

    /**
     * @brief Sensor state for power management
     */
    typedef enum Chirp_SensorState_e
    {
        /** Sensor is stopped */
        CHIRP_SENSOR_STATE_STOPPED = 0,

        /** Sensor is starting up */
        CHIRP_SENSOR_STATE_STARTING = 1,

        /** Sensor is running (active) */
        CHIRP_SENSOR_STATE_RUNNING = 2,

        /** Sensor is going to sleep */
        CHIRP_SENSOR_STATE_SLEEPING = 3,

        /** Sensor is in sleep mode */
        CHIRP_SENSOR_STATE_ASLEEP = 4,

        /** Sensor is waking up */
        CHIRP_SENSOR_STATE_WAKING = 5,

        /** Sensor is stopping */
        CHIRP_SENSOR_STATE_STOPPING = 6,

        /** Sensor encountered an error */
        CHIRP_SENSOR_STATE_ERROR = 7

    } Chirp_SensorState;

    /*******************************************************************************
     * Default Timing Parameters (in milliseconds)
     ******************************************************************************/

/** Full power: continuous operation */
#define CHIRP_POWER_FULL_ACTIVE_MS 0xFFFFFFFF
#define CHIRP_POWER_FULL_SLEEP_MS 0

/** Balanced: 500ms active, 500ms sleep (50% duty cycle) */
#define CHIRP_POWER_BALANCED_ACTIVE_MS 500
#define CHIRP_POWER_BALANCED_SLEEP_MS 500

/** Low power: 200ms active, 800ms sleep (20% duty cycle) */
#define CHIRP_POWER_LOW_ACTIVE_MS 200
#define CHIRP_POWER_LOW_SLEEP_MS 800

/** Ultra low: 100ms active, 2000ms sleep (~5% duty cycle) */
#define CHIRP_POWER_ULTRA_ACTIVE_MS 100
#define CHIRP_POWER_ULTRA_SLEEP_MS 2000

    /*******************************************************************************
     * Data Types
     ******************************************************************************/

    /**
     * @brief Power mode configuration
     */
    typedef struct Chirp_PowerConfig_t
    {
        /** Current power mode */
        Chirp_PowerMode mode;

        /** Active time in milliseconds (0xFFFFFFFF = continuous) */
        uint32_t activeMs;

        /** Sleep time in milliseconds */
        uint32_t sleepMs;

        /** Enable automatic duty cycling */
        uint8_t dutyCycleEnabled;

        /** Reserved for alignment */
        uint8_t reserved[3];

    } Chirp_PowerConfig;

    /**
     * @brief Power management state
     */
    typedef struct Chirp_PowerState_t
    {
        /** Current sensor state */
        Chirp_SensorState sensorState;

        /** Time when current state was entered (in system ticks) */
        uint32_t stateEntryTicks;

        /** Time remaining in current state (milliseconds) */
        uint32_t timeRemainingMs;

        /** Number of duty cycles completed */
        uint32_t cycleCount;

        /** Total active time (milliseconds) */
        uint32_t totalActiveMs;

        /** Total sleep time (milliseconds) */
        uint32_t totalSleepMs;

        /** Pending state transition request */
        Chirp_SensorState pendingState;

        /** State transition in progress */
        uint8_t transitionPending;

        /** Reserved */
        uint8_t reserved[3];

    } Chirp_PowerState;

    /*******************************************************************************
     * Function Prototypes
     ******************************************************************************/

    /**
     * @brief Initialize power management
     * @param config Configuration structure to initialize
     * @param state State structure to initialize
     */
    void Chirp_Power_init(Chirp_PowerConfig *config, Chirp_PowerState *state);

    /**
     * @brief Set power mode
     * @param config Configuration structure
     * @param mode Power mode to set
     * @return 0 on success, -1 on error
     */
    int32_t Chirp_Power_setMode(Chirp_PowerConfig *config, Chirp_PowerMode mode);

    /**
     * @brief Set custom duty cycle
     * @param config Configuration structure
     * @param activeMs Active time in milliseconds
     * @param sleepMs Sleep time in milliseconds
     * @return 0 on success, -1 on error
     */
    int32_t Chirp_Power_setCustomDutyCycle(Chirp_PowerConfig *config, uint32_t activeMs, uint32_t sleepMs);

    /**
     * @brief Process power state machine
     * @param config Configuration
     * @param state State (updated)
     * @param currentTicks Current system ticks (from timer)
     * @param ticksPerMs Ticks per millisecond
     * @return Next action: 0=none, 1=start_sensor, 2=stop_sensor
     */
    int32_t Chirp_Power_process(const Chirp_PowerConfig *config, Chirp_PowerState *state, uint32_t currentTicks,
                                uint32_t ticksPerMs);

    /**
     * @brief Request sensor start
     * @param state State structure
     * @return 0 on success, -1 if transition not allowed
     */
    int32_t Chirp_Power_requestStart(Chirp_PowerState *state);

    /**
     * @brief Request sensor stop
     * @param state State structure
     * @return 0 on success, -1 if transition not allowed
     */
    int32_t Chirp_Power_requestStop(Chirp_PowerState *state);

    /**
     * @brief Notify that sensor has started
     * @param state State structure
     */
    void Chirp_Power_notifyStarted(Chirp_PowerState *state);

    /**
     * @brief Notify that sensor has stopped
     * @param state State structure
     */
    void Chirp_Power_notifyStopped(Chirp_PowerState *state);

    /**
     * @brief Get power mode name
     * @param mode Power mode
     * @return Mode name string
     */
    const char *Chirp_Power_getModeName(Chirp_PowerMode mode);

    /**
     * @brief Get sensor state name
     * @param state Sensor state
     * @return State name string
     */
    const char *Chirp_Power_getStateName(Chirp_SensorState state);

    /**
     * @brief Check if sensor should be running
     * @param state State structure
     * @return 1 if sensor should be running, 0 otherwise
     */
    uint8_t Chirp_Power_shouldRun(const Chirp_PowerState *state);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_POWER_MODE_H */
