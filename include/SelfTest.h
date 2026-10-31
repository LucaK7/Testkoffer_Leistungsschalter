#pragma once

#include <Arduino.h>
#include "Outputs.h"
#include "Config.h"

class StateManager;
class Shell;

class SelfTest
{
public:
    void begin(StateManager* state, Shell* shell);

    void start();
    void stop();
    void update();

    bool running() const { return _running; }

private:
    enum Phase
    {
        IDLE,
        SET_MODE,
        WAIT_SAFETY,
        TURN_ON,
        WAIT_ACTIVE,
        TURN_OFF,
        WAIT_PAUSE,
        DONE
    };

    void nextMode();
    void nextOutput();

    StateManager* _state = nullptr;
    Shell*        _shell = nullptr;

    bool     _running = false;
    Phase    _phase   = IDLE;

    OutputID   _out  = OUT_MN_MX2;
    OutputMode _mode = MODE_24V;

    uint32_t _phaseStartMs = 0;
};
