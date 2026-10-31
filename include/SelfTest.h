/* ============================================================================
   Datei:   SelfTest.h
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Automatischer Relaistest für die Inbetriebnahme und den Service. Schaltet
   nacheinander jede Spannung jedes Ausgangs kurz ein (12 Relais), damit am
   Koffer geprüft werden kann, ob jedes Relais anzieht und die richtige
   Spannung an der richtigen Klemme anliegt.

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Ablaufsteuerung des Tests (Zustandsautomat, nicht blockierend)
   - Start nur über die Shell und nur nach ausdrücklicher Bestätigung

   Sicherheit
   ----------------------------------------------------------------------------
   - WÄHREND DES SELBSTTESTS DARF KEIN PRÜFLING ANGESCHLOSSEN SEIN: Jeder
     Ausgang erhält nacheinander 24 V, 48 V und 230 V.
   - Start in zwei Schritten: «selbsttest start» zeigt die Warnung,
     «selbsttest ja» innerhalb von 10 s startet den Test.
   - Der Test schaltet ausschliesslich über den StateManager. Damit gelten
     auch hier alle Sicherheitsregeln (Sicherheitspause, Verriegelung,
     Störungsabschaltung).
   - Bei einer Störung oder mit «selbsttest stop» bricht der Test ab und
     schaltet alles aus. Das Display ist während des Tests gesperrt.
   ============================================================================ */

#pragma once
#include <Arduino.h>
#include "StateManager.h"

class SelfTest
{
public:
    explicit SelfTest(StateManager& state);

    // Schritt 1: Start anfordern (Warnung ausgeben, Bestätigung abwarten).
    void requestStart();
    // Schritt 2: Start bestätigen. false, wenn keine gültige Anforderung vorliegt.
    bool confirmStart();
    void stop();

    // Muss in jedem loop()-Durchlauf aufgerufen werden.
    void update();

    bool    isRunning()     const { return _phase != Phase::IDLE; }
    bool    isAwaitingConfirm() const;
    uint8_t currentOutput() const { return _out; }
    Voltage currentVoltage() const { return voltageFromIndex(_voltage); }

    // Fortschrittsmeldung für die Shell (zählt bei jedem Schritt hoch)
    uint32_t    stepId()   const { return _stepId; }
    const char* stepText() const { return _stepText; }

private:
    enum class Phase : uint8_t
    {
        IDLE,          // kein Test aktiv
        SWITCH_ON,     // Spannung wählen und Einschalten anfordern
        WAIT_ON,       // warten, bis das Relais tatsächlich EIN ist
        HOLD_ON,       // Relais eingeschaltet halten
        PAUSE          // Relais aus, Pause bis zum nächsten Schritt
    };

    void next();
    void finish(const char* text);
    void report(const char* text);

    StateManager& _state;
    Phase    _phase         = Phase::IDLE;
    uint8_t  _out           = 0;
    uint8_t  _voltage       = 0;
    uint32_t _phaseStartMs  = 0;
    uint32_t _confirmUntil  = 0;   // 0 = keine Startanforderung offen
    uint32_t _stepId        = 0;
    char     _stepText[64]  = "";
};
