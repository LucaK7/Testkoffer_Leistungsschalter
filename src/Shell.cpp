/* ============================================================================
   Datei:   Shell.cpp
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Umsetzung der seriellen Kommandozeile (siehe Befehlsliste in Shell.h).

   Ablauf je loop()-Durchlauf
   ----------------------------------------------------------------------------
   1. readInput():    vorhandene Zeichen lesen; bei Enter Befehl ausführen
   2. reportEvents(): neue Ereignisse von McpManager, SelfTest und
                      StateManager ausgeben
   Beides ist nicht blockierend.
   ============================================================================ */

#include "Shell.h"
#include "Config.h"

Shell::Shell(StateManager& state, SelfTest& selftest, McpManager& mcp, DisplayUi& display)
    : _state(state), _selftest(selftest), _mcp(mcp), _display(display)
{
}

void Shell::begin()
{
    _line.reserve(64);
    _seenMcpEvent = _mcp.eventId();
    _seenTestStep = _selftest.stepId();
    _seenFault    = _state.isFault();

    Serial.println();
    Serial.println(F("SE Testkoffer - Steuerung V55 (Grundaufbau)"));
    Serial.print  (F("I2C: "));
    Serial.println(_mcp.lastEvent());
    Serial.println(F("'hilfe' zeigt alle Befehle."));
    prompt();
}

void Shell::update()
{
    readInput();
    reportEvents();
}

/* ============================================================================
   Eingabe
   ============================================================================ */

/* ----------------------------------------------------------------------------
   readInput()
   Sammelt Zeichen bis Enter (CR oder LF). Rückschritt löscht das letzte
   Zeichen. Eingaben werden zurückgesendet (Echo), damit sie im Terminal
   sichtbar sind.
   ---------------------------------------------------------------------------- */
void Shell::readInput()
{
    while (Serial.available())
    {
        const char c = static_cast<char>(Serial.read());

        if (c == '\r' || c == '\n')
        {
            if (_line.length() > 0)
            {
                Serial.println();
                execute(_line);
                _line = "";
                prompt();
            }
        }
        else if (c == '\b' || c == 0x7F)
        {
            if (_line.length() > 0)
            {
                _line.remove(_line.length() - 1);
                Serial.print(F("\b \b"));
            }
        }
        else if (c >= 32 && c <= 126 && _line.length() < 60)
        {
            _line += c;
            Serial.print(c);
        }
    }
}

/* ----------------------------------------------------------------------------
   execute()
   Zerlegt die Zeile in Befehl und bis zu zwei Argumente (getrennt durch
   Leerzeichen, Gross-/Kleinschreibung egal) und ruft den passenden Befehl auf.
   ---------------------------------------------------------------------------- */
void Shell::execute(const String& line)
{
    String text = line;
    text.trim();
    text.toLowerCase();

    String parts[3];
    uint8_t count = 0;
    int start = 0;
    while (count < 3 && start < static_cast<int>(text.length()))
    {
        int space = text.indexOf(' ', start);
        if (space < 0) space = text.length();
        if (space > start)
        {
            parts[count++] = text.substring(start, space);
        }
        start = space + 1;
    }

    const String& cmd = parts[0];
    if      (cmd == "hilfe" || cmd == "help" || cmd == "?") cmdHelp();
    else if (cmd == "status")                               cmdStatus();
    else if (cmd == "ein")                                  cmdOn(parts[1]);
    else if (cmd == "aus")                                  cmdOff(parts[1]);
    else if (cmd == "spg")                                  cmdVoltage(parts[1], parts[2]);
    else if (cmd == "ansicht")                              cmdView(parts[1]);
    else if (cmd == "selbsttest")                           cmdSelfTest(parts[1]);
    else if (cmd == "fehler")                               cmdErrors(parts[1]);
    else if (cmd == "kalibrieren")                          cmdCalibrate();
    else
    {
        Serial.println(F("Unbekannter Befehl ('hilfe' zeigt alle Befehle)"));
    }
}

/* ============================================================================
   Ereignismeldungen
   ============================================================================ */

/* ----------------------------------------------------------------------------
   reportEvents()
   Jede Komponente zählt bei einem neuen Ereignis einen Zähler hoch. Weicht
   dieser vom zuletzt gesehenen Wert ab, wird das Ereignis ausgegeben.
   ---------------------------------------------------------------------------- */
void Shell::reportEvents()
{
    if (_mcp.eventId() != _seenMcpEvent)
    {
        _seenMcpEvent = _mcp.eventId();
        Serial.print(F("\r\n[I2C]  "));
        Serial.println(_mcp.lastEvent());
        prompt();
    }

    if (_selftest.stepId() != _seenTestStep)
    {
        _seenTestStep = _selftest.stepId();
        Serial.print(F("\r\n[TEST] "));
        Serial.println(_selftest.stepText());
        prompt();
    }

    // Prüfkabel / Ansicht (R8): Änderungen melden, egal ob sie vom Display
    // oder von der Shell ausgelöst wurden.
    const uint8_t cable = _state.viewChangePending() ? 1 : (_state.cableConfirmed() ? 2 : 0);
    if (cable != _seenCable || _state.view() != _seenView)
    {
        if (cable == 1)
        {
            Serial.print(F("\r\n[KABEL] Ansichtswechsel auf "));
            Serial.print(viewName(_state.pendingView()));
            Serial.println(F(" - alle Ausgaenge AUS. Passendes Pruefkabel"));
            Serial.println(F("        einstecken und bestaetigen (Display oder 'ansicht ja')."));
        }
        else if (cable == 2)
        {
            Serial.print(F("\r\n[KABEL] Ansicht "));
            Serial.print(viewName(_state.view()));
            Serial.println(F(" aktiv - Einschalten freigegeben"));
        }
        else
        {
            Serial.println(F("\r\n[KABEL] Pruefkabel nicht bestaetigt - Einschalten gesperrt."));
            Serial.println(F("        Kabel auf dem Display waehlen oder 'ansicht mtz|nsx', dann 'ansicht ja'."));
        }
        prompt();
        _seenCable = cable;
        _seenView  = _state.view();
    }

    if (_state.isFault() != _seenFault)
    {
        _seenFault = _state.isFault();
        Serial.println(_seenFault
            ? F("\r\n[STOERUNG] I/O nicht erreichbar - alle Ausgaenge AUS, Einschalten gesperrt")
            : F("\r\n[OK] I/O wieder erreichbar - Ausgaenge bleiben AUS"));
        prompt();
    }
}

/* ============================================================================
   Befehle
   ============================================================================ */

void Shell::cmdHelp()
{
    Serial.println(F("Ausgang 1 = MN/MX2, 2 = XF, 3 = MX, 4 = MCH"));
    Serial.println(F("  status                       Zustand aller Ausgaenge"));
    Serial.println(F("  ein <1-4>                    Ausgang einschalten"));
    Serial.println(F("  aus <1-4|alle>               Ausgang / alle ausschalten"));
    Serial.println(F("  spg <1-4|alle> <24|48|230>   Spannung waehlen"));
    Serial.println(F("  ansicht <mtz|nsx>            Ansichtswechsel anfordern (alles AUS)"));
    Serial.println(F("  ansicht ja|nein              Pruefkabel bestaetigen (10 s) / abbrechen"));
    Serial.println(F("  selbsttest start|ja|stop     Relaistest - NUR OHNE PRUEFLING!"));
    Serial.println(F("  fehler [reset]               I2C-Fehlerstatistik"));
    Serial.println(F("  kalibrieren                  Touch neu kalibrieren (Neustart)"));
}

/* ----------------------------------------------------------------------------
   cmdStatus()
   Tabelle aller Ausgänge in der aktuell gewählten Ansicht.
   «wartet» = Einschalten angefordert, Sicherheitspause läuft noch.
   ---------------------------------------------------------------------------- */
void Shell::cmdStatus()
{
    Serial.print(F("Ansicht: "));
    Serial.print(viewName(_state.view()));
    if (_state.viewChangePending())
    {
        Serial.print(F("   Wechsel auf "));
        Serial.print(viewName(_state.pendingView()));
        Serial.println(F(" offen - Pruefkabel bestaetigen"));
    }
    else
    {
        Serial.println(_state.cableConfirmed() ? F("   Pruefkabel bestaetigt")
                                               : F("   Pruefkabel NICHT bestaetigt - Einschalten gesperrt"));
    }
    Serial.println(F("Nr Relais Kurzz.  Funktion          Spannung Zustand"));

    char row[80];
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        const OutputLabel label = outputLabel(_state.view(), out);
        const char* stateText = _state.isOn(out)        ? "EIN"
                              : _state.isRequested(out) ? "wartet"
                                                        : "AUS";
        snprintf(row, sizeof(row), "%u  %-6s %-7s %-17s %-8s %s%s",
                 out + 1, relayPin(out, _state.voltage(out)).name, label.code, label.function,
                 voltageName(_state.voltage(out)), stateText,
                 _state.isCoupled(out) ? "  (gekoppelt)" : "");
        Serial.println(row);
    }

    Serial.print(F("I/O: "));
    Serial.print(_state.isFault() ? F("STOERUNG") : F("OK"));
    Serial.print(F("   Fehler: "));
    Serial.print(_mcp.errorCount());
    Serial.print(F("   Reparaturen: "));
    Serial.println(_mcp.repairCount());
}

void Shell::cmdOn(const String& arg)
{
    if (rejectDuringSelfTest()) return;

    const int8_t out = parseOutput(arg);
    if (out < 0)
    {
        Serial.println(F("Ausgang 1-4 angeben, z. B. 'ein 3'"));
        return;
    }
    if (!_state.cableConfirmed())
    {
        Serial.println(F("Abgelehnt: Pruefkabel nicht bestaetigt ('ansicht mtz|nsx', dann 'ansicht ja')"));
        return;
    }
    if (!_state.requestOn(static_cast<uint8_t>(out)))
    {
        Serial.println(F("Abgelehnt: I/O-Stoerung"));
        return;
    }
    Serial.println(F("OK"));
}

/* ----------------------------------------------------------------------------
   cmdOff()
   Ausschalten ist immer erlaubt; ein laufender Selbsttest wird dabei
   abgebrochen.
   ---------------------------------------------------------------------------- */
void Shell::cmdOff(const String& arg)
{
    if (_selftest.isRunning())
    {
        _selftest.stop();
    }

    if (arg == "alle")
    {
        _state.allOff();
        Serial.println(F("OK, alle AUS"));
        return;
    }

    const int8_t out = parseOutput(arg);
    if (out < 0)
    {
        Serial.println(F("Ausgang 1-4 oder 'alle' angeben"));
        return;
    }
    _state.requestOff(static_cast<uint8_t>(out));
    Serial.println(F("OK"));
}

void Shell::cmdVoltage(const String& target, const String& value)
{
    if (rejectDuringSelfTest()) return;

    Voltage v;
    if (!parseVoltage(value, v))
    {
        Serial.println(F("Spannung 24, 48 oder 230 angeben, z. B. 'spg 2 48'"));
        return;
    }

    if (target == "alle")
    {
        _state.setVoltageAll(v);
    }
    else
    {
        const int8_t out = parseOutput(target);
        if (out < 0)
        {
            Serial.println(F("Ausgang 1-4 oder 'alle' angeben"));
            return;
        }
        _state.setVoltage(static_cast<uint8_t>(out), v);
        if (_state.isCoupled(static_cast<uint8_t>(out)))
        {
            Serial.println(F("Hinweis: NSX - A4, A2 und B4 gekoppelt"));
        }
    }
    Serial.println(F("OK (eingeschaltete Ausgaenge mit geaenderter Spannung sind jetzt AUS)"));
}

/* ----------------------------------------------------------------------------
   cmdView()
   Zweistufig wie beim Selbsttest (R8): «ansicht mtz|nsx» schaltet alles AUS
   und fragt nach dem Prüfkabel, «ansicht ja» bestätigt innerhalb von 10 s,
   «ansicht nein» bricht ab. Die Meldungen dazu gibt reportEvents() aus.
   ---------------------------------------------------------------------------- */
void Shell::cmdView(const String& arg)
{
    if (rejectDuringSelfTest()) return;

    if (arg == "mtz" || arg == "nsx")
    {
        _state.requestViewChange(arg == "nsx" ? View::NSX : View::MTZ_NW_NT,
                                 Timing::VIEW_CONFIRM_MS);
        Serial.println(F("Bestaetigen mit 'ansicht ja' (10 s), abbrechen mit 'ansicht nein'."));
    }
    else if (arg == "ja")
    {
        if (!_state.confirmViewChange())
        {
            Serial.println(F("Kein Wechsel offen: zuerst 'ansicht mtz' oder 'ansicht nsx'"));
        }
    }
    else if (arg == "nein")
    {
        _state.cancelViewChange();
    }
    else
    {
        Serial.println(F("'ansicht mtz|nsx', danach 'ansicht ja' bzw. 'ansicht nein'"));
    }
}

/* ----------------------------------------------------------------------------
   cmdSelfTest()
   Zweistufiger Start (start -> ja) als Schutz gegen versehentliches
   Anlegen von 230 V an einen angeschlossenen Prüfling.
   ---------------------------------------------------------------------------- */
void Shell::cmdSelfTest(const String& arg)
{
    if (arg == "start")
    {
        if (_selftest.isRunning())
        {
            Serial.println(F("Selbsttest laeuft bereits"));
            return;
        }
        _selftest.requestStart();
        Serial.println(F("WARNUNG: Alle Ausgaenge erhalten nacheinander 24 V, 48 V und 230 V!"));
        Serial.println(F("Pruefling und Testkabel abhaengen. Bestaetigen mit 'selbsttest ja' (10 s)."));
    }
    else if (arg == "ja")
    {
        if (!_selftest.confirmStart())
        {
            Serial.println(F("Kein Start: zuerst 'selbsttest start' (oder I/O-Stoerung)"));
        }
    }
    else if (arg == "stop")
    {
        _selftest.stop();
    }
    else
    {
        Serial.println(F("'selbsttest start', 'selbsttest ja' oder 'selbsttest stop'"));
    }
}

void Shell::cmdErrors(const String& arg)
{
    if (arg == "reset")
    {
        _mcp.resetStatistics();
        return;
    }
    Serial.print(F("Erkannte I2C-Fehler/Abweichungen: "));
    Serial.println(_mcp.errorCount());
    Serial.print(F("Erfolgreiche Reparaturen:         "));
    Serial.println(_mcp.repairCount());
    Serial.print(F("Letztes Ereignis: "));
    Serial.println(_mcp.lastEvent());
}

void Shell::cmdCalibrate()
{
    Serial.println(F("Neustart - danach auf dem Display die vier Ecken antippen."));
    Serial.flush();
    _display.scheduleCalibration();   // kehrt nicht zurück (Neustart)
}

/* ============================================================================
   Hilfsfunktionen
   ============================================================================ */

int8_t Shell::parseOutput(const String& s)
{
    if (s.length() != 1) return -1;
    const char c = s.charAt(0);
    if (c < '1' || c > '4') return -1;
    return static_cast<int8_t>(c - '1');
}

bool Shell::parseVoltage(const String& s, Voltage& v)
{
    String t = s;
    if (t.endsWith("v")) t.remove(t.length() - 1);   // «48v» ebenfalls zulassen
    if (t == "24")  { v = Voltage::V24;  return true; }
    if (t == "48")  { v = Voltage::V48;  return true; }
    if (t == "230") { v = Voltage::V230; return true; }
    return false;
}

bool Shell::rejectDuringSelfTest()
{
    if (_selftest.isRunning())
    {
        Serial.println(F("Selbsttest laeuft - zuerst 'selbsttest stop' oder 'aus alle'"));
        return true;
    }
    return false;
}

void Shell::prompt()
{
    Serial.print(F("> "));
    if (_line.length() > 0)
    {
        Serial.print(_line);   // angefangene Eingabe nach einer Meldung wieder anzeigen
    }
}
