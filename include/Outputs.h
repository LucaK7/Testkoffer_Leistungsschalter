/* ============================================================================
   Datei:   Outputs.h
   Projekt: SE Testkoffer – Steuerung V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Gemeinsame «Sprache» aller Programmteile: Welche Ausgänge gibt es, welche
   Spannungen, welche Ansichten (Schaltertypen) und wie heissen sie auf dem
   Display und in der Shell.

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Nummerierung der vier Ausgänge (Index 0 … 3 = Reihenfolge auf dem
     Display von oben nach unten und Nummer 1 … 4 in der Shell)
   - Die drei Prüfspannungen 24 VDC, 48 VDC, 230 VAC
   - Zuordnung Ausgang + Spannung -> Relais -> MCP23008-Pin (EPLAN-Schema)
   - Die zwei Ansichten «MTZ/NW/NT» und «NSX»
   - Beschriftungen und Farben (reine Daten, keine Logik)

   Wichtige Hinweise
   ----------------------------------------------------------------------------
   - Beide Ansichten verwenden DIESELBEN vier Relaisgruppen; nur die
     Beschriftung und die Kopplungsregel (NSX) unterscheiden sich:
         Index | Klemme / EPLAN | MTZ/NW/NT         | NSX (Motorantrieb MT)
         0     | J9  MN / MX2   | MN/MX2 Unterspg.  | MN/MX2 Unterspg.
         1     | J11 XF         | XF     Ein        | XF     EIN (A2)
         2     | J10 MX1        | MX     Aus        | MX     AUS (A4)
         3     | J12 MCH        | MCH    Motor      | MCH    Motor (B4)
   - NSX (Test am Schalter bestätigt): B4/A1 speist den Motorantrieb; beim
     Zuschalten spannt der Motor die Feder. A2 schaltet EIN, A4 schaltet AUS.
     Bleibt B4 unter Spannung, spannt der Motor nach dem Ausschalten sofort
     neu. A2, A4 und B4 haben den gemeinsamen Rückleiter A1; im NSX-Prüfkabel
     sind die Rückleiter der Ausgänge XF, MX und MCH deshalb gebrückt.
   - MTZ/NW/NT: Der Motor MCH spannt nur die Einschaltfeder; eingeschaltet
     wird separat über die Einschaltspule XF.
   - Die Zuordnung zu Relais und MCP-Pins steht ausschliesslich in der
     Tabelle relayPin() weiter unten. Wird die Verdrahtung geändert, ist
     nur diese Tabelle anzupassen.
   ============================================================================ */

#pragma once
#include <Arduino.h>
#include "Config.h"

/* ----------------------------------------------------------------------------
   Ausgänge
   ---------------------------------------------------------------------------- */
constexpr uint8_t OUTPUT_COUNT = 4;

constexpr uint8_t OUT_MN  = 0;   // MN/MX2: Unterspannungs- bzw. 2. Arbeitsstromauslöser
constexpr uint8_t OUT_XF  = 1;   // XF: Einschaltspule          (NSX: A2 «EIN»)
constexpr uint8_t OUT_MX  = 2;   // MX1: Ausschaltspule         (NSX: A4 «AUS»)
constexpr uint8_t OUT_MCH = 3;   // MCH: Federspannmotor        (NSX: B4 «Motor»)

// Verriegelungspaar: Ein- und Ausschaltspule dürfen nie gleichzeitig EIN sein.
constexpr uint8_t OUT_COIL_OFF = OUT_MX;
constexpr uint8_t OUT_COIL_ON  = OUT_XF;

/* ----------------------------------------------------------------------------
   Prüfspannungen
   ---------------------------------------------------------------------------- */
enum class Voltage : uint8_t
{
    V24  = 0,   // 24 VDC
    V48  = 1,   // 48 VDC
    V230 = 2    // 230 VAC
};
constexpr uint8_t VOLTAGE_COUNT = 3;

inline Voltage voltageFromIndex(uint8_t i) { return static_cast<Voltage>(i); }
inline uint8_t voltageIndex(Voltage v)     { return static_cast<uint8_t>(v); }

inline const char* voltageName(Voltage v)
{
    static const char* const NAMES[VOLTAGE_COUNT] = { "24 V", "48 V", "230 V" };
    return NAMES[voltageIndex(v)];
}

inline uint16_t voltageColor(Voltage v)
{
    static const uint16_t COLORS[VOLTAGE_COUNT] = { Colors::V24, Colors::V48, Colors::V230 };
    return COLORS[voltageIndex(v)];
}

/* ----------------------------------------------------------------------------
   Ansichten (Schaltertypen)
   ---------------------------------------------------------------------------- */
enum class View : uint8_t
{
    MTZ_NW_NT = 0,   // Leistungsschalter MasterPact MTZ, NW, NT
    NSX       = 1    // Leistungsschalter ComPact NSX mit Motormechanismus MT
};

inline const char* viewName(View v)
{
    return (v == View::NSX) ? "NSX" : "MTZ/NW/NT";
}

/* ----------------------------------------------------------------------------
   Beschriftung eines Ausgangs: Kurzzeichen + Funktion
   ---------------------------------------------------------------------------- */
struct OutputLabel
{
    const char* code;       // Kurzzeichen, gross dargestellt (z. B. «XF»)
    const char* function;   // Funktion, klein darunter (z. B. «Ein»)
};

inline OutputLabel outputLabel(View view, uint8_t out)
{
    static const OutputLabel MTZ[OUTPUT_COUNT] = {
        { "MN/MX2", "Unterspg."  },
        { "XF",     "Ein"        },
        { "MX",     "Aus"        },
        { "MCH",    "Motor"      }
    };
    static const OutputLabel NSX[OUTPUT_COUNT] = {
        { "MN/MX2", "Unterspg."  },
        { "XF",     "EIN (A2)"   },
        { "MX",     "AUS (A4)"   },
        { "MCH",    "Motor (B4)" }
    };
    return (view == View::NSX) ? NSX[out] : MTZ[out];
}

/* ----------------------------------------------------------------------------
   Zuordnung Ausgang + Spannung -> Relais -> MCP23008-Pin
   ----------------------------------------------------------------------------
   Gemäss Steuerprint V2 (Klemmen J9–J12) und dem daran angepassten
   EPLAN-Schema. Jeder MCP-Pin führt über einen ULN2803-Kanal auf genau
   einen Pin der Klemmen J9–J12 (GP0 -> In1 -> Out1 -> Klemme Pin 1 usw.).

     chip 0 = MCP -A2/U15 (0x20) -> ULN -A5/U19 -> J9 (GP0–GP3), J10 (GP4–GP7)
     chip 1 = MCP -A3/U16 (0x21) -> ULN -A6/U20 -> J11 (GP0–GP3), J12 (GP4–GP7)

     Ausgang   | 24 VDC           | 48 VDC           | 230 VAC
     MN / MX2  | K12 J9.1  (0,0)  | K11 J9.2  (0,1)  | K10 J9.3  (0,2)
     XF        | K6  J11.1 (1,0)  | K5  J11.2 (1,1)  | K4  J11.3 (1,2)
     MX1       | K9  J10.1 (0,4)  | K8  J10.2 (0,5)  | K7  J10.3 (0,6)
     MCH       | K3  J12.1 (1,4)  | K2  J12.2 (1,5)  | K1  J12.3 (1,6)
   Pin 4 jeder Klemme (GP3/GP7, Bestückungsdruck «Custom») ist unbelegt.
   ---------------------------------------------------------------------------- */
struct RelayPin
{
    uint8_t     chip;    // 0 = MCP -A2/U15 (0x20), 1 = MCP -A3/U16 (0x21)
    uint8_t     pin;     // GP0 … GP7
    const char* name;    // Relaisbezeichnung im Schema
};

inline RelayPin relayPin(uint8_t out, Voltage v)
{
    static const RelayPin MAP[OUTPUT_COUNT][VOLTAGE_COUNT] = {
        //   24 VDC            48 VDC            230 VAC
        { { 0, 0, "K12" }, { 0, 1, "K11" }, { 0, 2, "K10" } },   // 0 MN / MX2  (J9)
        { { 1, 0, "K6"  }, { 1, 1, "K5"  }, { 1, 2, "K4"  } },   // 1 XF        (J11)
        { { 0, 4, "K9"  }, { 0, 5, "K8"  }, { 0, 6, "K7"  } },   // 2 MX1       (J10)
        { { 1, 4, "K3"  }, { 1, 5, "K2"  }, { 1, 6, "K1"  } }    // 3 MCH       (J12)
    };
    return MAP[out][voltageIndex(v)];
}
