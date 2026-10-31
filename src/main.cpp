#include <Arduino.h>

#include "Config.h"
#include "Shell.h"
#include "StateManager.h"
#include "SelfTest.h"

/* ===================================================================== */
/* GLOBALE OBJEKTE */

StateManager state;
Shell        shell;
SelfTest     selftest;

/* ===================================================================== */

void setup()
{
    /* 🔐 Sicherheits-Delay für stabiles Flashen / Boot */
    delay(STARTUP_DELAY_MS);

    Serial.begin(SERIAL_BAUDRATE);
    while (!Serial) {;}

    Serial.println();
    Serial.println("System booting...");

    /* Initialisierung */
    state.begin();
    selftest.begin(&state, &shell);

    Serial.println("System ready.");
    shell.begin(&state, &selftest);

    
}

/* ===================================================================== */

void loop()
{
    shell.update();      // 🔹 Shell Eingaben
    state.update();      // 🔹 Hardware / Impulse
    selftest.update();   // 🔹 SelfTest Sequenz (WICHTIG!)
}
