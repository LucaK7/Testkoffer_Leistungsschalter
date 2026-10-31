/* ============================================================================
   Datei:   SelfTest.cpp
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Ablauf (Zustandsautomat, ein Schritt pro update()-Aufruf):

     für jeden Ausgang MN/MX2, MX, XF, MCH
       für jede Spannung 24 V, 48 V, 230 V
         SWITCH_ON : Spannung setzen, Einschalten anfordern
         WAIT_ON   : warten bis StateManager das Relais eingeschaltet hat
                     (Sicherheitspause 200 ms), max. 2 s
         HOLD_ON   : 400 ms eingeschaltet lassen
         PAUSE     : ausschalten, 300 ms warten
   Danach: alles aus, Ansicht bleibt MTZ/NW/NT.
   ============================================================================ */

#include "SelfTest.h"
#include "Config.h"

SelfTest::SelfTest(StateManager& state)
    : _state(state)
{
}

/* ----------------------------------------------------------------------------
   requestStart()
   Öffnet ein Zeitfenster von 10 s, in dem der Start bestätigt werden kann.
   ---------------------------------------------------------------------------- */
void SelfTest::requestStart()
{
    if (isRunning())
    {
        return;
    }
    _confirmUntil = millis() + Timing::SELFTEST_CONFIRM_MS;
    if (_confirmUntil == 0) _confirmUntil = 1;   // 0 bedeutet «keine Anforderung»
}

bool SelfTest::isAwaitingConfirm() const
{
    return (_confirmUntil != 0) && (static_cast<int32_t>(_confirmUntil - millis()) > 0);
}

/* ----------------------------------------------------------------------------
   confirmStart()
   Startet den Test, wenn eine gültige Anforderung offen ist und keine
   Störung vorliegt. Der Test läuft immer in der Ansicht MTZ/NW/NT, damit
   die NSX-Kopplung die einzelnen Schritte nicht beeinflusst. Die
   Bestätigung «Prüfling und Testkabel abgehängt» ersetzt die Auswahl des
   Prüfkabels (R8); nach dem Test muss das Kabel neu gewählt werden.
   ---------------------------------------------------------------------------- */
bool SelfTest::confirmStart()
{
    if (!isAwaitingConfirm() || isRunning() || _state.isFault())
    {
        _confirmUntil = 0;
        return false;
    }
    _confirmUntil = 0;

    _state.chooseView(View::MTZ_NW_NT);   // alles AUS, 24 V, Einschalten frei

    _out          = 0;
    _voltage      = 0;
    _phase        = Phase::SWITCH_ON;
    _phaseStartMs = millis();
    report("Selbsttest gestartet");
    return true;
}

void SelfTest::stop()
{
    _confirmUntil = 0;
    if (isRunning())
    {
        finish("Selbsttest abgebrochen");
    }
}

/* ----------------------------------------------------------------------------
   update()
   ---------------------------------------------------------------------------- */
void SelfTest::update()
{
    // Abgelaufene Startanforderung verwerfen
    if (_confirmUntil != 0 && !isAwaitingConfirm())
    {
        _confirmUntil = 0;
    }

    if (!isRunning())
    {
        return;
    }

    // Störung: sofort abbrechen (StateManager hat bereits alles ausgeschaltet)
    if (_state.isFault())
    {
        finish("Selbsttest abgebrochen: I/O-Stoerung");
        return;
    }

    const uint32_t now     = millis();
    const uint32_t elapsed = now - _phaseStartMs;

    switch (_phase)
    {
        case Phase::SWITCH_ON:
        {
            _state.setVoltage(_out, voltageFromIndex(_voltage));
            _state.requestOn(_out);

            char text[64];
            snprintf(text, sizeof(text), "%-3s %-6s %s EIN",
                     relayPin(_out, voltageFromIndex(_voltage)).name,
                     outputLabel(View::MTZ_NW_NT, _out).code,
                     voltageName(voltageFromIndex(_voltage)));
            report(text);

            _phase        = Phase::WAIT_ON;
            _phaseStartMs = now;
            break;
        }

        case Phase::WAIT_ON:
            if (_state.isOn(_out))
            {
                _phase        = Phase::HOLD_ON;
                _phaseStartMs = now;
            }
            else if (elapsed > 2000)
            {
                finish("Selbsttest abgebrochen: Ausgang schaltet nicht ein");
            }
            break;

        case Phase::HOLD_ON:
            if (elapsed >= Timing::SELFTEST_ON_MS)
            {
                _state.requestOff(_out);
                _phase        = Phase::PAUSE;
                _phaseStartMs = now;
            }
            break;

        case Phase::PAUSE:
            if (elapsed >= Timing::SELFTEST_PAUSE_MS)
            {
                next();
            }
            break;

        case Phase::IDLE:
        default:
            break;
    }
}

/* ----------------------------------------------------------------------------
   next()
   Nächste Spannung, danach nächster Ausgang, danach Ende.
   ---------------------------------------------------------------------------- */
void SelfTest::next()
{
    _voltage++;
    if (_voltage >= VOLTAGE_COUNT)
    {
        _voltage = 0;
        _out++;
    }

    if (_out >= OUTPUT_COUNT)
    {
        finish("Selbsttest beendet: alle 12 Relais geschaltet");
        return;
    }

    _phase        = Phase::SWITCH_ON;
    _phaseStartMs = millis();
}

void SelfTest::finish(const char* text)
{
    _phase = Phase::IDLE;
    _state.requireCableConfirmation();    // alles AUS, Prüfkabel neu wählen (R8)
    _state.setVoltageAll(Voltage::V24);   // definierter Zustand nach dem Test
    report(text);
}

void SelfTest::report(const char* text)
{
    strncpy(_stepText, text, sizeof(_stepText) - 1);
    _stepText[sizeof(_stepText) - 1] = '\0';
    _stepId++;
}
