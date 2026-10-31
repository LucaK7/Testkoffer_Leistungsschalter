/*
 * ====================================================
 * 📄 Dateiname: main.cpp
 * ====================================================
 *
 * 🎯 Funktion der Datei:
 * Terminal-gesteuertes Testprogramm (SelfTest).
 *
 * 🧠 Konzept & Überlegungen:
 * - Test aller Ausgänge
 * - Test aller Modi
 * - Nicht blockierend
 *
 * ====================================================
 */

#include <Arduino.h>

#include "StateManager.h"
#include "Outputs.h"
#include "config.h"

StateManager state;

uint8_t currentOutput = 0;
uint8_t currentMode = 0;
uint32_t lastStepMs = 0;
bool running = false;

void setup()
{
    Serial.begin(115200);
    delay(3000);

    Serial.println("=== SELFTEST ===");
    Serial.println("Taste 's' zum Starten");

    state.begin();
}

void loop()
{
    state.update();

    if (Serial.available())
    {
        char c = Serial.read();
        if (c == 's')
        {
            running = true;
            currentOutput = 0;
            currentMode = 0;
            lastStepMs = millis();
            Serial.println("SelfTest gestartet");
        }
    }

    if (!running)
        return;

    uint32_t now = millis();

    if (now - lastStepMs >= (SAFETY_DELAY_MS + SELFTEST_MODE_ACTIVE_MS))
    {
        state.turnOff((OutputID)currentOutput);

        currentMode++;
        if (currentMode >= MODE_COUNT)
        {
            currentMode = 0;
            currentOutput++;
        }

        if (currentOutput >= OUTPUT_COUNT)
        {
            running = false;
            Serial.println("SelfTest beendet");
            return;
        }

        state.setMode((OutputID)currentOutput, (OutputMode)currentMode);
        state.turnOn((OutputID)currentOutput);

        Serial.print("→ ");
        Serial.print(getOutputName((OutputID)currentOutput));
        Serial.print(" | ");
        Serial.println(getModeName((OutputMode)currentMode));

        lastStepMs = now;
    }
}
