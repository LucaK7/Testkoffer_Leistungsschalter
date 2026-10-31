#pragma once

/* ============================================================================
   Datei: McpManager.h

   Funktion:
   ---------
   Hardware-Abstraktion für zwei Adafruit MCP23008 I/O-Expander.
   Diese Klasse ist die EINZIGE Stelle im Projekt, die direkt
   MCP-Pins beschreibt.

   WICHTIGE GRUNDSÄTZE:
   -------------------
   👉 Keine Zustandslogik (kein Merken von Modus / Ein / Aus)
   👉 Keine Timing- oder Safety-Logik
   👉 Kein Wissen über Shell, StateManager oder SelfTest
   👉 Schreibt IMMER deterministisch alle relevanten Pins

   KONZEPT:
   --------
   - Jeder logische Ausgang besteht aus 4 Pins (MODE_24V..MODE_CUSTOM)
   - Beim Ausschalten werden IMMER alle 4 Pins LOW gesetzt
   - Beim Einschalten werden ALLE 4 Pins geschrieben:
       - gewünschter Modus = HIGH
       - alle anderen Modi = LOW
   - Dadurch: kein Flackern, kein undefinierter Zustand

   ABHÄNGIGKEITEN:
   ---------------
   - Outputs.h (OutputID, OutputMode, MODE_COUNT, OUTPUT_COUNT)
   - Adafruit MCP23008 Library

   ============================================================================ */

#include <Arduino.h>
#include <Adafruit_MCP23008.h>
#include "Outputs.h"

class McpManager
{
public:
    /* ------------------------------------------------------------------------
       Initialisierung
       ------------------------------------------------------------------------
       Beschreibung:
       - Initialisiert beide MCP23008 (0x20, 0x21)
       - Setzt alle Pins als OUTPUT
       - Schaltet beim Start ALLE Pins LOW (Hard-Reset-Zustand)
       ------------------------------------------------------------------------ */
    void begin();

    /* ------------------------------------------------------------------------
       Alle Ausgänge ausschalten (Global)
       ------------------------------------------------------------------------
       Beschreibung:
       - Schaltet ALLE Pins beider MCPs LOW
       - Wird typischerweise beim Systemstart verwendet
       - Keine Rücksicht auf Modi oder Logik
       ------------------------------------------------------------------------ */
    void allOutputsOff();

    /* ------------------------------------------------------------------------
       Ausgang einschalten (mit Modus)
       ------------------------------------------------------------------------
       Beschreibung:
       - Schaltet GENAU EINEN Modus eines Ausgangs ein
       - Alle anderen Modi dieses Ausgangs werden explizit ausgeschaltet
       - Andere Ausgänge bleiben unverändert

       Ablauf:
       1) Alle 4 Pins dieses Ausgangs auf LOW
       2) Gewählten Modus-Pin auf HIGH
       ------------------------------------------------------------------------ */
    void enableOutput(OutputID output, OutputMode mode);

    /* ------------------------------------------------------------------------
       Ausgang ausschalten
       ------------------------------------------------------------------------
       Beschreibung:
       - Schaltet ALLE 4 Pins des angegebenen Ausgangs LOW
       - Andere Ausgänge bleiben unverändert
       ------------------------------------------------------------------------ */
    void disableOutput(OutputID output);

private:
    /* MCP-Instanzen */
    Adafruit_MCP23008 _mcp1;
    Adafruit_MCP23008 _mcp2;

    /* ------------------------------------------------------------------------
       Pin-Auflösung
       ------------------------------------------------------------------------
       Beschreibung:
       - Ordnet einem logischen Ausgang + Modus
         den korrekten MCP und Pin zu

       Mapping:
       - OUT_MN_MX2 / OUT_MX  → MCP1
       - OUT_XF     / OUT_MCH → MCP2
       - Jeder Ausgang belegt 4 Pins (MODE_24V..MODE_CUSTOM)

       Rückgabe:
       - pin       → MCP-Pin 0..7
       - useMcp1   → true = MCP1, false = MCP2
       ------------------------------------------------------------------------ */
    uint8_t resolvePin(OutputID output, OutputMode mode, bool& useMcp1);
};
