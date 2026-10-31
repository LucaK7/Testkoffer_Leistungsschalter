/* ============================================================================
   Datei:   McpManager.h
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Hardwarenahe Schicht für die beiden Portexpander MCP23008 (-A2 = U15, -A3 = U16),
   die über die Treiber ULN2803 (-A5 = U19, -A6 = U20) und die Klemmen
   J9–J12 die Relais -K1 … -K12 schalten.
   Der McpManager ist die EINZIGE Stelle im Programm, die auf den I²C-Bus
   zugreift.

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Bus starten und beide Bausteine in einen sicheren Zustand versetzen
     (alle Pins Ausgang, alle Relais aus)
   - Einen vorgegebenen Sollzustand schreiben und durch Zurücklesen prüfen
   - Zyklische Kontrolle: Stimmen die Register noch mit dem Sollzustand?
   - Selbstständige Reparatur nach einer Störung (Bus freitakten,
     Bausteine neu initialisieren, Sollzustand neu schreiben)
   - Fehlerstatistik für Shell und Display

   Funktionsprinzip (warum so?)
   ----------------------------------------------------------------------------
   Beim Schalten von 230 VAC können Störimpulse den I²C-Bus verfälschen oder
   einen MCP23008 zurücksetzen (danach sind alle Pins Eingänge und kein
   Relais schaltet mehr). Deshalb gilt:
     1. Der Sollzustand wird im Speicher geführt (_target) und pro Baustein
        immer als ganzes Byte in EINEM Zugriff geschrieben. Es wird nie ein
        Wert gelesen, verändert und zurückgeschrieben – ein falsch gelesener
        Wert kann so keine Relais einschalten.
     2. Jeder Schreibzugriff wird durch Zurücklesen von IODIR und OLAT
        bestätigt.
     3. Weicht etwas ab, wird repariert. Gelingt das nicht, meldet die
        Funktion FAILED und der StateManager schaltet alles aus.
     4. Vor jedem Schreiben wird anhand der Tabelle relayPin() geprüft, dass
        pro Ausgang höchstens ein Relais aktiv ist und unbelegte Pins LOW
        bleiben (letzte Schutzschicht gegen Programmierfehler).

   Wichtige Hinweise
   ----------------------------------------------------------------------------
   - Keine Logik über Ausgänge, Ansichten oder Verriegelungen – das ist
     Aufgabe des StateManagers.
   - Keine Ausgaben auf Serial; Ereignisse werden über lastEvent()/eventId()
     bereitgestellt und von der Shell ausgegeben.
   ============================================================================ */

#pragma once
#include <Arduino.h>

class McpManager
{
public:
    // Ergebnis eines Schreib- oder Kontrollvorgangs
    enum class Result : uint8_t
    {
        OK,         // alles in Ordnung
        REPAIRED,   // Abweichung erkannt und erfolgreich behoben
        FAILED      // Baustein nicht erreichbar bzw. nicht reparierbar
    };

    // Bus starten und beide MCP23008 mit «alle Relais aus» initialisieren.
    // Gibt true zurück, wenn beide Bausteine korrekt antworten.
    bool begin();

    // Sollzustand schreiben. bitsA2 gehört zum MCP -A2/U15 (0x20, J9/J10),
    // bitsA3 zum MCP -A3/U16 (0x21, J11/J12). Bit n = GPn.
    Result writeOutputs(uint8_t bitsA2, uint8_t bitsA3);

    // Zyklische Kontrolle: Register zurücklesen und mit Sollzustand vergleichen.
    Result check();

    // Prüft einen Sollzustand: pro Ausgang höchstens ein Relais EIN und
    // keine Pins gesetzt, die laut relayPin() keinem Relais zugeordnet sind.
    static bool isPlausible(uint8_t bitsA2, uint8_t bitsA3);

    // --- Abfragen für Shell und Display -------------------------------------
    bool        isHealthy()    const { return _healthy; }
    uint32_t    errorCount()   const { return _errorCount; }    // erkannte Abweichungen/Busfehler
    uint32_t    repairCount()  const { return _repairCount; }   // erfolgreiche Reparaturen
    uint32_t    eventId()      const { return _eventId; }       // zählt bei jedem neuen Ereignis hoch
    const char* lastEvent()    const { return _lastEvent; }     // Klartext des letzten Ereignisses
    void        resetStatistics();

private:
    // Einzelne Registerzugriffe (je ein I²C-Telegramm), mit Fehlerprüfung
    bool writeRegister(uint8_t addr, uint8_t reg, uint8_t value);
    bool readRegister(uint8_t addr, uint8_t reg, uint8_t& value);

    // Baustein vollständig konfigurieren bzw. seinen Zustand prüfen
    bool initChip(uint8_t addr, uint8_t olat);
    bool verifyChip(uint8_t addr, uint8_t expectedOlat);
    bool verifyAll();

    // Störungsbehandlung
    Result handleDeviation(const char* reason);
    bool   repair();
    void   recoverBus();
    void   startBus();

    void   logEvent(const char* text);

    uint8_t  _target[2]   = { 0x00, 0x00 };   // Sollzustand -A2, -A3 (0x00 = alle Relais aus)
    bool     _healthy     = false;
    uint32_t _errorCount  = 0;
    uint32_t _repairCount = 0;
    uint32_t _eventId     = 0;
    char     _lastEvent[64] = "";
};
