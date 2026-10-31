#include "Shell.h"
#include "StateManager.h"
#include "SelfTest.h"
#include "Outputs.h"
#include <Arduino.h>

/* ===================================================================== */
/* TABELLEN-SPALTENBREITEN */

static const int COL_OUT  = 12;
static const int COL_MODE = 8;
static const int COL_ST   = 6;
static const int COL_IMP  = 8;

/* ===================================================================== */

void Shell::begin(StateManager* state, SelfTest* selftest)
{
    _state    = state;
    _selftest = selftest;

    Serial.println("Shell ready. Type 'help'.");
    printPrompt();
}

/* ===================================================================== */

void Shell::printPrompt()
{
    Serial.print("> ");
}

/* ===================================================================== */

void Shell::update()
{
    while (Serial.available())
    {
        char c = Serial.read();

        if (c == '\n')
        {
            Serial.println();

            if (_line.length())
            {
                handleCommand(_line);
                _line = "";
            }

            // ⛔ Kein Prompt während SelfTest
            if (!_selftest || !_selftest->running())
                printPrompt();
        }
        else if (c == 8 || c == 127)   // Backspace
        {
            if (_line.length())
            {
                _line.remove(_line.length() - 1);
                Serial.print("\b \b");
            }
        }
        else
        {
            _line += c;
            Serial.print(c);   // 🔴 GRUNDREGEL: Eingabe IMMER sichtbar
        }
    }
}

/* ===================================================================== */

void Shell::handleCommand(const String& cmd)
{
    String c = cmd;
    c.trim();
    c.toLowerCase();

    /* ---- System ---- */
    if (c == "help")          { cmdHelp(); return; }
    if (c == "status")        { printFullStatus(); return; }

    /* ---- All ---- */
    if (c == "all on")  { cmdAll(true);  return; }
    if (c == "all off") { cmdAll(false); return; }

    /* 🧪 SelfTest */
    if (c == "selftest stop" && _selftest)
    {
        _selftest->stop();
        return;
    }

    if (c == "selftest" && _selftest)
    {
        _selftest->start();
        return;
    }

    /* ---- Parameterbefehle ---- */
    int sp = c.indexOf(' ');
    String arg = sp > 0 ? c.substring(sp + 1) : "";

    if (c.startsWith("on "))        cmdOn(arg);
    else if (c.startsWith("off "))  cmdOff(arg);
    else if (c.startsWith("imp "))  cmdImpulse(arg);
    else if (c.startsWith("mode "))
    {
        int s = arg.indexOf(' ');
        if (s > 0)
            cmdMode(arg.substring(0, s), arg.substring(s + 1));
        else
            Serial.println("Usage: mode <out> <mode>");
    }
    else
    {
        Serial.println("Unknown command. Type 'help'.");
    }
}

/* ===================================================================== */
/* HELP */

void Shell::cmdHelp()
{
    Serial.println("\nCommands:");
    Serial.println("  on <out>");
    Serial.println("  off <out>");
    Serial.println("  imp <out>");
    Serial.println("  mode <out> <mode>");
    Serial.println("  all on | all off");
    Serial.println("  status");
    Serial.println("  selftest");
    Serial.println("  selftest stop\n");

    Serial.println("Outputs:");
    for (int i = 0; i < OUTPUT_COUNT; i++)
    {
        Serial.print("  ");
        Serial.print(getOutputName((OutputID)i));
        Serial.print(" -> ");
        Serial.println(getOutputShortName((OutputID)i));
    }

    Serial.println("\nModes:");
    for (int i = 0; i < MODE_COUNT; i++)
    {
        Serial.print("  ");
        Serial.print(getModeName((OutputMode)i));
        Serial.print(" -> ");
        Serial.println(getModeShortName((OutputMode)i));
    }
    Serial.println();
}

/* ===================================================================== */
/* COMMANDS */

void Shell::cmdOn(const String& arg)
{
    OutputID out;
    if (!parseOutput(arg, out)) return;

    _state->turnOn(out);

    printTableHeader();
    printTableRow(out);
    printTableFooter();
}

void Shell::cmdOff(const String& arg)
{
    OutputID out;
    if (!parseOutput(arg, out)) return;

    _state->turnOff(out);

    printTableHeader();
    printTableRow(out);
    printTableFooter();
}

void Shell::cmdImpulse(const String& arg)
{
    OutputID out;
    if (!parseOutput(arg, out)) return;

    _state->startImpulse(out);

    printTableHeader();
    printTableRow(out);
    printTableFooter();
}

void Shell::cmdMode(const String& outTxt, const String& modeTxt)
{
    OutputID out;
    OutputMode mode;

    if (!parseOutput(outTxt, out)) return;
    if (!parseMode(modeTxt, mode)) return;

    _state->setMode(out, mode);

    printTableHeader();
    printTableRow(out);
    printTableFooter();
}

void Shell::cmdAll(bool on)
{
    if (on) _state->allOn();
    else    _state->allOff();

    printFullStatus();
}

/* ===================================================================== */
/* TABLE OUTPUT */

void Shell::printTableHeader()
{
    Serial.println();
    Serial.println("-----------------------------------------------");
    Serial.print("OUTPUT"); pad(COL_OUT - 6);
    Serial.print("| MODE"); pad(COL_MODE - 5);
    Serial.print("| STATE"); pad(COL_ST - 6);
    Serial.print("| IMPULSE");
    Serial.println();
    Serial.println("-----------------------------------------------");
}

void Shell::printTableRow(OutputID out)
{
    const char* outName  = getOutputName(out);
    const char* modeName = getModeName(_state->getMode(out));

    Serial.print(outName);
    pad(COL_OUT - strlen(outName));

    Serial.print("| ");
    Serial.print(modeName);
    pad(COL_MODE - strlen(modeName) - 1);

    Serial.print("| ");
    const char* st = _state->isEnabled(out) ? "ON" : "OFF";
    Serial.print(st);
    pad(COL_ST - strlen(st) - 1);

    Serial.print("| ");
    Serial.println(_state->isImpulseActive(out) ? "ACTIVE" : "-");
}

void Shell::printFullStatus()
{
    printTableHeader();
    for (int i = 0; i < OUTPUT_COUNT; i++)
        printTableRow((OutputID)i);
    printTableFooter();
}

void Shell::printTableFooter()
{
    Serial.println("-----------------------------------------------");
    //Serial.println();
}

/* ===================================================================== */
/* UTIL */

void Shell::pad(int count)
{
    while (count-- > 0) Serial.print(" ");
}

/* ===================================================================== */
/* PARSER */

bool Shell::parseOutput(const String& txt, OutputID& out)
{
    for (int i = 0; i < OUTPUT_COUNT; i++)
    {
        if (txt == getOutputShortName((OutputID)i))
        {
            out = (OutputID)i;
            return true;
        }
    }
    Serial.println("Invalid output");
    return false;
}

bool Shell::parseMode(const String& txt, OutputMode& mode)
{
    for (int i = 0; i < MODE_COUNT; i++)
    {
        if (txt == getModeShortName((OutputMode)i))
        {
            mode = (OutputMode)i;
            return true;
        }
    }
    Serial.println("Invalid mode");
    return false;
}
