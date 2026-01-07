/**
 * @file watchdog.c
 * @brief Software watchdog implementation
 *
 * chirp - Open source mmWave radar firmware platform
 * https://github.com/baxter-barlow/chirp
 */

#include "watchdog.h"

#include <string.h>

/*******************************************************************************
 * Private Functions
 ******************************************************************************/

static void Chirp_Wdg_logEvent(Chirp_WdgState *state, Chirp_WdgEvent event, uint32_t timestamp, uint32_t data)
{
    if (state == NULL)
    {
        return;
    }

    Chirp_WdgLogEntry *entry = &state->log[state->logIndex];
    entry->event = event;
    entry->timestamp = timestamp;
    entry->frameCount = state->lastFrameCount;
    entry->data = data;

    state->logIndex = (state->logIndex + 1) % CHIRP_WDG_MAX_EVENTS;
}

/*******************************************************************************
 * Public Functions
 ******************************************************************************/

void Chirp_Wdg_init(Chirp_WdgConfig *config, Chirp_WdgState *state)
{
    if (config != NULL)
    {
        config->enabled = 0;
        config->timeoutMs = CHIRP_WDG_DEFAULT_TIMEOUT_MS;
        config->action = CHIRP_WDG_ACTION_LOG;
        memset(config->reserved, 0, sizeof(config->reserved));
    }

    if (state != NULL)
    {
        state->running = 0;
        state->lastKickTicks = 0;
        state->timeoutCount = 0;
        state->recoveryCount = 0;
        state->lastFrameCount = 0;
        state->logIndex = 0;
        memset(state->log, 0, sizeof(state->log));
        memset(state->reserved, 0, sizeof(state->reserved));
    }
}

int32_t Chirp_Wdg_configure(Chirp_WdgConfig *config, uint32_t timeoutMs, Chirp_WdgAction action)
{
    if (config == NULL)
    {
        return -1;
    }

    /* Validate timeout range */
    if (timeoutMs < CHIRP_WDG_MIN_TIMEOUT_MS || timeoutMs > CHIRP_WDG_MAX_TIMEOUT_MS)
    {
        return -1;
    }

    /* Validate action */
    if (action > CHIRP_WDG_ACTION_SYSTEM_RESET)
    {
        return -1;
    }

    config->timeoutMs = timeoutMs;
    config->action = action;
    config->enabled = 1;

    return 0;
}

int32_t Chirp_Wdg_start(const Chirp_WdgConfig *config, Chirp_WdgState *state, uint32_t currentTicks)
{
    if (config == NULL || state == NULL)
    {
        return -1;
    }

    if (!config->enabled)
    {
        return -1;
    }

    state->running = 1;
    state->lastKickTicks = currentTicks;
    state->lastFrameCount = 0;

    Chirp_Wdg_logEvent(state, CHIRP_WDG_EVENT_STARTED, currentTicks, config->timeoutMs);

    return 0;
}

void Chirp_Wdg_stop(Chirp_WdgState *state)
{
    if (state == NULL)
    {
        return;
    }

    if (state->running)
    {
        Chirp_Wdg_logEvent(state, CHIRP_WDG_EVENT_STOPPED, state->lastKickTicks, 0);
        state->running = 0;
    }
}

void Chirp_Wdg_kick(Chirp_WdgState *state, uint32_t currentTicks, uint32_t frameCount)
{
    if (state == NULL || !state->running)
    {
        return;
    }

    state->lastKickTicks = currentTicks;
    state->lastFrameCount = frameCount;
}

Chirp_WdgAction Chirp_Wdg_check(const Chirp_WdgConfig *config, Chirp_WdgState *state, uint32_t currentTicks,
                                uint32_t ticksPerMs)
{
    uint32_t elapsedTicks;
    uint32_t elapsedMs;

    if (config == NULL || state == NULL || !state->running)
    {
        return CHIRP_WDG_ACTION_LOG;
    }

    if (!config->enabled || ticksPerMs == 0)
    {
        return CHIRP_WDG_ACTION_LOG;
    }

    /* Calculate elapsed time since last kick */
    elapsedTicks = currentTicks - state->lastKickTicks;
    elapsedMs = elapsedTicks / ticksPerMs;

    /* Check for timeout */
    if (elapsedMs >= config->timeoutMs)
    {
        state->timeoutCount++;

        /* Log timeout event */
        Chirp_Wdg_logEvent(state, CHIRP_WDG_EVENT_TIMEOUT, currentTicks, elapsedMs);

        /* Execute recovery action */
        if (config->action != CHIRP_WDG_ACTION_LOG)
        {
            state->recoveryCount++;
            Chirp_Wdg_logEvent(state, CHIRP_WDG_EVENT_RECOVERY, currentTicks, (uint32_t)config->action);
        }

        /* Reset kick timer to prevent repeated triggers */
        state->lastKickTicks = currentTicks;

        return config->action;
    }

    return CHIRP_WDG_ACTION_LOG; /* No action needed */
}

uint32_t Chirp_Wdg_getTimeoutCount(const Chirp_WdgState *state)
{
    if (state == NULL)
    {
        return 0;
    }
    return state->timeoutCount;
}

int32_t Chirp_Wdg_getLogEntry(const Chirp_WdgState *state, uint8_t index, Chirp_WdgLogEntry *entry)
{
    uint8_t actualIndex;

    if (state == NULL || entry == NULL || index >= CHIRP_WDG_MAX_EVENTS)
    {
        return -1;
    }

    /* Calculate actual index (0 = most recent) */
    actualIndex = (state->logIndex + CHIRP_WDG_MAX_EVENTS - 1 - index) % CHIRP_WDG_MAX_EVENTS;

    *entry = state->log[actualIndex];
    return 0;
}

const char *Chirp_Wdg_getActionName(Chirp_WdgAction action)
{
    switch (action)
    {
        case CHIRP_WDG_ACTION_LOG:
            return "LOG";
        case CHIRP_WDG_ACTION_RESET_STATE:
            return "RESET_STATE";
        case CHIRP_WDG_ACTION_RESTART_SENSOR:
            return "RESTART_SENSOR";
        case CHIRP_WDG_ACTION_SYSTEM_RESET:
            return "SYSTEM_RESET";
        default:
            return "UNKNOWN";
    }
}
