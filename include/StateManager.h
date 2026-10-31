#pragma once

/* ============================================================================
   Datei: StateManager.h

   Funktion:
   ---------
   Zentrale Zustands- und Ablaufsteuerung aller Ausgänge.
   Diese Klasse ist das "Gehirn" zwischen Logik (Shell, SelfTest)
   und Hardware (McpManager).

   AUFGABEN:
   ----------
   - Speichert den aktuellen Modus jedes Ausgangs
   - Verwaltert Ein / Aus Zustände
   - Steuert Impulsbetrieb
   - Erzwingt Sicherheits-Wartezeiten beim Umschalten
   - Führt alle zeitabhängigen Abläufe über millis() aus

   WICHTIGE REGELN:
   ----------------
   👉 Kein direkter Zugriff auf MCP außerhalb des McpManager
   👉 Keine delay()-Aufrufe
   👉 Zustände sind IMMER eindeutig definiert

   ============================================================================ */

#include <Arduino.h>
#include "Outputs.h"
#include "McpManager.h"

class StateManager
{
public:
    /* ------------------------------------------------------------------------
       Initialisierung
       ------------------------------------------------------------------------
       Beschreibung:
       - Initialisiert interne Zustände
       - Setzt alle Ausgänge logisch AUS
       - Ruft den MCP-Layer für einen sicheren Startzustand auf
       ------------------------------------------------------------------------ */
    void begin();

    /* ------------------------------------------------------------------------
       Zyklisches Update
       ------------------------------------------------------------------------
       Beschreibung:
       - MUSS regelmäßig aus loop() aufgerufen werden
       - Verarbeitet:
         • Safety-Wartezeiten
         • Impulsabschaltungen
       ------------------------------------------------------------------------ */
    void update();

    /* ------------------------------------------------------------------------
       Modus setzen
       ------------------------------------------------------------------------
       Beschreibung:
       - Wählt den gewünschten Modus für einen Ausgang vor
       - Schaltet den Ausgang NICHT ein
       - Falls Ausgang aktiv war → wird ausgeschaltet
       ------------------------------------------------------------------------ */
    void setMode(OutputID output, OutputMode mode);

    /* ------------------------------------------------------------------------
       Ausgang einschalten
       ------------------------------------------------------------------------
       Beschreibung:
       - Startet eine Safety-Wartezeit
       - Erst nach Ablauf wird der Ausgang hardwareseitig aktiviert
       ------------------------------------------------------------------------ */
    void turnOn(OutputID output);

    /* ------------------------------------------------------------------------
       Ausgang ausschalten
       ------------------------------------------------------------------------
       Beschreibung:
       - Schaltet den Ausgang sofort hardwareseitig AUS
       - Bricht ggf. laufende Impulse ab
       ------------------------------------------------------------------------ */
    void turnOff(OutputID output);

    /* ------------------------------------------------------------------------
       Impuls starten
       ------------------------------------------------------------------------
       Beschreibung:
       - Schaltet den Ausgang EIN
       - Nach Ablauf der Impulszeit wird automatisch ausgeschaltet
       ------------------------------------------------------------------------ */
    void startImpulse(OutputID output);

    /* ------------------------------------------------------------------------
       Status-Abfragen
       ------------------------------------------------------------------------ */
    OutputMode getMode(OutputID output) const;
    bool       isEnabled(OutputID output) const;
    bool       isImpulseActive(OutputID output) const;

    /* ------------------------------------------------------------------------
       Sammelbefehle
       ------------------------------------------------------------------------ */
    void allOn();
    void allOff();

private:
    /* Hardware */
    McpManager _mcp;

    /* Zustände */
    OutputMode _mode[OUTPUT_COUNT];
    bool       _enabled[OUTPUT_COUNT];

    /* Safety-Handling */
    bool       _safetyPending[OUTPUT_COUNT];
    uint32_t   _safetyStartMs[OUTPUT_COUNT];

    /* Impuls-Handling */
    bool       _impulseActive[OUTPUT_COUNT];
    uint32_t   _impulseStartMs[OUTPUT_COUNT];
};
