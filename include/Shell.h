/* ============================================================================
   Datei:   Shell.h
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Serielle Kommandozeile über USB (115200 Baud, z. B. PlatformIO «Serial
   Monitor» oder PowerShell). Dient dem Service: Zustand abfragen, Ausgänge
   schalten, Selbsttest, Fehlerstatistik, Touch neu kalibrieren.

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Zeichen von der seriellen Schnittstelle sammeln und Befehle ausführen
   - Befehle an StateManager, SelfTest, McpManager bzw. DisplayUi weitergeben
   - Ereignisse (I²C-Störungen, Selbsttest-Schritte, Störungszustand)
     unaufgefordert melden

   Befehle (Ausgang 1 = MN/MX2, 2 = XF, 3 = MX, 4 = MCH)
   ----------------------------------------------------------------------------
     hilfe                         Befehlsübersicht
     status                        Zustand aller Ausgänge
     ein <1-4>                     Ausgang einschalten
     aus <1-4|alle>                Ausgang bzw. alle ausschalten
     spg <1-4|alle> <24|48|230>    Spannung wählen
     ansicht <mtz|nsx>             Ansichtswechsel anfordern (alles AUS)
     ansicht ja|nein               Prüfkabel bestätigen (10 s) bzw. abbrechen
     selbsttest start|ja|stop      Relaistest (nur ohne Prüfling!)
     fehler [reset]                I²C-Fehlerstatistik anzeigen/zurücksetzen
     kalibrieren                   Touch neu kalibrieren (Neustart)

   Wichtige Hinweise
   ----------------------------------------------------------------------------
   - Die Shell schaltet nie selbst Relais, sie ruft dieselben Funktionen des
     StateManagers auf wie das Display. Es gelten dieselben Sicherheitsregeln.
   - Während des Selbsttests sind Schaltbefehle gesperrt; «aus» bricht den
     Selbsttest ab und schaltet alles aus.
   - Ist die USB-Verbindung am PC, ist die Steuer-GND über den PC mit dem
     Schutzleiter verbunden. Für EMV-Tests deshalb ohne USB arbeiten.
   ============================================================================ */

#pragma once
#include <Arduino.h>
#include "StateManager.h"
#include "SelfTest.h"
#include "McpManager.h"
#include "DisplayUi.h"

class Shell
{
public:
    Shell(StateManager& state, SelfTest& selftest, McpManager& mcp, DisplayUi& display);

    void begin();    // Begrüssung und Eingabeaufforderung
    void update();   // Muss in jedem loop()-Durchlauf aufgerufen werden

private:
    void readInput();
    void execute(const String& line);
    void reportEvents();

    // Befehle
    void cmdHelp();
    void cmdStatus();
    void cmdOn(const String& arg);
    void cmdOff(const String& arg);
    void cmdVoltage(const String& target, const String& value);
    void cmdView(const String& arg);
    void cmdSelfTest(const String& arg);
    void cmdErrors(const String& arg);
    void cmdCalibrate();

    // Hilfsfunktionen
    static int8_t parseOutput(const String& s);              // "1".."4" -> 0..3, sonst -1
    static bool   parseVoltage(const String& s, Voltage& v);
    bool          rejectDuringSelfTest();
    void          prompt();

    StateManager& _state;
    SelfTest&     _selftest;
    McpManager&   _mcp;
    DisplayUi&    _display;

    String   _line;
    uint32_t _seenMcpEvent  = 0;
    uint32_t _seenTestStep  = 0;
    bool     _seenFault     = false;
    uint8_t  _seenCable     = 0xFF;   // 0 = nicht bestätigt, 1 = Wechsel offen, 2 = bestätigt
    View     _seenView      = View::MTZ_NW_NT;
};
