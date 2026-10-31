#pragma once

/* 🔌 Logische Ausgänge */
enum OutputID
{
    OUT_MN_MX2 = 0,
    OUT_MX,
    OUT_XF,
    OUT_MCH,
    OUTPUT_COUNT
};

/* ⚙️ Betriebsmodi */
enum OutputMode
{
    MODE_24V = 0,
    MODE_48V,
    MODE_230V,
    MODE_CUSTOM,
    MODE_COUNT
};

/* 📛 Lange Namen */
inline const char* getOutputName(OutputID id)
{
    static const char* names[] =
    {
        "MN/MX2",
        "MX",
        "XF",
        "MCH"
    };
    return names[id];
}

/* 🔤 Kurznamen (für Shell) */
inline const char* getOutputShortName(OutputID id)
{
    static const char* names[] =
    {
        "mn",
        "mx",
        "xf",
        "mch"
    };
    return names[id];
}

/* 📛 Modusnamen */
inline const char* getModeName(OutputMode mode)
{
    static const char* names[] =
    {
        "24V",
        "48V",
        "230V",
        "Custom"
    };
    return names[mode];
}

/* 🔤 Modus-Abkürzungen (für Shell) */
inline const char* getModeShortName(OutputMode mode)
{
    static const char* names[] =
    {
        "24",
        "48",
        "230",
        "cust"
    };
    return names[mode];
}
