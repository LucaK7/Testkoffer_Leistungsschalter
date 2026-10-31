#pragma once

/* ============================================================================
   Datei: Config.h

   Funktion:
   ---------
   Zentrale Konfigurationsdatei für globale Parameter:
   - Serielle Kommunikation
   - Zeitkonstanten
   - Sicherheits-Delays
   - Impuls- & SelfTest-Steuerung
   - Konsistentes Tabellenlayout

   Konzept:
   --------
   - ALLE Zeiten zentral
   - KEINE magischen Zahlen im Code
   - KEINE Logik in dieser Datei
   ============================================================================ */


/* ===================================================================== */
/* 🔌 Serielle Schnittstelle */

static const uint32_t SERIAL_BAUDRATE = 115200;


/* ===================================================================== */
/* 🚀 Systemstart */

/// Startverzögerung nach Boot (Flash-/USB-Stabilität)
static const uint32_t STARTUP_DELAY_MS = 2500;


/* ===================================================================== */
/* ⏳ Sicherheits- & Schaltzeiten */

/// Kurzes Schutz-Delay beim Umschalten von Relais / Ausgängen
static const uint32_t SAFETY_DELAY_MS = 200;


/* ===================================================================== */
/* ⚡ Impulssteuerung */

/// Standarddauer für Impuls-Ausgänge
static const uint32_t DEFAULT_IMPULSE_MS = 5000;


/* ===================================================================== */
/* 🧪 SelfTest */

/// Zeit, wie lange ein Modus aktiv bleibt
static const uint32_t SELFTEST_MODE_ACTIVE_MS = 400;

/// Pause zwischen zwei SelfTest-Schritten
static const uint32_t SELFTEST_STEP_PAUSE_MS  = 200;


/* ===================================================================== */
/* 📊 Shell Status-Tabelle */

/// Spaltenbreiten der Status-Tabelle (Shell)
static const uint8_t TABLE_COL_OUTPUT  = 12;
static const uint8_t TABLE_COL_MODE    = 8;
static const uint8_t TABLE_COL_STATE   = 6;
static const uint8_t TABLE_COL_IMPULSE = 8;


/* ===================================================================== */
/* 🔧 Debug / Entwicklung */

/// Globale Debug-Ausgaben aktivieren (optional nutzbar)
static const bool DEBUG_SERIAL = true;


/* ===================================================================== */
/* Ende der Config.h */
/* ===================================================================== */
