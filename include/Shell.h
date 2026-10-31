#pragma once

#include <Arduino.h>
#include "StateManager.h"
#include "SelfTest.h"

/* ============================================================================
   Datei: Shell.h

   Funktion:
   ---------
   Einfache serielle Kommando-Shell zur Steuerung der Ausgänge.

   Konzept:
   --------
   - Klare, stabile Befehle
   - Sichtbare Eingabe
   - Tabellenbasierte Statusanzeige
   - Keine Autovervollständigung (bewusst einfach)
   ============================================================================ */

class Shell
{
public:
    void begin(StateManager* state, SelfTest* selftest);
    void update();

    /* Status */
    void printFullStatus();
    void printTableHeader();
    void printTableRow(OutputID out);
    void printTableFooter();

    void printPrompt();

private:
    StateManager* _state    = nullptr;
    SelfTest*     _selftest = nullptr;

    String _line;

    /* Core */
    void handleCommand(const String& cmd);

    /* Commands */
    void cmdHelp();
    void cmdOn(const String& arg);
    void cmdOff(const String& arg);
    void cmdImpulse(const String& arg);
    void cmdMode(const String& outTxt, const String& modeTxt);
    void cmdAll(bool on);

    /* Helpers */
    void pad(int count);

    /* Parser */
    bool parseOutput(const String& txt, OutputID& out);
    bool parseMode(const String& txt, OutputMode& mode);
};
