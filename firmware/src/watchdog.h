/**
 * @file watchdog.h
 * @brief Software watchdog for chirp firmware
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 *
 * Provides a software watchdog to detect processing stalls
 * and trigger recovery actions.
 */

#ifndef CHIRP_WATCHDOG_H
#define CHIRP_WATCHDOG_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /*******************************************************************************
     * Watchdog Configuration
     ******************************************************************************/

/** Default watchdog timeout in milliseconds */
#define CHIRP_WDG_DEFAULT_TIMEOUT_MS 5000

/** Minimum allowed timeout */
#define CHIRP_WDG_MIN_TIMEOUT_MS 100

/** Maximum allowed timeout */
#define CHIRP_WDG_MAX_TIMEOUT_MS 60000

/** Maximum watchdog events to log */
#define CHIRP_WDG_MAX_EVENTS 8

    /*******************************************************************************
     * Watchdog Event Types
     ******************************************************************************/

    typedef enum Chirp_WdgEvent_e
    {
        /** Watchdog started */
        CHIRP_WDG_EVENT_STARTED = 0,

        /** Watchdog stopped */
        CHIRP_WDG_EVENT_STOPPED = 1,

        /** Watchdog kicked (normal operation) */
        CHIRP_WDG_EVENT_KICKED = 2,

        /** Watchdog timeout occurred */
        CHIRP_WDG_EVENT_TIMEOUT = 3,

        /** Recovery action triggered */
        CHIRP_WDG_EVENT_RECOVERY = 4,

        /** Timeout threshold changed */
        CHIRP_WDG_EVENT_CONFIG = 5

    } Chirp_WdgEvent;

    /*******************************************************************************
     * Watchdog Recovery Actions
     ******************************************************************************/

    typedef enum Chirp_WdgAction_e
    {
        /** Log only, no action */
        CHIRP_WDG_ACTION_LOG = 0,

        /** Reset chirp state */
        CHIRP_WDG_ACTION_RESET_STATE = 1,

        /** Restart sensor */
        CHIRP_WDG_ACTION_RESTART_SENSOR = 2,

        /** Full system reset (if supported) */
        CHIRP_WDG_ACTION_SYSTEM_RESET = 3

    } Chirp_WdgAction;

    /*******************************************************************************
     * Watchdog Event Log Entry
     ******************************************************************************/

    typedef struct Chirp_WdgLogEntry_t
    {
        /** Event type */
        Chirp_WdgEvent event;

        /** Timestamp when event occurred (system ticks) */
        uint32_t timestamp;

        /** Frame count at event time */
        uint32_t frameCount;

        /** Additional event data */
        uint32_t data;

    } Chirp_WdgLogEntry;

    /*******************************************************************************
     * Watchdog Configuration Structure
     ******************************************************************************/

    typedef struct Chirp_WdgConfig_t
    {
        /** Enable watchdog */
        uint8_t enabled;

        /** Timeout in milliseconds */
        uint32_t timeoutMs;

        /** Recovery action on timeout */
        Chirp_WdgAction action;

        /** Reserved */
        uint8_t reserved[3];

    } Chirp_WdgConfig;

    /*******************************************************************************
     * Watchdog State Structure
     ******************************************************************************/

    typedef struct Chirp_WdgState_t
    {
        /** Watchdog running flag */
        uint8_t running;

        /** Last kick timestamp (system ticks) */
        uint32_t lastKickTicks;

        /** Timeout count */
        uint32_t timeoutCount;

        /** Recovery count */
        uint32_t recoveryCount;

        /** Frame count at last kick */
        uint32_t lastFrameCount;

        /** Event log */
        Chirp_WdgLogEntry log[CHIRP_WDG_MAX_EVENTS];

        /** Current log index */
        uint8_t logIndex;

        /** Reserved */
        uint8_t reserved[3];

    } Chirp_WdgState;

    /*******************************************************************************
     * Function Prototypes
     ******************************************************************************/

    /**
     * @brief Initialize watchdog module
     * @param config Configuration structure to initialize
     * @param state State structure to initialize
     */
    void Chirp_Wdg_init(Chirp_WdgConfig *config, Chirp_WdgState *state);

    /**
     * @brief Configure watchdog timeout
     * @param config Configuration structure
     * @param timeoutMs Timeout in milliseconds
     * @param action Recovery action
     * @return 0 on success, error code on failure
     */
    int32_t Chirp_Wdg_configure(Chirp_WdgConfig *config, uint32_t timeoutMs, Chirp_WdgAction action);

    /**
     * @brief Start watchdog monitoring
     * @param config Configuration
     * @param state State structure
     * @param currentTicks Current system ticks
     * @return 0 on success, error code on failure
     */
    int32_t Chirp_Wdg_start(const Chirp_WdgConfig *config, Chirp_WdgState *state, uint32_t currentTicks);

    /**
     * @brief Stop watchdog monitoring
     * @param state State structure
     */
    void Chirp_Wdg_stop(Chirp_WdgState *state);

    /**
     * @brief Kick watchdog (call periodically to prevent timeout)
     * @param state State structure
     * @param currentTicks Current system ticks
     * @param frameCount Current frame count
     */
    void Chirp_Wdg_kick(Chirp_WdgState *state, uint32_t currentTicks, uint32_t frameCount);

    /**
     * @brief Check watchdog status and handle timeout
     * @param config Configuration
     * @param state State structure
     * @param currentTicks Current system ticks
     * @param ticksPerMs System ticks per millisecond
     * @return Recovery action to take (0 if none)
     */
    Chirp_WdgAction Chirp_Wdg_check(const Chirp_WdgConfig *config, Chirp_WdgState *state, uint32_t currentTicks,
                                    uint32_t ticksPerMs);

    /**
     * @brief Get timeout count
     * @param state State structure
     * @return Number of timeouts since start
     */
    uint32_t Chirp_Wdg_getTimeoutCount(const Chirp_WdgState *state);

    /**
     * @brief Get watchdog event log entry
     * @param state State structure
     * @param index Log index (0 = most recent)
     * @param entry Output entry
     * @return 0 on success, -1 if index invalid
     */
    int32_t Chirp_Wdg_getLogEntry(const Chirp_WdgState *state, uint8_t index, Chirp_WdgLogEntry *entry);

    /**
     * @brief Get action name string
     * @param action Action type
     * @return Action name
     */
    const char *Chirp_Wdg_getActionName(Chirp_WdgAction action);

#ifdef __cplusplus
}
#endif

#endif /* CHIRP_WATCHDOG_H */
