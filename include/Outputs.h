/*
 * ====================================================
 * 📄 Dateiname: Outputs.h
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Zentrale und EINZIGE Definitionsstelle für:
 * - Logische Ausgänge
 * - Betriebsmodi
 * - Namen für Debug & UI
 *
 * ⚠️ Wichtige Infos:
 * - KEINE Hardwarekenntnis
 * - KEINE Logik
 * - KEINE Seiteneffekte
 *
 * 🧠 Konzept & Überlegungen:
 * - Änderungen hier sollen das Projekt NICHT zerstören
 * - Trennung von Identität (ID) und Bedeutung (Name)
 * - Grundlage für SerialShell & Web-GUI
 *
 * ====================================================
 */

#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <Arduino.h>

/* 🔌 Logische Ausgänge
 * Reihenfolge ist systemweit verbindlich
 */
enum OutputID
{
    OUT_MN_MX2 = 0,
    OUT_MX,
    OUT_XF,
    OUT_MCH,
    OUTPUT_COUNT
};

/* ⚡ Betriebsmodi pro Ausgang
 * Jeder Modus entspricht GENAU EINEM Pin
 */
enum OutputMode
{
    MODE_24V = 0,
    MODE_48V,
    MODE_230V,
    MODE_CUSTOM,
    MODE_COUNT
};

/*
 * 📌 Beschreibung:
 * Liefert den Namen eines logischen Ausgangs.
 *
 * ⚙️ Funktion:
 * - Wird ausschließlich für Anzeigen genutzt
 *
 * 🔁 Ablauf:
 * - Switch über OutputID
 * - Rückgabe eines konstanten Strings
 */
static inline const char* getOutputName(OutputID id)
{
    switch (id)
    {
        case OUT_MN_MX2: return "OUT_MN_MX2";
        case OUT_MX:     return "OUT_MX";
        case OUT_XF:     return "OUT_XF";
        case OUT_MCH:    return "OUT_MCH";
        default:         return "UNKNOWN_OUTPUT";
    }
}

/*
 * 📌 Beschreibung:
 * Liefert den Namen eines Modus.
 *
 * ⚙️ Funktion:
 * - Einheitliche Textdarstellung
 *
 * 🔁 Ablauf:
 * - Switch über OutputMode
 */
static inline const char* getModeName(OutputMode mode)
{
    switch (mode)
    {
        case MODE_24V:    return "24V";
        case MODE_48V:    return "48V";
        case MODE_230V:   return "230V";
        case MODE_CUSTOM: return "CUSTOM";
        default:          return "UNKNOWN_MODE";
    }
}

#endif
