/* ============================================================================
   Datei:   StateManager.cpp
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Umsetzung der Sicherheitsregeln R1–R7 (siehe StateManager.h).

   Grundprinzip: Befehle ändern nur den gewünschten Zustand («requested»).
   Ausschalten wird sofort an die Hardware weitergegeben, Einschalten erst in
   update(), sobald alle Bedingungen (Sicherheitspause, Verriegelung, keine
   Störung) erfüllt sind. Dadurch gibt es genau EINE Stelle, an der ein Relais
   eingeschaltet wird – das macht die Regeln leicht überprüfbar.
   ============================================================================ */

#include "StateManager.h"
#include "Config.h"

StateManager::StateManager(McpManager& mcp)
    : _mcp(mcp)
{
}

/* ----------------------------------------------------------------------------
   begin()
   Setzt den definierten Startzustand. Die Hardware ist zu diesem Zeitpunkt
   bereits durch McpManager::begin() auf «alle Relais aus» gesetzt.
   ---------------------------------------------------------------------------- */
void StateManager::begin()
{
    const uint32_t now = millis();

    _view = View::MTZ_NW_NT;
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        _out[out].voltage    = Voltage::V24;   // niedrigste Spannung als Vorgabe
        _out[out].requested  = false;
        _out[out].relayOn    = false;
        _out[out].offSinceMs = now;
    }

    // War die Hardware schon beim Start nicht erreichbar, direkt in den
    // Störungszustand: Einschalten bleibt gesperrt (R7).
    _fault       = !_mcp.isHealthy();
    _lastCheckMs = now;
    changed();
}

/* ----------------------------------------------------------------------------
   update()
   1. Gewünschte Ausgänge einschalten, sobald dies erlaubt ist (R2, R3, R7).
   2. Hardware zyklisch kontrollieren bzw. im Störungszustand versuchen,
      sie wieder zu erreichen.
   ---------------------------------------------------------------------------- */
void StateManager::update()
{
    const uint32_t now = millis();

    // --- 1. Verzögertes Einschalten -----------------------------------------
    bool hardwareChanged = false;
    if (!_fault)
    {
        for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
        {
            OutputState& o = _out[out];
            if (o.requested && !o.relayOn && pauseElapsed(out, now))
            {
                o.relayOn = true;            // einzige Stelle, an der ein Relais EIN geht
                hardwareChanged = true;
            }
        }
    }
    if (hardwareChanged)
    {
        applyToHardware();
        changed();
    }

    // --- Zeitlimit eines über die Shell angeforderten Ansichtswechsels -------
    if (_pending && _pendingUntil != 0 && static_cast<int32_t>(now - _pendingUntil) >= 0)
    {
        cancelViewChange();
    }

    // --- 2. Kontrolle bzw. Wiederanlauf nach Störung ------------------------
    if (!_fault)
    {
        if (now - _lastCheckMs >= Timing::MCP_CHECK_INTERVAL_MS)
        {
            _lastCheckMs = now;
            handleResult(_mcp.check());
        }
    }
    else
    {
        if (now - _lastCheckMs >= Timing::FAULT_RETRY_MS)
        {
            _lastCheckMs = now;
            // Versuch, den sicheren Zustand «alles aus» zu schreiben. Gelingt
            // das, ist die Hardware wieder in Ordnung. Die Ausgänge bleiben
            // AUS, bis der Techniker sie bewusst wieder einschaltet.
            if (_mcp.writeOutputs(0x00, 0x00) != McpManager::Result::FAILED)
            {
                _fault = false;
                changed();
            }
        }
    }
}

/* ============================================================================
   Befehle
   ============================================================================ */

/* ----------------------------------------------------------------------------
   requestOn()
   Merkt den Einschaltwunsch vor. Verriegelung (R3): Ist die Partnerspule
   gewünscht oder EIN, wird sie sofort ausgeschaltet. Das tatsächliche
   Einschalten erfolgt in update().
   ---------------------------------------------------------------------------- */
bool StateManager::requestOn(uint8_t out)
{
    if (out >= OUTPUT_COUNT || _fault || !cableConfirmed())   // R7, R8
    {
        return false;
    }

    const int8_t partner = interlockPartner(out);
    if (partner >= 0 && (_out[partner].requested || _out[partner].relayOn))
    {
        requestOff(static_cast<uint8_t>(partner));
    }

    _out[out].requested = true;
    changed();
    return true;
}

/* ----------------------------------------------------------------------------
   requestOff()
   Ausschalten wirkt sofort und ohne Bedingungen.
   ---------------------------------------------------------------------------- */
void StateManager::requestOff(uint8_t out)
{
    if (out >= OUTPUT_COUNT)
    {
        return;
    }

    _out[out].requested = false;
    if (_out[out].relayOn)
    {
        switchRelayOff(out);
        applyToHardware();
    }
    changed();
}

void StateManager::toggle(uint8_t out)
{
    if (out >= OUTPUT_COUNT)
    {
        return;
    }
    if (_out[out].requested || _out[out].relayOn)
    {
        requestOff(out);
    }
    else
    {
        requestOn(out);
    }
}

/* ----------------------------------------------------------------------------
   setVoltage()
   In der Ansicht NSX gilt die Kopplung (R5): Eine Änderung an A4 (MX),
   A2 (XF) oder B4 (MCH) wird auf alle drei übertragen.
   ---------------------------------------------------------------------------- */
void StateManager::setVoltage(uint8_t out, Voltage v)
{
    if (out >= OUTPUT_COUNT)
    {
        return;
    }

    if (isCoupled(out))
    {
        setVoltageSingle(OUT_MX, v);
        setVoltageSingle(OUT_XF, v);
        setVoltageSingle(OUT_MCH, v);
    }
    else
    {
        setVoltageSingle(out, v);
    }
}

void StateManager::setVoltageAll(Voltage v)
{
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        setVoltageSingle(out, v);
    }
}

void StateManager::allOff()
{
    bool hardwareChanged = false;
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        _out[out].requested = false;
        if (_out[out].relayOn)
        {
            switchRelayOff(out);
            hardwareChanged = true;
        }
    }
    if (hardwareChanged)
    {
        applyToHardware();
    }
    changed();
}

/* ----------------------------------------------------------------------------
   Ansicht und Prüfkabel (R6, R8)
   ----------------------------------------------------------------------------
   Ablauf beim Wechsel der Ansicht:
     requestViewChange()  -> alles AUS, Einschalten gesperrt, Kabel umstecken
     confirmViewChange()  -> neue Ansicht gilt (R6), Einschalten wieder frei
     cancelViewChange()   -> alte Ansicht bleibt, Ausgänge bleiben AUS
   Beim Start und nach dem Selbsttest ist noch kein Kabel bestätigt; die
   Auswahl auf dem Startbildschirm ruft chooseView() auf.
   ---------------------------------------------------------------------------- */
void StateManager::requestViewChange(View v, uint32_t timeoutMs)
{
    allOff();
    _pending      = true;
    _pendingView  = v;
    _pendingUntil = 0;
    if (timeoutMs != 0)
    {
        _pendingUntil = millis() + timeoutMs;
        if (_pendingUntil == 0) _pendingUntil = 1;   // 0 bedeutet «kein Zeitlimit»
    }
    changed();
}

bool StateManager::confirmViewChange()
{
    if (!_pending)
    {
        return false;
    }
    chooseView(_pendingView);
    return true;
}

void StateManager::cancelViewChange()
{
    if (!_pending)
    {
        return;
    }
    _pending = false;   // alte Ansicht und alte Bestätigung bleiben
    changed();
}

void StateManager::chooseView(View v)
{
    applyView(v);
    _pending = false;
    _cableOk = true;
    changed();
}

void StateManager::requireCableConfirmation()
{
    allOff();
    _pending = false;
    _cableOk = false;
    changed();
}

/* ----------------------------------------------------------------------------
   applyView()
   R6: alle Ausgänge AUS und alle Spannungen auf 24 V, wie beim Start –
   auch wenn die Ansicht gleich bleibt. Damit ist die Kopplung (R5) in der
   Ansicht NSX von Anfang an erfüllt, und vor dem Prüfen muss die Spannung
   bewusst gewählt werden.
   ---------------------------------------------------------------------------- */
void StateManager::applyView(View v)
{
    allOff();
    _view = v;

    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        setVoltageSingle(out, Voltage::V24);
    }
    changed();
}

/* ============================================================================
   Abfragen
   ============================================================================ */

bool StateManager::anyOn() const
{
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        if (_out[out].requested || _out[out].relayOn)
        {
            return true;
        }
    }
    return false;
}

bool StateManager::allSameVoltage(Voltage& common) const
{
    common = _out[0].voltage;
    for (uint8_t out = 1; out < OUTPUT_COUNT; out++)
    {
        if (_out[out].voltage != common)
        {
            return false;
        }
    }
    return true;
}

bool StateManager::isCoupled(uint8_t out) const
{
    return (_view == View::NSX) && (out == OUT_MX || out == OUT_XF || out == OUT_MCH);
}

/* ============================================================================
   Interne Hilfsfunktionen
   ============================================================================ */

/* ----------------------------------------------------------------------------
   pauseElapsed()
   R2: Eigene Sicherheitspause abgelaufen?
   R3: Ist die Partnerspule AUS und ebenfalls seit mindestens 200 ms?
   ---------------------------------------------------------------------------- */
bool StateManager::pauseElapsed(uint8_t out, uint32_t now) const
{
    if (now - _out[out].offSinceMs < Timing::SAFETY_PAUSE_MS)
    {
        return false;
    }

    const int8_t partner = interlockPartner(out);
    if (partner >= 0)
    {
        const OutputState& p = _out[partner];
        if (p.relayOn || (now - p.offSinceMs < Timing::SAFETY_PAUSE_MS))
        {
            return false;
        }
    }
    return true;
}

// Liefert den Verriegelungspartner (MX <-> XF) oder -1.
int8_t StateManager::interlockPartner(uint8_t out) const
{
    if (out == OUT_COIL_OFF) return OUT_COIL_ON;
    if (out == OUT_COIL_ON)  return OUT_COIL_OFF;
    return -1;
}

void StateManager::switchRelayOff(uint8_t out)
{
    _out[out].relayOn    = false;
    _out[out].offSinceMs = millis();   // Startpunkt der Sicherheitspause (R2)
}

/* ----------------------------------------------------------------------------
   setVoltageSingle()
   R4: Ändert sich die Spannung eines eingeschalteten Ausgangs, wird er
   zuerst ausgeschaltet. Die Hardware erhält das Ausschalten mit der ALTEN
   Spannung, danach wird die neue Spannung nur noch vorgemerkt.
   ---------------------------------------------------------------------------- */
void StateManager::setVoltageSingle(uint8_t out, Voltage v)
{
    OutputState& o = _out[out];
    if (o.voltage == v)
    {
        return;
    }

    o.requested = false;
    if (o.relayOn)
    {
        switchRelayOff(out);
        applyToHardware();
    }
    o.voltage = v;
    changed();
}

/* ----------------------------------------------------------------------------
   applyToHardware()
   Übersetzt den Zustand in die zwei Steuerbytes und übergibt sie dem
   McpManager. Welches Bit in welchem Byte zu einem Ausgang und seiner
   Spannung gehört, liefert die Tabelle relayPin() (Outputs.h).
   Beispiel: MCH mit 24 V -> Relais K3 -> MCP -A3/U16 (Byte 1), GP4 (Bit 4) -> J12 Pin 1.
   R1 ist durch diese Bildung automatisch erfüllt (pro Ausgang genau ein
   Bit), R3 wird hier zusätzlich nochmals abgesichert.
   ---------------------------------------------------------------------------- */
void StateManager::applyToHardware()
{
    // Zusätzliche Absicherung R3: beide Spulen EIN darf nie vorkommen.
    if (_out[OUT_COIL_OFF].relayOn && _out[OUT_COIL_ON].relayOn)
    {
        _out[OUT_COIL_OFF].requested = false;
        _out[OUT_COIL_ON].requested  = false;
        switchRelayOff(OUT_COIL_OFF);
        switchRelayOff(OUT_COIL_ON);
    }

    uint8_t bits[2] = { 0x00, 0x00 };
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        if (_out[out].relayOn)
        {
            const RelayPin r = relayPin(out, _out[out].voltage);
            bits[r.chip] |= static_cast<uint8_t>(1u << r.pin);
        }
    }

    handleResult(_mcp.writeOutputs(bits[0], bits[1]));
}

/* ----------------------------------------------------------------------------
   handleResult()
   OK/REPAIRED: weiterarbeiten (eine Reparatur stellt den Sollzustand wieder
   her, der Prüfling merkt davon nichts). FAILED: Störungszustand (R7).
   ---------------------------------------------------------------------------- */
void StateManager::handleResult(McpManager::Result r)
{
    if (r == McpManager::Result::FAILED)
    {
        enterFault();
    }
    else if (r == McpManager::Result::REPAIRED)
    {
        changed();   // Display/Shell können die Fehlerstatistik aktualisieren
    }
}

/* ----------------------------------------------------------------------------
   enterFault()
   R7: Alle Ausgänge logisch AUS und – soweit der Bus noch reagiert – auch
   in der Hardware AUS. Einschaltbefehle werden abgelehnt, bis update() die
   Hardware wieder erreicht.
   ---------------------------------------------------------------------------- */
void StateManager::enterFault()
{
    const uint32_t now = millis();
    _fault = true;
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        _out[out].requested  = false;
        _out[out].relayOn    = false;
        _out[out].offSinceMs = now;
    }
    _mcp.writeOutputs(0x00, 0x00);   // bestmöglicher Versuch, Ergebnis bewusst ignoriert
    _lastCheckMs = now;
    changed();
}
