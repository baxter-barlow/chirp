/**
 * @file power_mode.c
 * @brief Power management implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "power_mode.h"

#include <string.h>

/*******************************************************************************
 * Private Constants
 ******************************************************************************/

static const char *POWER_MODE_NAMES[] = {"FULL", "BALANCED", "LOW_POWER", "ULTRA_LOW", "CUSTOM"};

static const char *SENSOR_STATE_NAMES[] = {"STOPPED", "STARTING", "RUNNING",  "SLEEPING",
                                           "ASLEEP",  "WAKING",   "STOPPING", "ERROR"};

/* Default timing for each power mode */
static const uint32_t DEFAULT_ACTIVE_MS[] = {
    CHIRP_POWER_FULL_ACTIVE_MS,     /* FULL */
    CHIRP_POWER_BALANCED_ACTIVE_MS, /* BALANCED */
    CHIRP_POWER_LOW_ACTIVE_MS,      /* LOW_POWER */
    CHIRP_POWER_ULTRA_ACTIVE_MS,    /* ULTRA_LOW */
    500                             /* CUSTOM default */
};

static const uint32_t DEFAULT_SLEEP_MS[] = {
    CHIRP_POWER_FULL_SLEEP_MS,     /* FULL */
    CHIRP_POWER_BALANCED_SLEEP_MS, /* BALANCED */
    CHIRP_POWER_LOW_SLEEP_MS,      /* LOW_POWER */
    CHIRP_POWER_ULTRA_SLEEP_MS,    /* ULTRA_LOW */
    500                            /* CUSTOM default */
};

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void Chirp_Power_init(Chirp_PowerConfig *config, Chirp_PowerState *state)
{
    if (config != NULL)
    {
        config->mode = CHIRP_POWER_MODE_FULL;
        config->activeMs = CHIRP_POWER_FULL_ACTIVE_MS;
        config->sleepMs = CHIRP_POWER_FULL_SLEEP_MS;
        config->dutyCycleEnabled = 0;
        memset(config->reserved, 0, sizeof(config->reserved));
    }

    if (state != NULL)
    {
        state->sensorState = CHIRP_SENSOR_STATE_STOPPED;
        state->stateEntryTicks = 0;
        state->timeRemainingMs = 0;
        state->cycleCount = 0;
        state->totalActiveMs = 0;
        state->totalSleepMs = 0;
        state->pendingState = CHIRP_SENSOR_STATE_STOPPED;
        state->transitionPending = 0;
        memset(state->reserved, 0, sizeof(state->reserved));
    }
}

int32_t Chirp_Power_setMode(Chirp_PowerConfig *config, Chirp_PowerMode mode)
{
    if (config == NULL)
    {
        return -1;
    }

    if (mode >= CHIRP_POWER_MODE_COUNT)
    {
        return -1;
    }

    config->mode = mode;
    config->activeMs = DEFAULT_ACTIVE_MS[mode];
    config->sleepMs = DEFAULT_SLEEP_MS[mode];

    /* Enable duty cycling for all modes except FULL */
    config->dutyCycleEnabled = (mode != CHIRP_POWER_MODE_FULL) ? 1 : 0;

    return 0;
}

int32_t Chirp_Power_setCustomDutyCycle(Chirp_PowerConfig *config, uint32_t activeMs, uint32_t sleepMs)
{
    if (config == NULL)
    {
        return -1;
    }

    /* Minimum active time: 50ms */
    if (activeMs < 50 && activeMs != 0)
    {
        return -1;
    }

    config->mode = CHIRP_POWER_MODE_CUSTOM;
    config->activeMs = activeMs;
    config->sleepMs = sleepMs;
    config->dutyCycleEnabled = (sleepMs > 0) ? 1 : 0;

    return 0;
}

int32_t Chirp_Power_process(const Chirp_PowerConfig *config, Chirp_PowerState *state, uint32_t currentTicks,
                            uint32_t ticksPerMs)
{
    int32_t action = 0; /* 0=none, 1=start, 2=stop */
    uint32_t elapsedTicks;
    uint32_t elapsedMs;

    if (config == NULL || state == NULL)
    {
        return 0;
    }

    /* Handle pending transitions */
    if (state->transitionPending)
    {
        switch (state->sensorState)
        {
            case CHIRP_SENSOR_STATE_STOPPED:
                if (state->pendingState == CHIRP_SENSOR_STATE_RUNNING)
                {
                    state->sensorState = CHIRP_SENSOR_STATE_STARTING;
                    state->stateEntryTicks = currentTicks;
                    state->transitionPending = 0;
                    action = 1; /* Request sensor start */
                }
                break;

            case CHIRP_SENSOR_STATE_RUNNING:
                if (state->pendingState == CHIRP_SENSOR_STATE_STOPPED)
                {
                    state->sensorState = CHIRP_SENSOR_STATE_STOPPING;
                    state->stateEntryTicks = currentTicks;
                    state->transitionPending = 0;
                    action = 2; /* Request sensor stop */
                }
                else if (state->pendingState == CHIRP_SENSOR_STATE_ASLEEP && config->dutyCycleEnabled)
                {
                    state->sensorState = CHIRP_SENSOR_STATE_SLEEPING;
                    state->stateEntryTicks = currentTicks;
                    state->transitionPending = 0;
                    action = 2; /* Request sensor stop for sleep */
                }
                break;

            case CHIRP_SENSOR_STATE_ASLEEP:
                if (state->pendingState == CHIRP_SENSOR_STATE_RUNNING)
                {
                    state->sensorState = CHIRP_SENSOR_STATE_WAKING;
                    state->stateEntryTicks = currentTicks;
                    state->transitionPending = 0;
                    action = 1; /* Request sensor start */
                }
                else if (state->pendingState == CHIRP_SENSOR_STATE_STOPPED)
                {
                    state->sensorState = CHIRP_SENSOR_STATE_STOPPED;
                    state->stateEntryTicks = currentTicks;
                    state->transitionPending = 0;
                }
                break;

            default:
                break;
        }
    }

    /* Handle duty cycling if enabled */
    if (config->dutyCycleEnabled && !state->transitionPending)
    {
        elapsedTicks = currentTicks - state->stateEntryTicks;
        elapsedMs = elapsedTicks / ticksPerMs;

        switch (state->sensorState)
        {
            case CHIRP_SENSOR_STATE_RUNNING:
                /* Check if active time has elapsed */
                if (config->activeMs != 0xFFFFFFFF && elapsedMs >= config->activeMs)
                {
                    state->totalActiveMs += config->activeMs;

                    if (config->sleepMs > 0)
                    {
                        /* Transition to sleep */
                        state->pendingState = CHIRP_SENSOR_STATE_ASLEEP;
                        state->transitionPending = 1;
                        state->timeRemainingMs = config->sleepMs;
                    }
                }
                else
                {
                    state->timeRemainingMs = config->activeMs - elapsedMs;
                }
                break;

            case CHIRP_SENSOR_STATE_ASLEEP:
                /* Check if sleep time has elapsed */
                if (elapsedMs >= config->sleepMs)
                {
                    state->totalSleepMs += config->sleepMs;
                    state->cycleCount++;

                    /* Wake up */
                    state->pendingState = CHIRP_SENSOR_STATE_RUNNING;
                    state->transitionPending = 1;
                    state->timeRemainingMs = config->activeMs;
                }
                else
                {
                    state->timeRemainingMs = config->sleepMs - elapsedMs;
                }
                break;

            default:
                break;
        }
    }

    return action;
}

int32_t Chirp_Power_requestStart(Chirp_PowerState *state)
{
    if (state == NULL)
    {
        return -1;
    }

    /* Can only start from stopped or asleep state */
    if (state->sensorState != CHIRP_SENSOR_STATE_STOPPED && state->sensorState != CHIRP_SENSOR_STATE_ASLEEP)
    {
        return -1;
    }

    state->pendingState = CHIRP_SENSOR_STATE_RUNNING;
    state->transitionPending = 1;

    return 0;
}

int32_t Chirp_Power_requestStop(Chirp_PowerState *state)
{
    if (state == NULL)
    {
        return -1;
    }

    /* Can stop from running or asleep state */
    if (state->sensorState != CHIRP_SENSOR_STATE_RUNNING && state->sensorState != CHIRP_SENSOR_STATE_ASLEEP)
    {
        return -1;
    }

    state->pendingState = CHIRP_SENSOR_STATE_STOPPED;
    state->transitionPending = 1;

    return 0;
}

void Chirp_Power_notifyStarted(Chirp_PowerState *state)
{
    if (state == NULL)
    {
        return;
    }

    if (state->sensorState == CHIRP_SENSOR_STATE_STARTING || state->sensorState == CHIRP_SENSOR_STATE_WAKING)
    {
        state->sensorState = CHIRP_SENSOR_STATE_RUNNING;
    }
}

void Chirp_Power_notifyStopped(Chirp_PowerState *state)
{
    if (state == NULL)
    {
        return;
    }

    if (state->sensorState == CHIRP_SENSOR_STATE_STOPPING)
    {
        state->sensorState = CHIRP_SENSOR_STATE_STOPPED;
    }
    else if (state->sensorState == CHIRP_SENSOR_STATE_SLEEPING)
    {
        state->sensorState = CHIRP_SENSOR_STATE_ASLEEP;
    }
}

const char *Chirp_Power_getModeName(Chirp_PowerMode mode)
{
    if (mode >= CHIRP_POWER_MODE_COUNT)
    {
        return "UNKNOWN";
    }
    return POWER_MODE_NAMES[mode];
}

const char *Chirp_Power_getStateName(Chirp_SensorState state)
{
    if (state > CHIRP_SENSOR_STATE_ERROR)
    {
        return "UNKNOWN";
    }
    return SENSOR_STATE_NAMES[state];
}

uint8_t Chirp_Power_shouldRun(const Chirp_PowerState *state)
{
    if (state == NULL)
    {
        return 0;
    }

    return (state->sensorState == CHIRP_SENSOR_STATE_RUNNING) ? 1 : 0;
}
