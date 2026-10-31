/*
 * ====================================================
 * 📄 Dateiname: McpManager.h
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Abstraktionsebene für MCP23008 I²C-Portexpander.
 *
 * ⚠️ Wichtige Infos:
 * - KEINE Zustände
 * - KEINE Sicherheitslogik
 * - KEINE Delays
 *
 * 🧠 Konzept & Überlegungen:
 * - MCPManager ist die unterste Schicht
 * - Kennt nur Hardware, keine Bedeutung
 * - Jeder Schreibzugriff ist gezielt & isoliert
 *
 * 🔒 Sicherheitsgedanke:
 * - Keine Sammelschreiboperationen
 * - Kein Überschreiben fremder Pins
 *
 * ====================================================
 */

#ifndef MCPMANAGER_H
#define MCPMANAGER_H

#include <Arduino.h>
#include <Adafruit_MCP23008.h>

#include "Outputs.h"
#include "config.h"

class McpManager
{
public:
    /*
     * 📌 Beschreibung:
     * Initialisiert beide MCP23008 Bausteine.
     *
     * ⚙️ Funktion:
     * - Startet I²C-Kommunikation
     * - Setzt alle Pins als OUTPUT
     * - Erzwingt definierten LOW-Zustand
     *
     * 🔁 Ablauf:
     * - MCP1 starten
     * - MCP2 starten
     * - Schleife über alle 8 Pins
     */
    void begin();

    /*
     * 📌 Beschreibung:
     * Aktiviert oder deaktiviert einen Modus-Pin.
     *
     * ⚙️ Funktion:
     * - Kapselt die Zuordnung Modus → Pin
     *
     * 🔁 Ablauf:
     * - Modus wird in Pinindex umgewandelt
     * - Übergabe an setPin()
     */
    void setModePin(OutputID output, OutputMode mode, bool state);

    /*
     * 📌 Beschreibung:
     * Schaltet ALLE Pins eines Ausgangs aus.
     *
     * ⚙️ Funktion:
     * - Sicherheitsreset eines Ausgangs
     *
     * 🔁 Ablauf:
     * - Vier Pins gezielt auf LOW setzen
     */
    void allPinsOff(OutputID output);

private:
    Adafruit_MCP23008 _mcp1;
    Adafruit_MCP23008 _mcp2;

    /*
     * 📌 Beschreibung:
     * Setzt einen einzelnen MCP-Pin.
     *
     * ⚙️ Funktion:
     * - Niedrigste Schreiboperation
     *
     * 🔁 Ablauf:
     * - MCP bestimmen
     * - Pin berechnen
     * - digitalWrite ausführen
     */
    void setPin(OutputID output, uint8_t pin, bool state);

    /*
     * 📌 Beschreibung:
     * Liefert den zuständigen MCP.
     *
     * ⚙️ Funktion:
     * - Routing je Ausgang
     *
     * 🔁 Ablauf:
     * - Vergleich der OutputID
     */
    Adafruit_MCP23008& getMcp(OutputID output);
};

#endif
