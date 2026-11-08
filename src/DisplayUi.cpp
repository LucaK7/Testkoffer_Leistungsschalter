/* ============================================================================
   Datei:   DisplayUi.cpp
   Projekt: SE Testkoffer – Steuerung V55
   ============================================================================
   Umsetzung der Bedienoberfläche (siehe Bildschirmaufbau in DisplayUi.h).

   Abschnitte dieser Datei
   ----------------------------------------------------------------------------
   1. Geometrie       – Lage aller Schaltflächen an einer Stelle
   2. Start           – Display, Zwischenspeicher, Touch-Kalibrierung
   3. Zyklus          – update(): Touch abfragen, Änderungen nachzeichnen
   4. Touch-Auswertung– welcher Tipp löst welchen Befehl aus
   5. Zeichnen        – Bänder, Kopfzeile, Zeilen, Auswahlliste

   Nachzeichnen ohne Flackern (Kurzfassung, Details in DisplayUi.h)
   ----------------------------------------------------------------------------
   Pro Band (Kopfzeile, vier Zeilen) wird ein Schlüssel gebildet. Nur Bänder
   mit geändertem Schlüssel werden im Zwischenspeicher (Sprite) neu
   aufgebaut und am Stück übertragen. Alle Zeichenfunktionen erhalten
   deshalb das Ziel «g» (Sprite oder notfalls direkt das Display) und den
   Versatz «oy» (Oberkante des Bandes): Gezeichnet wird mit Bildschirm-
   koordinaten minus oy. Was über das Band hinausragt, schneidet das
   Sprite automatisch ab – so kann z. B. die hohe MT-Taste in drei Bändern
   einfach jeweils ganz gezeichnet werden.
   ============================================================================ */

#include "DisplayUi.h"
#include <Preferences.h>
#include "Config.h"

/* ============================================================================
   1. Geometrie
   ============================================================================ */

namespace Layout
{
    constexpr int16_t SCREEN_W   = 320;
    constexpr int16_t SCREEN_H   = 240;

    constexpr int16_t HEADER_Y   = 2;     // Tasten der Kopfzeile
    constexpr int16_t HEADER_H   = 34;
    constexpr int16_t ROW_Y0     = 42;    // Oberkante der ersten Zeile (= Ende Kopfzeilen-Band)
    constexpr int16_t ROW_PITCH  = 50;    // Abstand der Zeilen (= Höhe eines Zeilen-Bandes)
    constexpr int16_t ROW_H      = 46;    // Höhe der Tasten in einer Zeile

    constexpr int16_t LABEL_X    = 6;     // Beschriftung links
    constexpr int16_t VOLT_X     = 124;   // Spannungstaste
    constexpr int16_t VOLT_W     = 90;
    constexpr int16_t SWITCH_X   = 220;   // EIN/AUS-Taste
    constexpr int16_t SWITCH_W   = 98;

    // Kopfzeile (x, Breite)
    constexpr int16_t TAB_MTZ_X  = 2;     constexpr int16_t TAB_MTZ_W = 84;
    constexpr int16_t TAB_NSX_X  = 90;    constexpr int16_t TAB_NSX_W = 52;
    constexpr int16_t ALL_X      = 146;   constexpr int16_t ALL_W     = 92;
    constexpr int16_t ALLOFF_X   = 242;   constexpr int16_t ALLOFF_W  = 76;   // MTZ/NW/NT
    // NSX: «Alle AUS» belegt den Platz von «Alle» und «Alle AUS» zusammen.

    constexpr int16_t POPUP_ITEM_H = 44;  // Höhe eines Eintrags der Auswahlliste
    constexpr int16_t POPUP_MARGIN = 4;   // schwarzer Rand + weisser Rahmen um die Liste
    constexpr int16_t RADIUS       = 6;   // Eckenradius der Tasten

    constexpr int16_t SPRITE_H     = ROW_PITCH;   // höchstes Band
}

// Die drei Ausgänge des Motorantriebs müssen untereinander liegen, damit die
// gemeinsame Spannungstaste MT über drei Zeilen reichen kann.
static_assert(OUT_MX == OUT_XF + 1 && OUT_MCH == OUT_XF + 2,
              "XF, MX und MCH muessen aufeinanderfolgende Zeilen sein");

DisplayUi::Rect DisplayUi::bandRect(uint8_t band)
{
    if (band == BAND_HEADER)
    {
        return { 0, 0, Layout::SCREEN_W, Layout::ROW_Y0 };
    }
    const int16_t y = Layout::ROW_Y0 + (band - 1) * Layout::ROW_PITCH;
    const int16_t rest = Layout::SCREEN_H - y;
    const int16_t h    = (rest < Layout::ROW_PITCH) ? rest : Layout::ROW_PITCH;   // letztes Band: 48
    return { 0, y, Layout::SCREEN_W, h };
}

DisplayUi::Rect DisplayUi::tabMtzRect() { return { Layout::TAB_MTZ_X, Layout::HEADER_Y, Layout::TAB_MTZ_W, Layout::HEADER_H }; }
DisplayUi::Rect DisplayUi::tabNsxRect() { return { Layout::TAB_NSX_X, Layout::HEADER_Y, Layout::TAB_NSX_W, Layout::HEADER_H }; }
DisplayUi::Rect DisplayUi::allRect()    { return { Layout::ALL_X,     Layout::HEADER_Y, Layout::ALL_W,     Layout::HEADER_H }; }

DisplayUi::Rect DisplayUi::allOffRect() const
{
    if (_state.view() == View::NSX)
    {
        return { Layout::ALL_X, Layout::HEADER_Y,
                 static_cast<int16_t>(Layout::ALLOFF_X + Layout::ALLOFF_W - Layout::ALL_X),
                 Layout::HEADER_H };
    }
    return { Layout::ALLOFF_X, Layout::HEADER_Y, Layout::ALLOFF_W, Layout::HEADER_H };
}

DisplayUi::Rect DisplayUi::rowRect(uint8_t out)
{
    return { 0, static_cast<int16_t>(Layout::ROW_Y0 + out * Layout::ROW_PITCH),
             Layout::SCREEN_W, Layout::ROW_H };
}

DisplayUi::Rect DisplayUi::motorVoltageRect()
{
    const int16_t top    = rowRect(OUT_XF).y;
    const int16_t bottom = rowRect(OUT_MCH).y + Layout::ROW_H;
    return { Layout::VOLT_X, top, Layout::VOLT_W, static_cast<int16_t>(bottom - top) };   // 146 Pixel hoch
}

bool DisplayUi::motorGroupShown() const
{
    return _state.view() == View::NSX;
}

// Tasten des Start- bzw. Zwischenbildschirms (je 150 × 70 Pixel)
DisplayUi::Rect DisplayUi::modalLeftRect()  { return { 6,   160, 150, 70 }; }
DisplayUi::Rect DisplayUi::modalRightRect() { return { 164, 160, 150, 70 }; }

DisplayUi::Screen DisplayUi::screen() const
{
    if (_state.viewChangePending()) return Screen::CONFIRM_CHANGE;
    if (!_state.cableConfirmed())   return Screen::CHOOSE_CABLE;
    return Screen::MAIN;
}

DisplayUi::Rect DisplayUi::voltageRect(uint8_t out) const
{
    if (motorGroupShown() && _state.isCoupled(out))
    {
        return motorVoltageRect();
    }
    const Rect row = rowRect(out);
    return { Layout::VOLT_X, row.y, Layout::VOLT_W, Layout::ROW_H };
}

DisplayUi::Rect DisplayUi::switchRect(uint8_t out)
{
    const Rect row = rowRect(out);
    return { Layout::SWITCH_X, row.y, Layout::SWITCH_W, Layout::ROW_H };
}

/* ----------------------------------------------------------------------------
   Auswahlliste: popupAnchor() / popupItemRect() / popupFrameRect()
   Die Liste legt sich über die angetippte Taste und klappt nach unten auf.
   Hat sie dort keinen Platz (untere Zeilen), klappt sie nach oben auf,
   sodass ihr unterer Rand mit der Taste abschliesst. Ist die Taste höher
   als die Liste (gemeinsame Taste MT), wird die Liste darauf zentriert.
   Rundherum bleibt ein Rand von POPUP_MARGIN Pixeln für den Rahmen.
   ---------------------------------------------------------------------------- */
DisplayUi::Rect DisplayUi::popupAnchor() const
{
    return (_popup == POPUP_ALL) ? allRect() : voltageRect(static_cast<uint8_t>(_popup));
}

DisplayUi::Rect DisplayUi::popupItemRect(uint8_t item) const
{
    const Rect    anchor = popupAnchor();
    const int16_t listH  = VOLTAGE_COUNT * Layout::POPUP_ITEM_H;

    int16_t y = anchor.y;                                       // nach unten aufklappen
    if (anchor.h > listH)
    {
        y = anchor.y + (anchor.h - listH) / 2;                  // auf hoher Taste zentrieren
    }
    if (y + listH + Layout::POPUP_MARGIN > Layout::SCREEN_H)
    {
        y = anchor.y + anchor.h - listH;                        // nach oben aufklappen
    }
    if (y + listH + Layout::POPUP_MARGIN > Layout::SCREEN_H)
    {
        y = Layout::SCREEN_H - Layout::POPUP_MARGIN - listH;    // Rahmen ganz sichtbar
    }
    if (y < Layout::POPUP_MARGIN)
    {
        y = Layout::POPUP_MARGIN;
    }
    return { anchor.x, static_cast<int16_t>(y + item * Layout::POPUP_ITEM_H),
             anchor.w, Layout::POPUP_ITEM_H };
}

DisplayUi::Rect DisplayUi::popupFrameRect() const
{
    const Rect    first = popupItemRect(0);
    const int16_t m     = Layout::POPUP_MARGIN;
    return { static_cast<int16_t>(first.x - m), static_cast<int16_t>(first.y - m),
             static_cast<int16_t>(first.w + 2 * m),
             static_cast<int16_t>(VOLTAGE_COUNT * Layout::POPUP_ITEM_H + 2 * m) };
}

/* ============================================================================
   2. Start
   ============================================================================ */

DisplayUi::DisplayUi(StateManager& state, SelfTest& selftest)
    : _spr(&_tft), _state(state), _selftest(selftest)
{
}

/* ----------------------------------------------------------------------------
   begin()
   IO32 (Taster BTN0 des Displays) als Eingang festlegen, Display
   initialisieren, Querformat einstellen, Zwischenspeicher anlegen, Touch
   vorbereiten und das erste Bild zeichnen.
   Kann der Zwischenspeicher (32 kB) nicht angelegt werden, zeichnet die
   Oberfläche direkt auf das Display (funktioniert, aber mit Flackern).
   ---------------------------------------------------------------------------- */
void DisplayUi::begin()
{
    // Definierter Pegel an IO32; der Pin wird nie als Ausgang betrieben.
    pinMode(Pins::LCD_BUTTON0, INPUT_PULLUP);

    _tft.init();
    _tft.setRotation(DisplayConfig::ROTATION);
    _tft.fillScreen(Colors::BACKGROUND);

    _spr.setColorDepth(16);
    _sprOk = (_spr.createSprite(Layout::SCREEN_W, Layout::SPRITE_H) != nullptr);

    loadOrCalibrateTouch();

    _tft.fillScreen(Colors::BACKGROUND);   // Reste der Kalibrierung entfernen
    _shownValid = false;
    redrawChanges();
}

/* ----------------------------------------------------------------------------
   loadOrCalibrateTouch()
   Die fünf Kalibrierwerte des Touch-Controllers werden im nichtflüchtigen
   Speicher des ESP32 (Namensraum «touch») abgelegt. Eine neue Kalibrierung
   erfolgt, wenn
     - noch keine Werte gespeichert sind (erster Start),
     - die gespeicherten Werte zu einer anderen Displaydrehung gehören, oder
     - über die Shell («kalibrieren») eine Kalibrierung angefordert wurde.
   ---------------------------------------------------------------------------- */
void DisplayUi::loadOrCalibrateTouch()
{
    Preferences prefs;
    prefs.begin("touch", false);

    uint16_t cal[5] = { 0, 0, 0, 0, 0 };
    const size_t  stored   = prefs.getBytes("cal", cal, sizeof(cal));
    const uint8_t rotation = prefs.getUChar("rot", 0xFF);
    const bool    forced   = prefs.getBool("recal", false);

    if (forced || stored != sizeof(cal) || rotation != DisplayConfig::ROTATION)
    {
        runCalibration(cal);
        prefs.putBytes("cal", cal, sizeof(cal));
        prefs.putUChar("rot", DisplayConfig::ROTATION);
        prefs.putBool("recal", false);
    }
    prefs.end();

    _tft.setTouch(cal);   // TFT_eSPI rechnet Rohwerte ab jetzt in Pixel um
}

/* ----------------------------------------------------------------------------
   runCalibration()
   Kalibrierfunktion von TFT_eSPI: Der Benutzer tippt nacheinander auf die
   vier Markierungen in den Ecken. Die Funktion wartet, bis alle vier
   Punkte erfasst sind (blockierend, deshalb vor dem Watchdog).
   ---------------------------------------------------------------------------- */
void DisplayUi::runCalibration(uint16_t cal[5])
{
    _tft.fillScreen(Colors::BACKGROUND);
    _tft.setTextColor(Colors::TEXT, Colors::BACKGROUND);
    _tft.setTextDatum(MC_DATUM);
    _tft.drawString("Touch kalibrieren", Layout::SCREEN_W / 2, 90, 4);
    _tft.drawString("Nacheinander auf die Markierung", Layout::SCREEN_W / 2, 130, 2);
    _tft.drawString("in jeder Ecke tippen", Layout::SCREEN_W / 2, 150, 2);

    _tft.calibrateTouch(cal, TFT_MAGENTA, Colors::BACKGROUND, 15);

    _tft.fillScreen(Colors::BACKGROUND);
    _tft.drawString("Kalibrierung gespeichert", Layout::SCREEN_W / 2, 120, 4);
    delay(800);
}

void DisplayUi::scheduleCalibration()
{
    Preferences prefs;
    prefs.begin("touch", false);
    prefs.putBool("recal", true);
    prefs.end();
    ESP.restart();   // McpManager::begin() schaltet nach dem Neustart alle Relais aus
}

/* ============================================================================
   3. Zyklus
   ============================================================================ */

void DisplayUi::update()
{
    pollTouch();
    redrawChanges();
}

/* ----------------------------------------------------------------------------
   pollTouch()
   Fragt den Touch-Controller alle 30 ms ab. Ein Tipp wird nur beim
   Aufsetzen des Fingers ausgelöst (Flanke) und erst wieder, wenn der Finger
   sicher abgehoben wurde (3 Abfragen ohne Berührung) und mindestens 250 ms
   vergangen sind. So löst ein längerer oder unruhiger Fingerdruck nicht
   mehrfach aus.
   ---------------------------------------------------------------------------- */
void DisplayUi::pollTouch()
{
    const uint32_t now = millis();
    if (now - _lastPollMs < Timing::TOUCH_POLL_MS)
    {
        return;
    }
    _lastPollMs = now;

    uint16_t x = 0;
    uint16_t y = 0;
    const bool pressed = _tft.getTouch(&x, &y, DisplayConfig::TOUCH_THRESHOLD);

    if (pressed)
    {
        _releaseCount = 0;
        if (!_touchActive && (now - _lastTapMs >= Timing::TOUCH_LOCKOUT_MS))
        {
            _touchActive = true;
            _lastTapMs   = now;
            onTap(static_cast<int16_t>(x), static_cast<int16_t>(y));
        }
    }
    else if (_touchActive)
    {
        if (++_releaseCount >= 3)
        {
            _touchActive = false;
        }
    }
}

/* ============================================================================
   4. Touch-Auswertung
   ============================================================================ */

/* ----------------------------------------------------------------------------
   onTap()
   Ordnet eine Berührung einer Schaltfläche zu und gibt den entsprechenden
   Befehl an den StateManager.
   ---------------------------------------------------------------------------- */
void DisplayUi::onTap(int16_t x, int16_t y)
{
    // Während des Selbsttests ist das Display gesperrt (Steuerung über Shell).
    if (_selftest.isRunning())
    {
        return;
    }

    // Start- bzw. Zwischenbildschirm: nur dessen zwei Tasten sind aktiv.
    if (screen() != Screen::MAIN)
    {
        onModalTap(x, y);
        return;
    }

    // Offene Auswahlliste hat Vorrang vor allen anderen Schaltflächen.
    if (_popup != POPUP_NONE)
    {
        onPopupTap(x, y);
        return;
    }

    // --- Kopfzeile (bei Störung durch das rote Band verdeckt) --------------
    if (!_state.isFault())
    {
        // Tipp auf den anderen Reiter: alles AUS und Zwischenbildschirm (R8).
        // Tipp auf den bereits aktiven Reiter: keine Wirkung.
        if (tabMtzRect().contains(x, y))
        {
            if (_state.view() != View::MTZ_NW_NT) _state.requestViewChange(View::MTZ_NW_NT, 0);
            return;
        }
        if (tabNsxRect().contains(x, y))
        {
            if (_state.view() != View::NSX) _state.requestViewChange(View::NSX, 0);
            return;
        }
        if (allOffRect().contains(x, y)) { _state.allOff();                 return; }
        if (_state.view() == View::MTZ_NW_NT && allRect().contains(x, y))
        {
            _popup = POPUP_ALL;
            return;
        }
    }

    // --- Zeilen der Ausgänge -------------------------------------------------
    // In der Ansicht NSX liefert voltageRect() für XF, MX und MCH die
    // gemeinsame Taste MT; die Liste öffnet sich dann für den angetippten
    // Ausgang, und der StateManager überträgt die Spannung auf alle drei (R5).
    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        if (voltageRect(out).contains(x, y))
        {
            _popup = static_cast<int8_t>(out);
            return;
        }
        if (switchRect(out).contains(x, y))
        {
            _state.toggle(out);   // wird bei Störung vom StateManager abgelehnt
            return;
        }
    }
}

/* ----------------------------------------------------------------------------
   onPopupTap()
   Tipp auf einen Eintrag: Spannung übernehmen. Tipp daneben: Liste ohne
   Änderung schliessen. In beiden Fällen ist die Liste danach geschlossen.
   ---------------------------------------------------------------------------- */
void DisplayUi::onPopupTap(int16_t x, int16_t y)
{
    for (uint8_t item = 0; item < VOLTAGE_COUNT; item++)
    {
        if (popupItemRect(item).contains(x, y))
        {
            const Voltage v = voltageFromIndex(item);
            if (_popup == POPUP_ALL)
            {
                _state.setVoltageAll(v);
            }
            else
            {
                _state.setVoltage(static_cast<uint8_t>(_popup), v);
            }
            break;
        }
    }
    _popup = POPUP_NONE;
}

/* ----------------------------------------------------------------------------
   onModalTap()
   Startbildschirm: Tipp auf MTZ/NW/NT bzw. NSX wählt die Ansicht und
   bestätigt damit das eingesteckte Prüfkabel.
   Zwischenbildschirm: «Ja» übernimmt die neue Ansicht, «Abbrechen» kehrt
   zur bisherigen Ansicht zurück. Die Ausgänge bleiben in jedem Fall AUS.
   ---------------------------------------------------------------------------- */
void DisplayUi::onModalTap(int16_t x, int16_t y)
{
    if (screen() == Screen::CHOOSE_CABLE)
    {
        if (modalLeftRect().contains(x, y))  _state.chooseView(View::MTZ_NW_NT);
        if (modalRightRect().contains(x, y)) _state.chooseView(View::NSX);
    }
    else
    {
        if (modalLeftRect().contains(x, y))  _state.cancelViewChange();
        if (modalRightRect().contains(x, y)) _state.confirmViewChange();
    }
}

/* ============================================================================
   5. Zeichnen
   ============================================================================ */

/* ----------------------------------------------------------------------------
   redrawChanges()
   Bildet für jedes Band den Schlüssel und zeichnet nur die Bänder neu,
   deren Schlüssel sich seit dem letzten Zeichnen geändert hat.
   Eine offene Auswahlliste wird geschlossen, wenn sich die Ansicht ändert
   (z. B. über die Shell) oder Selbsttest bzw. Störung beginnen.
   ---------------------------------------------------------------------------- */
void DisplayUi::redrawChanges()
{
    if (_popup != POPUP_NONE &&
        (_state.view() != _lastView || _selftest.isRunning() || _state.isFault() ||
         screen() != Screen::MAIN))
    {
        _popup = POPUP_NONE;
    }
    _lastView = _state.view();

    for (uint8_t band = 0; band < BAND_COUNT; band++)
    {
        const BandKey key = bandKey(band);
        if (!_shownValid || key != _shown[band])
        {
            drawBand(band);
            _shown[band] = key;
        }
    }
    _shownValid = true;
}

/* ----------------------------------------------------------------------------
   bandKey()
   Fasst alles zusammen, was im Band sichtbar ist:
     Kopfzeile: Ansicht, Statusband, Auswahlliste, Spannung der Taste «Alle»
     Zeile:     Ansicht, Auswahlliste, Spannung und Zustand des Ausgangs,
                gemeinsame Spannung des Motorantriebs (NSX)
   Die Auswahlliste zählt nur, wenn sie das Band überdeckt.
   ---------------------------------------------------------------------------- */
DisplayUi::BandKey DisplayUi::bandKey(uint8_t band) const
{
    BandKey k = {};
    k.d[0] = static_cast<uint8_t>(_state.view());

    // Start- bzw. Zwischenbildschirm: Inhalt hängt nur von der Art des
    // Bildschirms, der gewünschten Ansicht und einer Störung ab.
    if (screen() != Screen::MAIN)
    {
        k.d[3] = _state.isFault() ? 1 : 0;
        k.d[6] = static_cast<uint8_t>(screen());
        k.d[7] = static_cast<uint8_t>(_state.pendingView());
        return k;
    }

    const bool popupHere = (_popup != POPUP_NONE) && popupFrameRect().overlaps(bandRect(band));
    k.d[1] = popupHere ? static_cast<uint8_t>(_popup + 1) : 0;
    k.d[2] = popupHere ? popupHighlight() : 0xFF;

    if (band == BAND_HEADER)
    {
        Voltage common = Voltage::V24;
        const bool same = _state.allSameVoltage(common);
        k.d[3] = _state.isFault() ? 1 : (_selftest.isRunning() ? 2 : 0);
        k.d[4] = same ? 1 : 0;
        k.d[5] = voltageIndex(common);
    }
    else
    {
        const uint8_t out = band - 1;
        k.d[3] = voltageIndex(_state.voltage(out));
        k.d[4] = _state.isOn(out) ? 1 : 0;
        if (motorGroupShown() && _state.isCoupled(out))
        {
            k.d[5] = voltageIndex(_state.voltage(OUT_XF));   // gemeinsame Taste MT
        }
    }
    return k;
}

/* ----------------------------------------------------------------------------
   drawBand()
   Baut ein Band im Zwischenspeicher auf und überträgt es am Stück. Im
   Notbetrieb ohne Zwischenspeicher wird direkt auf das Display gezeichnet.
   ---------------------------------------------------------------------------- */
void DisplayUi::drawBand(uint8_t band)
{
    const Rect b  = bandRect(band);
    TFT_eSPI&  g  = _sprOk ? static_cast<TFT_eSPI&>(_spr) : _tft;
    const int16_t oy = _sprOk ? b.y : 0;

    if (_sprOk)
    {
        _spr.fillSprite(Colors::BACKGROUND);
    }
    else
    {
        _tft.fillRect(b.x, b.y, b.w, b.h, Colors::BACKGROUND);
    }

    if (screen() != Screen::MAIN)
    {
        drawModal(g, oy);   // ganzer Bildschirm; das Band schneidet seinen Teil ab
    }
    else if (band == BAND_HEADER)
    {
        drawHeader(g, oy);
    }
    else
    {
        drawRow(g, oy, band - 1);
    }

    if (screen() == Screen::MAIN && _popup != POPUP_NONE && popupFrameRect().overlaps(b))
    {
        drawPopup(g, oy);
    }

    if (_sprOk)
    {
        _spr.pushSprite(b.x, b.y, 0, 0, b.w, b.h);
    }
}

/* ----------------------------------------------------------------------------
   drawHeader()
   Normalfall: zwei Reiter für die Ansicht, Taste «Alle» (nur MTZ/NW/NT) und
   die gelbe Taste «Alle AUS». Bei Störung bzw. Selbsttest ersetzt ein
   farbiges Band die Kopfzeile.
   ---------------------------------------------------------------------------- */
void DisplayUi::drawHeader(TFT_eSPI& g, int16_t oy)
{
    if (_state.isFault())
    {
        drawBanner(g, oy, Colors::BANNER_FAULT, "I/O-FEHLER - alle Ausgaenge AUS");
        return;
    }
    if (_selftest.isRunning())
    {
        drawBanner(g, oy, Colors::BANNER_SELFTEST, "SELBSTTEST LAEUFT (Shell)");
        return;
    }

    const bool nsx = (_state.view() == View::NSX);
    // Aktiver Reiter violett, inaktiver fast schwarz: die gewählte Ansicht
    // ist auf einen Blick erkennbar.
    drawButton(g, oy, tabMtzRect(), nsx ? Colors::TAB_INACTIVE : Colors::TAB_ACTIVE,
               "MTZ/NW/NT", 2, false, nsx ? Colors::TAB_INACTIVE_TEXT : Colors::TAB_ACTIVE_TEXT);
    drawButton(g, oy, tabNsxRect(), nsx ? Colors::TAB_ACTIVE : Colors::TAB_INACTIVE,
               "NSX", 2, false, nsx ? Colors::TAB_ACTIVE_TEXT : Colors::TAB_INACTIVE_TEXT);

    if (!nsx)
    {
        // Taste «Alle»: Farbe der gemeinsamen Spannung, grau bei gemischten Spannungen
        Voltage common = Voltage::V24;
        const bool same = _state.allSameVoltage(common);
        char text[20];
        snprintf(text, sizeof(text), "Alle: %s", same ? voltageName(common) : "div.");
        drawButton(g, oy, allRect(), same ? voltageColor(common) : Colors::MIXED,
                   text, 2, true, Colors::TEXT);
    }

    drawButton(g, oy, allOffRect(), Colors::ALL_OFF, "Alle AUS", 2, false, Colors::ALL_OFF_TEXT);
}

void DisplayUi::drawBanner(TFT_eSPI& g, int16_t oy, uint16_t color, const char* text)
{
    g.fillRect(0, 0 - oy, Layout::SCREEN_W, Layout::ROW_Y0 - 4, color);
    g.setTextDatum(MC_DATUM);
    g.setTextColor(Colors::TEXT, color);
    g.drawString(text, Layout::SCREEN_W / 2, (Layout::ROW_Y0 - 4) / 2 - oy, 2);
}

/* ----------------------------------------------------------------------------
   drawRow()
   Eine Zeile = Beschriftung (Kurzzeichen + Funktion), Spannungstaste in der
   Farbe der Spannung und EIN/AUS-Taste (grau = AUS, grün = EIN).
   In der Ansicht NSX zeichnen XF, MX und MCH statt einer eigenen
   Spannungstaste ihren Ausschnitt der gemeinsamen Taste MT.
   ---------------------------------------------------------------------------- */
void DisplayUi::drawRow(TFT_eSPI& g, int16_t oy, uint8_t out)
{
    const Rect row = rowRect(out);

    // Beschriftung
    const OutputLabel label = outputLabel(_state.view(), out);
    g.setTextDatum(TL_DATUM);
    g.setTextColor(Colors::TEXT, Colors::BACKGROUND);
    g.drawString(label.code, Layout::LABEL_X, row.y + 2 - oy, 4);
    g.setTextColor(Colors::TEXT_DIM, Colors::BACKGROUND);
    g.drawString(label.function, Layout::LABEL_X, row.y + 28 - oy, 2);

    // Spannungstaste
    if (motorGroupShown() && _state.isCoupled(out))
    {
        drawMotorVoltage(g, oy);
    }
    else
    {
        const Voltage v = _state.voltage(out);
        drawButton(g, oy, voltageRect(out), voltageColor(v), voltageName(v), 4, true, Colors::TEXT);
    }

    // EIN/AUS-Taste
    const bool on = _state.isOn(out);
    drawButton(g, oy, switchRect(out), on ? Colors::OUT_ON : Colors::OUT_OFF,
               on ? "EIN" : "AUS", 4, false, Colors::TEXT);
}

/* ----------------------------------------------------------------------------
   drawMotorVoltage()
   Gemeinsame Spannungstaste des Motorantriebs MT (Ansicht NSX): oben «MT»,
   darunter die Spannung und das Dreieck der Auswahlliste. Die Taste wird
   immer ganz gezeichnet; das Band schneidet seinen Ausschnitt ab.
   ---------------------------------------------------------------------------- */
void DisplayUi::drawMotorVoltage(TFT_eSPI& g, int16_t oy)
{
    const Rect    r    = motorVoltageRect();
    const Voltage v    = _state.voltage(OUT_XF);
    const uint16_t fill = voltageColor(v);
    const int16_t cx   = r.x + r.w / 2;
    const int16_t cy   = r.y + r.h / 2 - oy;

    g.fillRoundRect(r.x, r.y - oy, r.w, r.h, Layout::RADIUS, fill);
    g.setTextDatum(MC_DATUM);
    g.setTextColor(Colors::TEXT, fill);
    g.drawString("MT", cx, cy - 16, 4);
    g.drawString(voltageName(v), cx - 7, cy + 14, 4);

    const int16_t ax = r.x + r.w - 14;
    const int16_t ay = cy + 11;
    g.fillTriangle(ax, ay, ax + 9, ay, ax + 4, ay + 6, Colors::TEXT);
}

/* ----------------------------------------------------------------------------
   drawModal()
   Start- bzw. Zwischenbildschirm (siehe DisplayUi.h). Wird wie die hohe
   Taste MT immer ganz gezeichnet; jedes Band schneidet seinen Teil ab.
   ---------------------------------------------------------------------------- */
void DisplayUi::drawModal(TFT_eSPI& g, int16_t oy)
{
    const bool    choose = (screen() == Screen::CHOOSE_CABLE);
    const int16_t cx     = Layout::SCREEN_W / 2;

    // Titelband in der Farbe der Reiter
    g.fillRect(0, 0 - oy, Layout::SCREEN_W, 38, Colors::TAB_ACTIVE);
    g.setTextDatum(MC_DATUM);
    g.setTextColor(Colors::TAB_ACTIVE_TEXT, Colors::TAB_ACTIVE);
    g.drawString(choose ? "Pruefkabel waehlen" : "Ansicht wechseln", cx, 20 - oy, 4);

    // Frage
    g.setTextColor(Colors::TEXT, Colors::BACKGROUND);
    if (choose)
    {
        g.drawString("Welches Pruefkabel", cx, 66 - oy, 4);
        g.drawString("ist eingesteckt?",   cx, 96 - oy, 4);
    }
    else
    {
        char line[32];
        snprintf(line, sizeof(line), "%s eingesteckt?", viewName(_state.pendingView()));
        g.drawString("Ist das Pruefkabel fuer", cx, 66 - oy, 4);
        g.drawString(line,                      cx, 96 - oy, 4);
    }

    // Hinweis (bei Störung rot)
    if (_state.isFault())
    {
        g.setTextColor(Colors::BANNER_FAULT, Colors::BACKGROUND);
        g.drawString("I/O-FEHLER - Einschalten gesperrt", cx, 132 - oy, 2);
    }
    else
    {
        g.setTextColor(Colors::TEXT_DIM, Colors::BACKGROUND);
        g.drawString(choose ? "Bis zur Auswahl bleiben alle Ausgaenge AUS."
                            : "Alle Ausgaenge wurden ausgeschaltet.", cx, 132 - oy, 2);
    }

    // Tasten
    if (choose)
    {
        drawButton(g, oy, modalLeftRect(),  Colors::TAB_ACTIVE, "MTZ/NW/NT", 4, false, Colors::TAB_ACTIVE_TEXT);
        drawButton(g, oy, modalRightRect(), Colors::TAB_ACTIVE, "NSX",       4, false, Colors::TAB_ACTIVE_TEXT);
    }
    else
    {
        drawButton(g, oy, modalLeftRect(),  Colors::OUT_OFF, "Abbrechen", 4, false, Colors::TEXT);
        drawButton(g, oy, modalRightRect(), Colors::OUT_ON,  "Ja",        4, false, Colors::TEXT);
    }
}

/* ----------------------------------------------------------------------------
   drawPopup()
   Auswahlliste mit den drei Spannungen in ihren Farben. Die aktuell
   gewählte Spannung ist weiss umrandet. Eine schwarze Fläche mit weisser
   Kontur hebt die Liste deutlich vom Inhalt dahinter ab.
   ---------------------------------------------------------------------------- */
uint8_t DisplayUi::popupHighlight() const
{
    if (_popup == POPUP_ALL)
    {
        Voltage common = Voltage::V24;
        return _state.allSameVoltage(common) ? voltageIndex(common) : 0xFF;
    }
    return voltageIndex(_state.voltage(static_cast<uint8_t>(_popup)));
}

void DisplayUi::drawPopup(TFT_eSPI& g, int16_t oy)
{
    const Rect    f       = popupFrameRect();
    const uint8_t current = popupHighlight();

    g.fillRoundRect(f.x, f.y - oy, f.w, f.h, Layout::RADIUS, Colors::BACKGROUND);
    g.drawRoundRect(f.x, f.y - oy, f.w, f.h, Layout::RADIUS, Colors::FRAME);

    for (uint8_t item = 0; item < VOLTAGE_COUNT; item++)
    {
        const Voltage v = voltageFromIndex(item);
        const Rect    r = popupItemRect(item);
        drawButton(g, oy, { r.x, static_cast<int16_t>(r.y + 1), r.w, static_cast<int16_t>(r.h - 2) },
                   voltageColor(v), voltageName(v), 4, false, Colors::TEXT);
        if (item == current)
        {
            g.drawRoundRect(r.x,     r.y + 1 - oy, r.w,     r.h - 2, Layout::RADIUS, Colors::FRAME);
            g.drawRoundRect(r.x + 1, r.y + 2 - oy, r.w - 2, r.h - 4, Layout::RADIUS, Colors::FRAME);
        }
    }
}

/* ----------------------------------------------------------------------------
   drawButton()
   Gefüllte Taste mit abgerundeten Ecken und zentriertem Text. Optional ein
   kleines Dreieck rechts als Hinweis auf eine Auswahlliste.
   ---------------------------------------------------------------------------- */
void DisplayUi::drawButton(TFT_eSPI& g, int16_t oy, const Rect& r, uint16_t fill,
                           const char* text, uint8_t font, bool arrow, uint16_t textColor)
{
    const int16_t y = r.y - oy;
    g.fillRoundRect(r.x, y, r.w, r.h, Layout::RADIUS, fill);

    int16_t textCenterX = r.x + r.w / 2;
    if (arrow)
    {
        const int16_t ax = r.x + r.w - 14;
        const int16_t ay = y + r.h / 2 - 3;
        g.fillTriangle(ax, ay, ax + 9, ay, ax + 4, ay + 6, textColor);
        textCenterX -= 7;
    }

    g.setTextDatum(MC_DATUM);
    g.setTextColor(textColor, fill);
    g.drawString(text, textCenterX, y + r.h / 2, font);
}
