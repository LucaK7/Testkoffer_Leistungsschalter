/*
 * ====================================================
 * 📄 Dateiname: config.h
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Zentrale Konfigurations- und Parameterdatei des
 * gesamten Testkoffer-Projekts.
 *
 * ⚠️ Wichtige Infos:
 * - Enthält AUSSCHLIESSLICH globale Konstanten
 * - KEINE Klassen
 * - KEINE Funktionen
 * - KEINE Logik
 * - Wird von ALLEN Modulen eingebunden
 *
 * 🧠 Konzept & Überlegungen:
 * - Alle sicherheitsrelevanten Zeiten sind zentral definiert
 * - Änderungen wirken sofort systemweit
 * - Erhöht Nachvollziehbarkeit und Wartbarkeit
 * - Verhindert "Magic Numbers" im Code
 *
 * 🔒 Sicherheitsgedanke:
 * - Safety-Delays dürfen NICHT lokal überschrieben werden
 * - Debug-Schalter sind bewusst zentral gehalten
 *
 * ====================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

/* 🔒 Sicherheits-Delay:
 * Zeitspanne zwischen:
 * 1) Alle Pins eines Ausgangs AUS
 * 2) Aktivieren des neuen Modus-Pins
 *
 * Zweck:
 * - Schutz von Relais
 * - Vermeidung von Kurzschlüssen
 * - Sauberes Umschalten der Spannungen
 */
static const uint32_t SAFETY_DELAY_MS = 200;

/* 🔁 SelfTest-Zeiten:
 * Dauer, wie lange ein Modus aktiv bleibt
 * bevor zum nächsten gewechselt wird
 */
static const uint32_t SELFTEST_MODE_ACTIVE_MS = 1000;

/* ⏸️ Pause zwischen zwei SelfTest-Schritten */
static const uint32_t SELFTEST_STEP_PAUSE_MS  = 300;

/* 🐞 Debug-Schalter:
 * Aktivieren serielle Debug-Ausgaben
 */
static const bool DEBUG_MCP   = true;
static const bool DEBUG_STATE = true;

#endif
