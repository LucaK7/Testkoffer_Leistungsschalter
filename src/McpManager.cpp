/* ============================================================================
   Datei:   McpManager.cpp
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Umsetzung der in McpManager.h beschriebenen Hardwareschicht.
   Ablauf eines Schreibvorgangs (writeOutputs):

       Sollzustand plausibel? ──nein──> alles aus, FAILED
              │ja
       OLAT beider Bausteine schreiben
              │
       IODIR und OLAT zurücklesen ──stimmt──> OK
              │weicht ab / Busfehler
       bis zu 3 × reparieren:  Bus freitakten → Bus neu starten →
                               Bausteine neu initialisieren → prüfen
              │gelungen ──> REPAIRED
              │misslungen
            FAILED  (StateManager schaltet daraufhin alles aus)
   ============================================================================ */

#include "McpManager.h"
#include <Wire.h>
#include "Config.h"
#include "Outputs.h"

/* ============================================================================
   Öffentliche Funktionen
   ============================================================================ */

/* ----------------------------------------------------------------------------
   begin()
   Startet den Bus und schreibt in beide Bausteine «alle Relais aus».
   Wird als ALLERERSTES in setup() aufgerufen, damit nach einem Neustart des
   ESP32 (z. B. durch den Watchdog) sicher kein Relais angezogen bleibt –
   die MCP23008 behalten ihre Ausgangszustände nämlich, wenn nur der ESP32
   neu startet.
   ---------------------------------------------------------------------------- */
bool McpManager::begin()
{
    startBus();

    _target[0] = 0x00;
    _target[1] = 0x00;

    bool ok = initChip(I2cConfig::ADDR_MCP_A2,  _target[0])
           && initChip(I2cConfig::ADDR_MCP_A3, _target[1])
           && verifyAll();

    if (!ok)
    {
        // Erster Versuch misslungen (z. B. Bus beim Einschalten blockiert):
        // regulär über die Reparaturfunktion nochmals versuchen.
        ok = (handleDeviation("Start: MCP antwortet nicht") != Result::FAILED);
    }

    _healthy = ok;
    logEvent(ok ? "MCP23008 -A2/-A3 bereit, alle Relais aus"
                : "FEHLER: MCP23008 beim Start nicht erreichbar");
    return ok;
}

/* ----------------------------------------------------------------------------
   writeOutputs()
   Übernimmt einen neuen Sollzustand und schreibt ihn in beide Bausteine.
   ---------------------------------------------------------------------------- */
McpManager::Result McpManager::writeOutputs(uint8_t bitsA2, uint8_t bitsA3)
{
    // Letzte Schutzschicht: Ein unzulässiger Sollzustand (zwei Spannungen auf
    // demselben Ausgang) darf die Relais nie erreichen.
    if (!isPlausible(bitsA2, bitsA3))
    {
        _target[0] = 0x00;
        _target[1] = 0x00;
        writeRegister(I2cConfig::ADDR_MCP_A2,  Mcp23008Reg::OLAT, 0x00);
        writeRegister(I2cConfig::ADDR_MCP_A3, Mcp23008Reg::OLAT, 0x00);
        _errorCount++;
        logEvent("FEHLER: unplausibler Sollzustand, alle Relais aus");
        return Result::FAILED;
    }

    _target[0] = bitsA2;
    _target[1] = bitsA3;

    // Je Baustein genau ein Schreibtelegramm: Register OLAT = ganzes Byte.
    bool ok = writeRegister(I2cConfig::ADDR_MCP_A2,  Mcp23008Reg::OLAT, _target[0])
           && writeRegister(I2cConfig::ADDR_MCP_A3, Mcp23008Reg::OLAT, _target[1])
           && verifyAll();

    if (ok)
    {
        _healthy = true;
        return Result::OK;
    }
    return handleDeviation("Schreiben/Kontrolle fehlgeschlagen");
}

/* ----------------------------------------------------------------------------
   check()
   Wird vom StateManager alle 100 ms aufgerufen. Erkennt insbesondere einen
   durch Störung zurückgesetzten Baustein (IODIR = 0xFF statt 0x00).
   ---------------------------------------------------------------------------- */
McpManager::Result McpManager::check()
{
    if (verifyAll())
    {
        _healthy = true;
        return Result::OK;
    }
    return handleDeviation("Kontrolle: Register weichen ab");
}

/* ----------------------------------------------------------------------------
   isPlausible()
   Prüft den Sollzustand anhand der Zuordnungstabelle relayPin():
   1. Pro Ausgang darf höchstens EIN Relais (eine Spannung) gesetzt sein,
      sonst lägen zwei Prüfspannungen gleichzeitig auf demselben Ausgang.
   2. Pins, die keinem Relais zugeordnet sind (GP3, GP7 = Klemmen-Pin 4 «Custom»),
      müssen LOW sein.
   ---------------------------------------------------------------------------- */
bool McpManager::isPlausible(uint8_t bitsA2, uint8_t bitsA3)
{
    const uint8_t bits[2] = { bitsA2, bitsA3 };
    uint8_t used[2] = { 0x00, 0x00 };

    for (uint8_t out = 0; out < OUTPUT_COUNT; out++)
    {
        uint8_t active = 0;
        for (uint8_t v = 0; v < VOLTAGE_COUNT; v++)
        {
            const RelayPin r    = relayPin(out, voltageFromIndex(v));
            const uint8_t  mask = static_cast<uint8_t>(1u << r.pin);
            used[r.chip] |= mask;
            if (bits[r.chip] & mask)
            {
                active++;
            }
        }
        if (active > 1)
        {
            return false;   // zwei Spannungen auf demselben Ausgang
        }
    }
    return ((bits[0] & ~used[0]) == 0) && ((bits[1] & ~used[1]) == 0);
}

void McpManager::resetStatistics()
{
    _errorCount  = 0;
    _repairCount = 0;
    logEvent("Fehlerstatistik zurueckgesetzt");
}

/* ============================================================================
   Registerzugriffe
   ============================================================================ */

/* ----------------------------------------------------------------------------
   writeRegister()
   Ein Telegramm: Adresse, Registernummer, Wert. endTransmission() liefert 0,
   wenn der Baustein alle Bytes quittiert (ACK) hat.
   ---------------------------------------------------------------------------- */
bool McpManager::writeRegister(uint8_t addr, uint8_t reg, uint8_t value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    return (Wire.endTransmission() == 0);
}

/* ----------------------------------------------------------------------------
   readRegister()
   Zuerst die Registernummer senden (ohne Stopp-Bedingung, «repeated start»),
   dann genau ein Byte lesen. Jeder Fehler wird als false gemeldet – es wird
   nie ein Ersatzwert wie 0xFF zurückgegeben.
   ---------------------------------------------------------------------------- */
bool McpManager::readRegister(uint8_t addr, uint8_t reg, uint8_t& value)
{
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0)
    {
        return false;
    }
    if (Wire.requestFrom(addr, static_cast<uint8_t>(1)) != 1)
    {
        return false;
    }
    int data = Wire.read();
    if (data < 0)
    {
        return false;
    }
    value = static_cast<uint8_t>(data);
    return true;
}

/* ----------------------------------------------------------------------------
   initChip()
   Vollständige Konfiguration eines MCP23008. Reihenfolge wichtig:
   zuerst OLAT (Sollpegel), dann IODIR (Pins zu Ausgängen machen). So gehen
   die Pins direkt mit dem richtigen Pegel in Betrieb und kein Relais zuckt.
   ---------------------------------------------------------------------------- */
bool McpManager::initChip(uint8_t addr, uint8_t olat)
{
    return writeRegister(addr, Mcp23008Reg::OLAT,    olat)
        && writeRegister(addr, Mcp23008Reg::IPOL,    0x00)   // keine Invertierung
        && writeRegister(addr, Mcp23008Reg::GPINTEN, 0x00)   // keine Interrupts
        && writeRegister(addr, Mcp23008Reg::GPPU,    0x00)   // keine Pull-ups (Ausgänge)
        && writeRegister(addr, Mcp23008Reg::IODIR,   0x00);  // alle 8 Pins = Ausgang
}

/* ----------------------------------------------------------------------------
   verifyChip()
   Liest IODIR und OLAT zurück. Erwartet: IODIR = 0x00 (alles Ausgänge) und
   OLAT = Sollzustand. Ein zurückgesetzter Baustein hat IODIR = 0xFF.
   ---------------------------------------------------------------------------- */
bool McpManager::verifyChip(uint8_t addr, uint8_t expectedOlat)
{
    uint8_t iodir = 0xFF;
    uint8_t olat  = 0x00;

    if (!readRegister(addr, Mcp23008Reg::IODIR, iodir)) return false;
    if (!readRegister(addr, Mcp23008Reg::OLAT,  olat))  return false;

    return (iodir == 0x00) && (olat == expectedOlat);
}

bool McpManager::verifyAll()
{
    return verifyChip(I2cConfig::ADDR_MCP_A2,  _target[0])
        && verifyChip(I2cConfig::ADDR_MCP_A3, _target[1]);
}

/* ============================================================================
   Störungsbehandlung
   ============================================================================ */

/* ----------------------------------------------------------------------------
   handleDeviation()
   Zählt den Fehler und versucht bis zu REPAIR_ATTEMPTS-mal zu reparieren.
   ---------------------------------------------------------------------------- */
McpManager::Result McpManager::handleDeviation(const char* reason)
{
    _errorCount++;

    for (uint8_t attempt = 1; attempt <= I2cConfig::REPAIR_ATTEMPTS; attempt++)
    {
        if (repair())
        {
            _repairCount++;
            _healthy = true;
            char text[64];
            snprintf(text, sizeof(text), "%s -> repariert (Versuch %u)", reason, attempt);
            logEvent(text);
            return Result::REPAIRED;
        }
    }

    _healthy = false;
    char text[64];
    snprintf(text, sizeof(text), "FEHLER: %s, nicht reparierbar", reason);
    logEvent(text);
    return Result::FAILED;
}

/* ----------------------------------------------------------------------------
   repair()
   Bus freitakten, neu starten, beide Bausteine mit dem aktuellen Sollzustand
   neu initialisieren und das Ergebnis prüfen.
   ---------------------------------------------------------------------------- */
bool McpManager::repair()
{
    recoverBus();
    return initChip(I2cConfig::ADDR_MCP_A2,  _target[0])
        && initChip(I2cConfig::ADDR_MCP_A3, _target[1])
        && verifyAll();
}

/* ----------------------------------------------------------------------------
   recoverBus()
   Hält ein Baustein nach einer Störung die Datenleitung SDA auf LOW, ist der
   Bus blockiert. Abhilfe nach I²C-Spezifikation: bis zu 9 Taktimpulse auf
   SCL, bis der Baustein SDA freigibt, danach eine Stopp-Bedingung erzeugen.
   Die Pins werden dafür kurz von Hand (Open-Drain) angesteuert.
   ---------------------------------------------------------------------------- */
void McpManager::recoverBus()
{
    Wire.end();

    pinMode(Pins::I2C_SDA, INPUT_PULLUP);
    pinMode(Pins::I2C_SCL, OUTPUT_OPEN_DRAIN);
    digitalWrite(Pins::I2C_SCL, HIGH);
    delayMicroseconds(5);

    for (uint8_t i = 0; i < 9; i++)
    {
        digitalWrite(Pins::I2C_SCL, LOW);
        delayMicroseconds(5);
        digitalWrite(Pins::I2C_SCL, HIGH);
        delayMicroseconds(5);
    }

    // Stopp-Bedingung: SDA geht bei SCL = HIGH von LOW auf HIGH
    pinMode(Pins::I2C_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(Pins::I2C_SDA, LOW);
    delayMicroseconds(5);
    digitalWrite(Pins::I2C_SCL, HIGH);
    delayMicroseconds(5);
    digitalWrite(Pins::I2C_SDA, HIGH);
    delayMicroseconds(5);

    startBus();
}

/* ----------------------------------------------------------------------------
   startBus()
   Startet die I²C-Schnittstelle des ESP32 auf den Pins gemäss Schema.
   ---------------------------------------------------------------------------- */
void McpManager::startBus()
{
    Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL, I2cConfig::CLOCK_HZ);
    Wire.setTimeOut(I2cConfig::TIMEOUT_MS);
}

void McpManager::logEvent(const char* text)
{
    strncpy(_lastEvent, text, sizeof(_lastEvent) - 1);
    _lastEvent[sizeof(_lastEvent) - 1] = '\0';
    _eventId++;
}
