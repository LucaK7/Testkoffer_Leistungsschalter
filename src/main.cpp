/* ============================================================================
   Datei:   main.cpp
   Projekt: SE Testkoffer – Steuerung (Grundaufbau) V55
   ============================================================================
   Rolle im Gesamtsystem
   ----------------------------------------------------------------------------
   Einstiegspunkt des Programms. Erzeugt alle Programmteile, verbindet sie
   miteinander und ruft sie in der Hauptschleife zyklisch auf. Hier steht
   bewusst KEINE Fachlogik.

   Aufbau des Programms (Module)
   ----------------------------------------------------------------------------

        ┌───────────┐   ┌───────────┐
        │ DisplayUi │   │   Shell   │      Bedienung (Touch / USB)
        └─────┬─────┘   └─────┬─────┘
              │   Befehle     │      ┌──────────┐
              └──────┬────────┘◄─────┤ SelfTest │  (nur über Shell)
                     ▼               └────┬─────┘
              ┌──────────────┐            │
              │ StateManager │◄───────────┘       Logik + Sicherheitsregeln
              └──────┬───────┘
                     ▼ Steuerbytes
              ┌──────────────┐
              │  McpManager  │                    I²C, MCP23008, Störungsbehandlung
              └──────┬───────┘
                     ▼
          -A2/-A3 MCP23008 → -A5/-A6 ULN2803 → J9–J12 → Relais -K1 … -K12

   Ablauf
   ----------------------------------------------------------------------------
   setup():
     1. McpManager zuerst: alle Relais sicher AUS (auch nach Watchdog-Neustart)
     2. StateManager: Startzustand (MTZ/NW/NT, 24 V, alles AUS)
     3. Display: Bild aufbauen, ggf. Touch kalibrieren (wartet auf Benutzer)
     4. Shell: Begrüssung
     5. Watchdog aktivieren: hängt das Programm > 5 s, startet der ESP32 neu
        und Schritt 1 schaltet alles aus.
   loop():
     Alle Module werden nacheinander aufgerufen. Keines blockiert, ein
     Durchlauf dauert typischerweise wenige Millisekunden.
   ============================================================================ */

#include <Arduino.h>
#include <esp_task_wdt.h>

#include "Config.h"
#include "McpManager.h"
#include "StateManager.h"
#include "SelfTest.h"
#include "DisplayUi.h"
#include "Shell.h"

/* ----------------------------------------------------------------------------
   Programmteile (Objekte). Die Reihenfolge entspricht den Abhängigkeiten:
   jedes Objekt erhält Verweise auf die Teile, die es benutzen darf.
   ---------------------------------------------------------------------------- */
McpManager   mcp;
StateManager state(mcp);
SelfTest     selftest(state);
DisplayUi    display(state, selftest);
Shell        shell(state, selftest, mcp, display);

void setup()
{
    Serial.begin(115200);

    mcp.begin();        // 1. Relais sicher AUS – so früh wie möglich
    state.begin();      // 2. Startzustand
    display.begin();    // 3. Bedienoberfläche (ggf. Touch-Kalibrierung)
    shell.begin();      // 4. serielle Kommandozeile

    // 5. Watchdog für die Hauptschleife
    esp_task_wdt_init(Timing::WATCHDOG_S, true);   // true = Neustart bei Zeitüberschreitung
    esp_task_wdt_add(nullptr);                     // diese Aufgabe (loop) überwachen
}

void loop()
{
    state.update();      // Sicherheitspausen, Einschalten, Hardwarekontrolle
    selftest.update();   // Ablauf des Selbsttests (falls aktiv)
    display.update();    // Touch abfragen, Änderungen zeichnen
    shell.update();      // serielle Befehle, Ereignismeldungen

    esp_task_wdt_reset();   // «Lebenszeichen» an den Watchdog
}
