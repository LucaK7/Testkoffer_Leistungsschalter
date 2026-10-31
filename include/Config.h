/* ============================================================================
   Datei:   Config.h
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Zentrale Sammlung aller festen Werte des Programms: Pinbelegung,
   I²C-Adressen, Zeiten und Farben. Alle anderen Dateien lesen diese Werte
   nur, sie definieren keine eigenen «magischen Zahlen».

   Zuständigkeiten
   ----------------------------------------------------------------------------
   - Pinbelegung gemäss Schema «SE Testkoffer – Steuerung V2»
     (die Display-Pins stehen in platformio.ini, weil TFT_eSPI sie dort
     beim Übersetzen erwartet)
   - I²C-Adressen und Registeradressen der Portexpander MCP23008
   - Zeitkonstanten (Sicherheitspause, Kontrollintervall, Touch)
   - Farben der Bedienoberfläche

   Wichtige Hinweise
   ----------------------------------------------------------------------------
   - Wer einen Wert ändern will (z. B. Sicherheitspause), ändert ihn NUR hier.
   - Die Werte sind in Namensräume (namespace) gruppiert, damit sofort
     ersichtlich ist, wozu ein Wert gehört (z. B. Pins::I2C_SDA).
   ============================================================================ */

#pragma once
#include <Arduino.h>

/* ----------------------------------------------------------------------------
   Pinbelegung ESP32 (LOLIN D32, U14) gemäss Schema V2
   ----------------------------------------------------------------------------
   Display/Touch-SPI (IO5, IO16, IO17, IO18, IO19, IO23, IO4) -> platformio.ini
   Nicht verwendet im Grundaufbau:
     IO39 (VN)          LCD_TP_Irq  -> Display Pin 11 (Touch wird zyklisch abgefragt)
     IO32               LCD_Button0 -> Display Pin 12 (Taster BTN0, ohne Funktion)
     IO33, IO34         auf dem Print nicht angeschlossen (BTN1, BTN2 n.c.)
     IO25, IO26, IO27   Modbus/RS-485 (U10 nicht bestückt)
   ---------------------------------------------------------------------------- */
namespace Pins
{
    constexpr uint8_t I2C_SDA     = 21;   // IO21 -> SDA MCP23008 -A2/U15, -A3/U16
    constexpr uint8_t I2C_SCL     = 22;   // IO22 -> SCL MCP23008 -A2/U15, -A3/U16

    // IO32 ist gemäss Schema (Blatt 3) und Steuerprint V2 mit dem Taster
    // BTN0 des Displays (Pin 12) verbunden. Der Pin wird nur als Eingang mit
    // Pull-up definiert, damit er nie als Ausgang gegen den Taster treibt.
    constexpr uint8_t LCD_BUTTON0 = 32;   // IO32 <- Display Pin 12 (BTN0)
}

/* ----------------------------------------------------------------------------
   I²C-Bus und Portexpander MCP23008
   ----------------------------------------------------------------------------
   Gemäss Steuerprint V2 und angepasstem EPLAN-Schema:
     -A2/U15 MCP23008 (0x20) -> -A5/U19 ULN2803 -> J9 (MN/MX2), J10 (MX1)
     -A3/U16 MCP23008 (0x21) -> -A6/U20 ULN2803 -> J11 (XF),    J12 (MCH)
   Pin 1/2/3 jeder Klemme = 24 V / 48 V / 230 V, Pin 4 (Custom) unbelegt.
   Welcher MCP-Pin welches Relais schaltet, steht in der Tabelle relayPin()
   in Outputs.h.
   ---------------------------------------------------------------------------- */
namespace I2cConfig
{
    constexpr uint32_t CLOCK_HZ    = 100000;   // Standard-Mode 100 kHz (störfester als 400 kHz)
    constexpr uint16_t TIMEOUT_MS  = 20;       // Abbruch eines hängenden Buszugriffs nach 20 ms

    constexpr uint8_t  ADDR_MCP_A2 = 0x20;   // -A2/U15: J9, J10 (Relais K7–K12)
    constexpr uint8_t  ADDR_MCP_A3 = 0x21;   // -A3/U16: J11, J12 (Relais K1–K6)

    constexpr uint8_t  REPAIR_ATTEMPTS = 3;    // Reparaturversuche, bevor ein Fehler als dauerhaft gilt
}

namespace Mcp23008Reg
{
    constexpr uint8_t IODIR   = 0x00;   // Richtung: 1 = Eingang, 0 = Ausgang
    constexpr uint8_t IPOL    = 0x01;   // Polarität der Eingänge
    constexpr uint8_t GPINTEN = 0x02;   // Interrupt bei Pegelwechsel
    constexpr uint8_t GPPU    = 0x06;   // interne Pull-ups
    constexpr uint8_t OLAT    = 0x0A;   // Ausgangsregister (Sollpegel der Pins)
}

/* ----------------------------------------------------------------------------
   Zeiten
   ---------------------------------------------------------------------------- */
namespace Timing
{
    // Mindestzeit, die ein Ausgang AUS sein muss, bevor er (mit gleicher oder
    // anderer Spannung) wieder EIN geht. Gilt auch zwischen Ein- und
    // Ausschaltspule. Die Relais G7L fallen in < 20 ms ab, 200 ms gibt Reserve.
    constexpr uint32_t SAFETY_PAUSE_MS = 200;

    // Intervall, in dem die Register der MCP23008 zurückgelesen werden.
    constexpr uint32_t MCP_CHECK_INTERVAL_MS = 100;

    // Im Störungszustand: Abstand der Versuche, die Hardware wieder zu erreichen.
    constexpr uint32_t FAULT_RETRY_MS = 1000;

    // Abfrageintervall des Touch-Controllers und Sperrzeit nach einem Tipp.
    constexpr uint32_t TOUCH_POLL_MS     = 30;
    constexpr uint32_t TOUCH_LOCKOUT_MS  = 250;

    // Watchdog: hängt das Programm länger, startet der ESP32 neu.
    constexpr uint32_t WATCHDOG_S = 5;

    // Selbsttest: Einschaltdauer je Relais und Pause dazwischen.
    constexpr uint32_t SELFTEST_ON_MS    = 400;
    constexpr uint32_t SELFTEST_PAUSE_MS = 300;
    // Zeit, um den Start des Selbsttests in der Shell zu bestätigen.
    constexpr uint32_t SELFTEST_CONFIRM_MS = 10000;
    // Zeit, um einen in der Shell angeforderten Ansichtswechsel zu bestätigen.
    constexpr uint32_t VIEW_CONFIRM_MS = 10000;
}

/* ----------------------------------------------------------------------------
   Display
   ---------------------------------------------------------------------------- */
namespace DisplayConfig
{
    // 1 = Querformat (320 x 240). Steht das Bild auf dem Kopf: 3 eintragen
    // und anschliessend in der Shell «kalibrieren» ausführen.
    constexpr uint8_t ROTATION = 1;

    // Mindestdruck für eine gültige Berührung (TFT_eSPI, Standard 600).
    constexpr uint16_t TOUCH_THRESHOLD = 600;
}

/* ----------------------------------------------------------------------------
   Farben (RGB565, wie sie das Display erwartet)
   ---------------------------------------------------------------------------- */
constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

namespace Colors
{
    constexpr uint16_t BACKGROUND = rgb565(  0,   0,   0);   // Schwarz
    constexpr uint16_t TEXT       = rgb565(255, 255, 255);   // Weiss
    constexpr uint16_t TEXT_DIM   = rgb565(170, 170, 170);   // Hellgrau (Untertitel)

    constexpr uint16_t V24  = rgb565( 20, 100, 220);   // 24 V  = Blau
    constexpr uint16_t V48  = rgb565(235, 140,   0);   // 48 V  = Orange
    constexpr uint16_t V230 = rgb565(200,  20,  20);   // 230 V = Rot (Gefahr)
    constexpr uint16_t MIXED = rgb565( 90,  90,  90);  // «Alle» bei gemischten Spannungen

    constexpr uint16_t OUT_OFF = rgb565( 80,  80,  80);   // Ausgang AUS = schlichtes Grau
    constexpr uint16_t OUT_ON  = rgb565(  0, 170,  60);   // Ausgang EIN = Grün

    // Taste «Alle AUS»: Gelb ist keiner Spannung und keinem Zustand zugeordnet
    // (Orange = 48 V, Rot = 230 V, Grau = AUS) und hebt sich deutlich ab.
    constexpr uint16_t ALL_OFF      = rgb565(255, 210,   0);   // Gelb
    constexpr uint16_t ALL_OFF_TEXT = rgb565(  0,   0,   0);   // Schwarz

    // Reiter der Ansicht: aktiver Reiter violett (keiner Spannung und keinem
    // Zustand zugeordnet), inaktiver Reiter fast schwarz mit grauer Schrift.
    constexpr uint16_t TAB_ACTIVE        = rgb565(150,  60, 220);   // Violett
    constexpr uint16_t TAB_ACTIVE_TEXT   = rgb565(255, 255, 255);   // Weiss
    constexpr uint16_t TAB_INACTIVE      = rgb565( 35,  35,  35);   // fast Schwarz
    constexpr uint16_t TAB_INACTIVE_TEXT = rgb565(140, 140, 140);   // Grau
    constexpr uint16_t FRAME        = rgb565(255, 255, 255);

    constexpr uint16_t BANNER_FAULT    = rgb565(200,  20,  20);
    constexpr uint16_t BANNER_SELFTEST = rgb565(235, 140,   0);
}
