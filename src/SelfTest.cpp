#include "SelfTest.h"
#include "StateManager.h"
#include "Shell.h"

void SelfTest::begin(StateManager* state, Shell* shell)
{
    _state = state;
    _shell = shell;
}

void SelfTest::start()
{
    if (_running) return;

    _state->allOff();

    _running = true;
    _out  = OUT_MN_MX2;
    _mode = MODE_24V;

    _phase = SET_MODE;
    _phaseStartMs = millis();

    if (_shell)
    {
        Serial.println();
        Serial.println("-----------------------------------------------");
        Serial.println("-----------------------------------------------");
        Serial.print("SelfTest - Stautstabelle");
        _shell->printTableHeader();
    }
}

void SelfTest::stop()
{
    if (!_running) return;

    _state->allOff();

    Serial.println("-----------------------------------------------");
    Serial.println("SelfTest beendet");
    Serial.println();
    _running = false;
    _phase = IDLE;
}
    
void SelfTest::update()
{
    if (!_running) return;

    uint32_t now = millis();

    switch (_phase)
    {
        case SET_MODE:
            _state->setMode(_out, _mode);
            _phaseStartMs = now;
            _phase = WAIT_SAFETY;
            break;

        case WAIT_SAFETY:
            if (now - _phaseStartMs >= SAFETY_DELAY_MS)
                _phase = TURN_ON;
            break;

        case TURN_ON:
            _state->turnOn(_out);
            _phaseStartMs = now;

            if (_shell)
                _shell->printTableRow(_out);

            _phase = WAIT_ACTIVE;
            break;

        case WAIT_ACTIVE:
            if (now - _phaseStartMs >= SELFTEST_MODE_ACTIVE_MS)
                _phase = TURN_OFF;
            break;

        case TURN_OFF:
            _state->turnOff(_out);
            _phaseStartMs = now;

            if (_shell)
                _shell->printTableRow(_out);

            _phase = WAIT_PAUSE;
            break;

        case WAIT_PAUSE:
            if (now - _phaseStartMs >= SELFTEST_STEP_PAUSE_MS)
                nextMode();
            break;

        case DONE:
            stop();
            break;

        default:
            break;
    }
}

void SelfTest::nextMode()
{
    int m = (int)_mode + 1;
    if (m < MODE_COUNT)
    {
        _mode = (OutputMode)m;
        _phase = SET_MODE;
        return;
    }
    _shell->printTableFooter();
    nextOutput();
}

void SelfTest::nextOutput()
{
    int o = (int)_out + 1;
    if (o < OUTPUT_COUNT)
    {
        _out = (OutputID)o;
        _mode = MODE_24V;
        _phase = SET_MODE;
        return;
    }
    
    _phase = DONE;
}
