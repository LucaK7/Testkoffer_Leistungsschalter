#include "McpManager.h"

/* ============================================================================
   Initialisierung der MCPs
   ============================================================================ */
void McpManager::begin()
{
    /* MCP-Adressen */
    _mcp1.begin(0x20);
    _mcp2.begin(0x21);

    /* Alle Pins als OUTPUT setzen und LOW schalten */
    for (uint8_t i = 0; i < 8; i++)
    {
        _mcp1.pinMode(i, OUTPUT);
        _mcp2.pinMode(i, OUTPUT);

        _mcp1.digitalWrite(i, LOW);
        _mcp2.digitalWrite(i, LOW);
    }
}

/* ============================================================================
   Alle Ausgänge ausschalten (Global)
   ============================================================================ */
void McpManager::allOutputsOff()
{
    /* Beide MCPs vollständig auf LOW setzen */
    for (uint8_t i = 0; i < 8; i++)
    {
        _mcp1.digitalWrite(i, LOW);
        _mcp2.digitalWrite(i, LOW);
    }
}

/* ============================================================================
   Ausgang einschalten
   ============================================================================ */
void McpManager::enableOutput(OutputID output, OutputMode mode)
{
    /* Sicherheitsregel:
       Immer zuerst ALLE Pins dieses Ausgangs ausschalten */
    disableOutput(output);

    /* Gewünschten Modus aktivieren */
    bool useMcp1;
    uint8_t pin = resolvePin(output, mode, useMcp1);

    (useMcp1 ? _mcp1 : _mcp2).digitalWrite(pin, HIGH);
}

/* ============================================================================
   Ausgang ausschalten
   ============================================================================ */
void McpManager::disableOutput(OutputID output)
{
    /* Alle 4 Modus-Pins dieses Ausgangs explizit LOW setzen */
    for (uint8_t m = 0; m < MODE_COUNT; m++)
    {
        bool useMcp1;
        uint8_t pin = resolvePin(output, (OutputMode)m, useMcp1);

        (useMcp1 ? _mcp1 : _mcp2).digitalWrite(pin, LOW);
    }
}

/* ============================================================================
   Pin-Auflösung
   ============================================================================ */
uint8_t McpManager::resolvePin(OutputID output, OutputMode mode, bool& useMcp1)
{
    /* MCP-Zuordnung:
       - Erste zwei Ausgänge → MCP1
       - Letzte zwei Ausgänge → MCP2 */
    useMcp1 = (output < 2);

    /* Jeder Ausgang nutzt 4 Pins */
    uint8_t basePin = (output % 2) * MODE_COUNT;

    /* Pin = Basis + Modus */
    return basePin + mode;
}
