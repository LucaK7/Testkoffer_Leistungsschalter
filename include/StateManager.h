/* ============================================================================
   Datei:   StateManager.h
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Zentrale Logik- und Sicherheitsinstanz. Display, Shell und Selbsttest
   schalten NIE selbst Relais, sondern richten ihre Wünsche an den
   StateManager. Dieser entscheidet nach festen Regeln, was wann an den
   McpManager (Hardware) weitergegeben wird.

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Zustand jedes Ausgangs: gewählte Spannung, gewünscht EIN, Relais EIN
   - Sicherheitsregeln:
       R1  Pro Ausgang ist immer höchstens eine Spannung aktiv.
       R2  Ein Ausgang geht frühestens 200 ms nach seinem letzten
           Ausschalten wieder EIN (Relais sicher abgefallen, «Break before
           make» auch beim Spannungswechsel).
       R3  Verriegelung Ein-/Ausschaltspule (MX/XF): Wird die eine
           eingeschaltet, geht die andere sofort AUS; die gewünschte
           schaltet erst 200 ms danach ein. Nicht abschaltbar.
       R4  Wird die Spannung eines eingeschalteten Ausgangs geändert, geht
           dieser AUS. Der Techniker muss bewusst wieder EIN drücken.
       R5  Ansicht NSX: A2, A4 und B4 (Motorantrieb MT, Ausgänge XF, MX,
           MCH) haben immer dieselbe Spannung. Sicherheitsfunktion: Im
           NSX-Prüfkabel sind die Rückleiter dieser drei Ausgänge auf A1
           gebrückt; unterschiedliche Spannungen würden z. B. den
           Neutralleiter 230 VAC mit GND 24 VDC verbinden.
       R6  Ansichtswechsel schaltet alle Ausgänge AUS und setzt alle
           Spannungen auf 24 V (wie beim Start).
       R7  Meldet der McpManager einen nicht reparierbaren Fehler, werden
           alle Ausgänge AUS geschaltet (Störungszustand). Neue
           Einschaltbefehle werden abgelehnt, bis die Hardware wieder
           einwandfrei antwortet.
       R8  Prüfkabel bestätigen: Einschalten ist erst möglich, wenn der
           Techniker bestätigt hat, dass das Prüfkabel zur Ansicht passt.
           Beim Start und nach dem Selbsttest muss das Kabel gewählt
           werden. Ein Ansichtswechsel beginnt mit «alles AUS» (das Kabel
           wird umgesteckt) und gilt erst nach der Bestätigung; bis dahin
           ist Einschalten gesperrt. Hintergrund: Im NSX-Prüfkabel sind
           Rückleiter gebrückt (siehe R5).
   - Zyklische Kontrolle der Hardware (alle 100 ms)
   - Änderungszähler (revision), damit das Display weiss, wann es neu
     zeichnen muss

   Begriffe
   ----------------------------------------------------------------------------
   requested : Der Benutzer möchte den Ausgang EIN haben.
   relayOn   : Der Ausgang ist tatsächlich eingeschaltet (an die Hardware
               übergeben). Zwischen beiden liegt höchstens die
               Sicherheitspause.
   ============================================================================ */

#pragma once
#include <Arduino.h>
#include "Outputs.h"
#include "McpManager.h"

class StateManager
{
public:
    explicit StateManager(McpManager& mcp);

    // Startzustand: Ansicht MTZ/NW/NT, alle Ausgänge 24 V und AUS,
    // Prüfkabel noch nicht bestätigt (R8).
    void begin();

    // Muss in jedem loop()-Durchlauf aufgerufen werden (nicht blockierend).
    void update();

    // --- Befehle (von Display, Shell, Selbsttest) ----------------------------
    bool requestOn(uint8_t out);                    // false = abgelehnt (Störung, R8)
    void requestOff(uint8_t out);
    void toggle(uint8_t out);                       // EIN <-> AUS (Display-Taste)
    void setVoltage(uint8_t out, Voltage v);        // beachtet R4 und R5
    void setVoltageAll(Voltage v);                  // alle Ausgänge der Ansicht
    void allOff();

    // --- Ansicht und Prüfkabel (R6, R8) --------------------------------------
    // Ansichtswechsel anfordern: alles AUS, Einschalten gesperrt, bis
    // bestätigt oder abgebrochen. timeoutMs = 0: ohne Zeitlimit (Display),
    // sonst Abbruch nach Ablauf (Shell).
    void requestViewChange(View v, uint32_t timeoutMs);
    bool confirmViewChange();                       // false = kein Wechsel offen
    void cancelViewChange();
    // Ansicht direkt wählen und Prüfkabel als bestätigt setzen (Auswahl
    // auf dem Startbildschirm, Start des Selbsttests).
    void chooseView(View v);
    // Bestätigung zurücknehmen (nach dem Selbsttest): alles AUS, neue
    // Auswahl des Prüfkabels nötig.
    void requireCableConfirmation();

    // --- Abfragen ------------------------------------------------------------
    View     view()                    const { return _view; }
    Voltage  voltage(uint8_t out)      const { return _out[out].voltage; }
    bool     isRequested(uint8_t out)  const { return _out[out].requested; }
    bool     isOn(uint8_t out)         const { return _out[out].relayOn; }
    bool     anyOn()                   const;
    bool     isFault()                 const { return _fault; }
    bool     allSameVoltage(Voltage& common) const;
    bool     isCoupled(uint8_t out)    const;   // gehört der Ausgang zur NSX-Kopplung?
    bool     cableConfirmed()          const { return _cableOk && !_pending; }
    bool     viewChangePending()       const { return _pending; }
    View     pendingView()             const { return _pendingView; }
    uint32_t revision()                const { return _revision; }

private:
    struct OutputState
    {
        Voltage  voltage    = Voltage::V24;
        bool     requested  = false;
        bool     relayOn    = false;
        uint32_t offSinceMs = 0;    // Zeitpunkt des letzten Ausschaltens
    };

    bool    pauseElapsed(uint8_t out, uint32_t now) const;
    int8_t  interlockPartner(uint8_t out) const;
    void    switchRelayOff(uint8_t out);
    void    setVoltageSingle(uint8_t out, Voltage v);
    void    applyView(View v);
    void    applyToHardware();
    void    handleResult(McpManager::Result r);
    void    enterFault();
    void    changed() { _revision++; }

    McpManager& _mcp;
    OutputState _out[OUTPUT_COUNT];
    View        _view          = View::MTZ_NW_NT;
    bool        _fault         = false;
    uint32_t    _revision      = 0;
    uint32_t    _lastCheckMs   = 0;

    bool        _cableOk       = false;   // R8: Prüfkabel für die Ansicht bestätigt
    bool        _pending       = false;   // Ansichtswechsel angefordert, noch nicht bestätigt
    View        _pendingView   = View::MTZ_NW_NT;
    uint32_t    _pendingUntil  = 0;       // Zeitlimit (0 = keines)
};
