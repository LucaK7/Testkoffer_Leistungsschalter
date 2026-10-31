/*
 * ====================================================
 * 📄 Dateiname: StateManager.h
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Zentrale Sicherheits- und Ablaufsteuerung.
 *
 * 🧠 Konzept & Überlegungen:
 * - Erzwingt sichere Schaltreihenfolgen
 * - Verhindert illegale Zustände
 *
 * ====================================================
 */

#ifndef STATEMANAGER_H
#define STATEMANAGER_H

#include <Arduino.h>

#include "Outputs.h"
#include "McpManager.h"
#include "config.h"

class StateManager
{
public:
    void begin();
    void update();

    bool setMode(OutputID output, OutputMode mode);
    bool turnOn(OutputID output);
    void turnOff(OutputID output);

private:
    McpManager _mcp;

    OutputMode _currentMode[OUTPUT_COUNT];
    bool _isOn[OUTPUT_COUNT];

    uint32_t _safetyStartMs[OUTPUT_COUNT];
    bool _waitingForSafety[OUTPUT_COUNT];
};

#endif
