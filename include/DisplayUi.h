/* ============================================================================
   Datei:   DisplayUi.h
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Bedienoberfläche auf dem Touch-Display Joy-IT RB-TFT3.2-V3 (ILI9341,
   320 × 240 Pixel im Querformat, Touch-Controller XPT2046). Hauptbedienung
   für den Techniker.

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Display einschalten
   - Touch-Kalibrierung laden bzw. beim ersten Start durchführen und im
     Flash des ESP32 speichern
   - Bildschirm zeichnen und bei Änderungen flackerfrei nachzeichnen
   - Berührungen auswerten und als Befehle an den StateManager weitergeben

   Bildschirmaufbau (320 × 240 Pixel)
   ----------------------------------------------------------------------------
   Ansicht MTZ/NW/NT:
     y   0 ┌──────────┬──────┬────────────┬──────────┐
           │MTZ/NW/NT │ NSX  │ Alle:24V ▼ │ Alle AUS │  Kopfzeile («Alle AUS» gelb)
     y  42 ├──────────┴──────┴──┬─────────┴┬─────────┤
           │ MN/MX2  Unterspg.  │ 24 V  ▼  │  AUS    │  Ausgang 1
     y  92 ├────────────────────┼──────────┼─────────┤
           │ XF      Ein        │ 24 V  ▼  │  AUS    │  Ausgang 2
     y 142 ├────────────────────┼──────────┼─────────┤
           │ MX      Aus        │ 24 V  ▼  │  AUS    │  Ausgang 3
     y 192 ├────────────────────┼──────────┼─────────┤
           │ MCH     Motor      │ 24 V  ▼  │  AUS    │  Ausgang 4
     y 240 └────────────────────┴──────────┴─────────┘

   Ansicht NSX: «Alle AUS» füllt die rechte Kopfzeile. Die drei Ausgänge des
   Motorantriebs MT (XF = A2, MX = A4, MCH = B4) haben eine gemeinsame,
   über drei Zeilen reichende Spannungstaste «MT» (Kopplung R5).
           │ MN/MX2  Unterspg.  │ 24 V  ▼  │  AUS    │
           ├────────────────────┼──────────┼─────────┤
           │ XF   EIN (A2)      │    MT    │  AUS    │
           │ MX   AUS (A4)      │  24 V ▼  │  AUS    │
           │ MCH  Motor (B4)    │          │  AUS    │

   Prüfkabel bestätigen (R8)
   ----------------------------------------------------------------------------
   Startbildschirm (nach dem Einschalten und nach dem Selbsttest):
     ┌────────────────────────────────────────┐
     │          Pruefkabel waehlen            │  violettes Titelband
     │          Welches Pruefkabel            │
     │           ist eingesteckt?             │
     │ Bis zur Auswahl bleiben alle Ausg. AUS │
     │ ┌─────────────────┐ ┌────────────────┐ │
     │ │   MTZ/NW/NT     │ │      NSX       │ │  je 150 × 70 Pixel
     │ └─────────────────┘ └────────────────┘ │
     └────────────────────────────────────────┘
   Zwischenbildschirm beim Tipp auf den anderen Reiter (alle Ausgänge sind
   bereits AUS, Einschalten gesperrt, bis bestätigt):
     │           Ansicht wechseln             │
     │        Ist das Pruefkabel fuer         │
     │           NSX eingesteckt?             │
     │  Alle Ausgaenge wurden ausgeschaltet.  │
     │ [   Abbrechen   ]   [       Ja       ] │  grau / grün
   «Abbrechen» kehrt zur bisherigen Ansicht zurück (Ausgänge bleiben AUS).

   Tastengrössen: Kopfzeile 34 Pixel (≈ 7 mm) hoch, EIN/AUS-Tasten 46 × 98
   Pixel (≈ 9 × 20 mm) – gut mit dem Finger zu treffen.
   Ein Tipp auf eine Spannungstaste öffnet eine Auswahlliste (24/48/230 V).
   Ein Tipp ausserhalb der Liste schliesst sie ohne Änderung.

   Flackerfreies Zeichnen
   ----------------------------------------------------------------------------
   Der Bildschirm ist in fünf waagrechte Streifen («Bänder») geteilt: die
   Kopfzeile und die vier Zeilen der Ausgänge. Für jedes Band wird ein
   kurzer Schlüssel aus allem gebildet, was darin zu sehen ist (Ansicht,
   Spannung, EIN/AUS, Auswahlliste …). Nur Bänder, deren Schlüssel sich
   geändert hat, werden neu gezeichnet.
   Ein Band wird zuerst unsichtbar in einem Zwischenspeicher (Sprite,
   320 × 50 Pixel, 32 kB RAM) vollständig aufgebaut und dann in einem
   Stück zum Display übertragen. Der Aufbau ist dadurch nicht sichtbar:
   Es gibt weder schwarze Zwischenbilder noch Flackern. Auch die
   Auswahlliste wird so über den Inhalt gelegt und wieder entfernt.

   Wichtige Hinweise
   ----------------------------------------------------------------------------
   - Die Bedienoberfläche schaltet nie selbst Relais; jeder Tipp wird als
     Befehl an den StateManager gegeben, der die Sicherheitsregeln anwendet.
   - Die verwendeten Schriften enthalten keine Umlaute; Texte auf dem
     Display sind deshalb mit ae/oe/ue geschrieben.
   - Während des Selbsttests und bei einer I/O-Störung ersetzt ein farbiges
     Band die Kopfzeile; Bedienung ist dann gesperrt bzw. eingeschränkt.
   ============================================================================ */

#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
#include "StateManager.h"
#include "SelfTest.h"

class DisplayUi
{
public:
    DisplayUi(StateManager& state, SelfTest& selftest);

    // Display starten, Touch kalibrieren (falls nötig), erstes Bild zeichnen.
    // Muss VOR dem Aktivieren des Watchdogs aufgerufen werden, weil die
    // Kalibrierung auf Eingaben des Benutzers wartet.
    void begin();

    // Muss in jedem loop()-Durchlauf aufgerufen werden (nicht blockierend).
    void update();

    // Merkt eine neue Touch-Kalibrierung vor und startet den ESP32 neu.
    // Die Kalibrierung läuft dann in begin(), bevor der Watchdog aktiv ist.
    void scheduleCalibration();

private:
    // --- Auswahlliste («Popup») ------------------------------------------------
    static constexpr int8_t POPUP_NONE = -1;             // keine Liste offen
    static constexpr int8_t POPUP_ALL  = OUTPUT_COUNT;   // Liste der Taste «Alle»

    // --- Bänder ----------------------------------------------------------------
    static constexpr uint8_t BAND_HEADER = 0;                  // Kopfzeile
    static constexpr uint8_t BAND_COUNT  = OUTPUT_COUNT + 1;   // + eine Zeile je Ausgang

    // Schlüssel eines Bandes: alles, was darin sichtbar ist. Ändert sich ein
    // Byte, wird das Band neu gezeichnet.
    struct BandKey
    {
        uint8_t d[8];
        bool operator!=(const BandKey& o) const { return memcmp(d, o.d, sizeof(d)) != 0; }
    };

    // Rechteck auf dem Bildschirm
    struct Rect
    {
        int16_t x, y, w, h;
        bool contains(int16_t px, int16_t py) const
        {
            return px >= x && px < x + w && py >= y && py < y + h;
        }
        bool overlaps(const Rect& o) const
        {
            return x < o.x + o.w && o.x < x + w && y < o.y + o.h && o.y < y + h;
        }
    };

    // Touch
    void loadOrCalibrateTouch();
    void runCalibration(uint16_t cal[5]);
    void pollTouch();
    void onTap(int16_t x, int16_t y);
    void onPopupTap(int16_t x, int16_t y);
    void onModalTap(int16_t x, int16_t y);

    // Bildschirm: 0 = Bedienung, 1 = Startbildschirm (Kabel wählen),
    // 2 = Zwischenbildschirm (Ansichtswechsel bestätigen)
    enum class Screen : uint8_t { MAIN = 0, CHOOSE_CABLE = 1, CONFIRM_CHANGE = 2 };
    Screen screen() const;

    // Zeichnen
    void     redrawChanges();
    BandKey  bandKey(uint8_t band) const;
    void     drawBand(uint8_t band);
    void     drawHeader(TFT_eSPI& g, int16_t oy);
    void     drawBanner(TFT_eSPI& g, int16_t oy, uint16_t color, const char* text);
    void     drawRow(TFT_eSPI& g, int16_t oy, uint8_t out);
    void     drawMotorVoltage(TFT_eSPI& g, int16_t oy);
    void     drawPopup(TFT_eSPI& g, int16_t oy);
    void     drawModal(TFT_eSPI& g, int16_t oy);
    void     drawButton(TFT_eSPI& g, int16_t oy, const Rect& r, uint16_t fill,
                        const char* text, uint8_t font, bool arrow, uint16_t textColor);
    uint8_t  popupHighlight() const;   // Index der aktuellen Spannung in der Liste, 0xFF = keine

    // Geometrie
    static Rect bandRect(uint8_t band);
    static Rect tabMtzRect();
    static Rect tabNsxRect();
    static Rect allRect();              // «Alle»-Spannung (nur MTZ/NW/NT)
    Rect        allOffRect() const;     // «Alle AUS» (Breite je nach Ansicht)
    static Rect rowRect(uint8_t out);
    static Rect motorVoltageRect();     // gemeinsame Spannungstaste MT (NSX)
    Rect        voltageRect(uint8_t out) const;
    static Rect switchRect(uint8_t out);
    Rect        popupAnchor() const;
    Rect        popupItemRect(uint8_t item) const;
    Rect        popupFrameRect() const;
    bool        motorGroupShown() const;   // Ansicht NSX: gemeinsame Taste MT
    static Rect modalLeftRect();
    static Rect modalRightRect();

    TFT_eSPI      _tft;
    TFT_eSprite   _spr;                // Zwischenspeicher für ein Band
    bool          _sprOk = false;      // false: direkt zeichnen (Notbetrieb)
    StateManager& _state;
    SelfTest&     _selftest;

    BandKey  _shown[BAND_COUNT] = {};  // zuletzt gezeichnete Schlüssel
    bool     _shownValid    = false;
    int8_t   _popup         = POPUP_NONE;
    View     _lastView      = View::MTZ_NW_NT;

    uint32_t _lastPollMs    = 0;
    uint32_t _lastTapMs     = 0;
    uint8_t  _releaseCount  = 0;       // Anzahl Abfragen ohne Berührung
    bool     _touchActive   = false;   // Finger liegt (noch) auf
};
