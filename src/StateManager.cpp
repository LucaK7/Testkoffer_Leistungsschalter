/*
 * ====================================================
 * 📄 Dateiname: StateManager.cpp
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Implementiert die komplette Sicherheitslogik.
 *
 * 🧠 Konzept & Überlegungen:
 * - millis() statt delay()
 * - Keine blockierenden Abläufe
 * - Deterministisches Verhalten
 *
 * ====================================================
 */

#include "StateManager.h"

/*
 * 📌 Beschreibung:
 * Initialisiert StateManager und Hardware.
 *
 * ⚙️ Funktion:
 * - Setzt alle Ausgänge in sicheren Zustand
 *
 * 🔁 Ablauf:
 * - MCP initialisieren
 * - Arrays setzen
 * - Alle Pins AUS
 */
void StateManager::begin()
{
    _mcp.begin();

    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        _currentMode[i] = MODE_24V;
        _isOn[i] = false;
        _waitingForSafety[i] = false;

        _mcp.allPinsOff((OutputID)i);
    }
}

/*
 * 📌 Beschreibung:
 * Zyklische Ablaufsteuerung.
 *
 * ⚙️ Funktion:
 * - Überwacht Safety-Delays
 *
 * 🔁 Ablauf:
 * - Zeit vergleichen
 * - Modus aktivieren
 */
void StateManager::update()
{
    uint32_t now = millis();

    for (uint8_t i = 0; i < OUTPUT_COUNT; i++)
    {
        if (_waitingForSafety[i])
        {
            if (now - _safetyStartMs[i] >= SAFETY_DELAY_MS)
            {
                _mcp.setModePin((OutputID)i, _currentMode[i], true);
                _isOn[i] = true;
                _waitingForSafety[i] = false;

                if (DEBUG_STATE)
                {
                    Serial.print("[STATE] ");
                    Serial.print(getOutputName((OutputID)i));
                    Serial.println(" EIN");
                }
            }
        }
    }
}

/*
 * 📌 Beschreibung:
 * Setzt den Modus eines Ausgangs.
 *
 * ⚙️ Funktion:
 * - Nur erlaubt wenn Ausgang AUS ist
 *
 * 🔁 Ablauf:
 * - Zustand prüfen
 * - Modus speichern
 */
bool StateManager::setMode(OutputID output, OutputMode mode)
{
    if (_isOn[output])
        return false;

    _currentMode[output] = mode;
    return true;
}

/*
 * 📌 Beschreibung:
 * Schaltet einen Ausgang EIN.
 *
 * ⚙️ Funktion:
 * - Erzwingt Sicherheitssequenz
 *
 * 🔁 Ablauf:
 * - Alle Pins AUS
 * - Safety-Timer starten
 */
bool StateManager::turnOn(OutputID output)
{
    if (_isOn[output])
        return false;

    _mcp.allPinsOff(output);
    _safetyStartMs[output] = millis();
    _waitingForSafety[output] = true;

    return true;
}

/*
 * 📌 Beschreibung:
 * Schaltet einen Ausgang AUS.
 *
 * ⚙️ Funktion:
 * - Sofortiger Reset
 *
 * 🔁 Ablauf:
 * - Alle Pins LOW
 */
void StateManager::turnOff(OutputID output)
{
    _mcp.allPinsOff(output);
    _isOn[output] = false;
}
