/*
 * ====================================================
 * 📄 Dateiname: McpManager.cpp
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Implementierung der MCP23008-Hardwareabstraktion.
 *
 * 🧠 Konzept & Überlegungen:
 * - Jeder Pin wird einzeln geschrieben
 * - Keine unnötigen MCP-Zugriffe
 * - Maximale Kontrolle & Sicherheit
 *
 * ====================================================
 */

#include "McpManager.h"

/*
 * 📌 Beschreibung:
 * Initialisiert beide MCPs in einen sicheren Zustand.
 *
 * ⚙️ Funktion:
 * - Verhindert undefinierte Pegel beim Start
 *
 * 🔁 Ablauf:
 * - MCPs starten
 * - Alle Pins OUTPUT
 * - Alle Pins LOW
 */
void McpManager::begin()
{
    _mcp1.begin(0x20);
    _mcp2.begin(0x21);

    for (uint8_t i = 0; i < 8; i++)
    {
        _mcp1.pinMode(i, OUTPUT);
        _mcp2.pinMode(i, OUTPUT);

        _mcp1.digitalWrite(i, LOW);
        _mcp2.digitalWrite(i, LOW);
    }
}

/*
 * 📌 Beschreibung:
 * Schaltet alle Pins eines Ausgangs aus.
 *
 * ⚙️ Funktion:
 * - Garantiert spannungsfreien Zustand
 *
 * 🔁 Ablauf:
 * - Iteration über alle 4 Modus-Pins
 */
void McpManager::allPinsOff(OutputID output)
{
    for (uint8_t pin = 0; pin < 4; pin++)
    {
        McpManager::setPin(output, pin, false);
    }
}

/*
 * 📌 Beschreibung:
 * Setzt den Modus-Pin eines Ausgangs.
 *
 * ⚙️ Funktion:
 * - Aktiviert GENAU einen Pin
 *
 * 🔁 Ablauf:
 * - Modus → Pinindex
 * - Weitergabe an setPin()
 */
void McpManager::setModePin(OutputID output, OutputMode mode, bool state)
{
    McpManager::setPin(output, (uint8_t)mode, state);
}

/*
 * 📌 Beschreibung:
 * Setzt einen einzelnen MCP-Pin.
 *
 * ⚙️ Funktion:
 * - Physischer Schreibzugriff
 *
 * 🔁 Ablauf:
 * - MCP auswählen
 * - Basispin berechnen
 * - Pin setzen
 */
void McpManager::setPin(OutputID output, uint8_t pin, bool state)
{
    Adafruit_MCP23008& mcp = getMcp(output);

    uint8_t basePin = (output % 2 == 0) ? 0 : 4;
    mcp.digitalWrite(basePin + pin, state ? HIGH : LOW);

    if (DEBUG_MCP)
    {
        Serial.print("[MCP] ");
        Serial.print(getOutputName(output));
        Serial.print(" Pin ");
        Serial.print(basePin + pin);
        Serial.print(" -> ");
        Serial.println(state ? "HIGH" : "LOW");
    }
}

/*
 * 📌 Beschreibung:
 * Liefert den zuständigen MCP-Baustein.
 *
 * ⚙️ Funktion:
 * - Trennung der MCPs nach Ausgang
 *
 * 🔁 Ablauf:
 * - Vergleich der OutputID
 */
Adafruit_MCP23008& McpManager::getMcp(OutputID output)
{
    return (output < OUT_XF) ? _mcp1 : _mcp2;
}
