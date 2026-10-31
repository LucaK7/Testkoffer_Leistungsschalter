#include "StateManager.h"
#include "Config.h"

/* ============================================================================
   Initialisierung
   ============================================================================ */
void StateManager::begin()
{
    _mcp.begin();
    _mcp.allOutputsOff();

    for (int i = 0; i < OUTPUT_COUNT; i++)
    {
        _mode[i]           = MODE_24V;
        _enabled[i]        = false;
        _safetyPending[i]  = false;
        _impulseActive[i]  = false;
        _safetyStartMs[i]  = 0;
        _impulseStartMs[i] = 0;
    }
}

/* ============================================================================
   Update
   ============================================================================ */
void StateManager::update()
{
    uint32_t now = millis();

    for (int i = 0; i < OUTPUT_COUNT; i++)
    {
        /* Safety-Wartezeit abgelaufen? */
        if (_safetyPending[i])
        {
            if (now - _safetyStartMs[i] >= SAFETY_DELAY_MS)
            {
                _safetyPending[i] = false;

                if (_enabled[i])
                {
                    _mcp.enableOutput((OutputID)i, _mode[i]);
                }
            }
        }

        /* Impuls abgelaufen? */
        if (_impulseActive[i])
        {
            if (now - _impulseStartMs[i] >= DEFAULT_IMPULSE_MS)
            {
                _impulseActive[i] = false;
                turnOff((OutputID)i);
            }
        }
    }
}

/* ============================================================================
   Modus setzen
   ============================================================================ */
void StateManager::setMode(OutputID output, OutputMode mode)
{
    int i = (int)output;

    /* Ausgang sofort ausschalten */
    turnOff(output);

    /* Modus speichern */
    _mode[i] = mode;
}

/* ============================================================================
   Einschalten
   ============================================================================ */
void StateManager::turnOn(OutputID output)
{
    int i = (int)output;

    /* Ausgang vorbereiten */
    _enabled[i]       = true;
    _safetyPending[i]= true;
    _safetyStartMs[i]= millis();
}

/* ============================================================================
   Ausschalten
   ============================================================================ */
void StateManager::turnOff(OutputID output)
{
    int i = (int)output;

    _enabled[i]       = false;
    _safetyPending[i]= false;
    _impulseActive[i]= false;

    _mcp.disableOutput(output);
}

/* ============================================================================
   Impuls
   ============================================================================ */
void StateManager::startImpulse(OutputID output)
{
    int i = (int)output;

    turnOn(output);

    _impulseActive[i]  = true;
    _impulseStartMs[i] = millis();
}

/* ============================================================================
   Statusabfragen
   ============================================================================ */
OutputMode StateManager::getMode(OutputID output) const
{
    return _mode[(int)output];
}

bool StateManager::isEnabled(OutputID output) const
{
    return _enabled[(int)output];
}

bool StateManager::isImpulseActive(OutputID output) const
{
    return _impulseActive[(int)output];
}

/* ============================================================================
   Sammelbefehle
   ============================================================================ */
void StateManager::allOn()
{
    for (int i = 0; i < OUTPUT_COUNT; i++)
    {
        turnOn((OutputID)i);
    }
}

void StateManager::allOff()
{
    for (int i = 0; i < OUTPUT_COUNT; i++)
    {
        turnOff((OutputID)i);
    }
}
