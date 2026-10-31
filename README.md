# SE Testkoffer – Steuerung V55 (Grundaufbau)

Programm für die Steuerplatine «SE Testkoffer – Steuerung V2» (ESP32 LOLIN D32). Es steuert die Prüfspannungen 24 VDC, 48 VDC und 230 VAC auf die vier Ausgänge MN/MX2, MX, XF und MCH. Bedient wird der Koffer über das Touch-Display; für den Service gibt es zusätzlich eine serielle Kommandozeile.

Der Funktionsumfang beschränkt sich auf den Grundaufbau. Rückmeldeeingänge (U13), Modbus und Impulsbetrieb sind bewusst nicht enthalten.

---

## 1. Hardware und Pinbelegung (Schema V2)

| Funktion | ESP32-Pin | Bauteil / Netz |
|---|---|---|
| I²C SDA | IO21 | MCP_SDA → U15, U16 |
| I²C SCL | IO22 | MCP_SCL → U15, U16 |
| Display MOSI | IO23 | LCD_SPI_Mosi (Display Pin 19) |
| Display MISO | IO19 | LCD_SPI_Miso (Pin 21) |
| Display SCK | IO18 | LCD_SPI_Sck (Pin 23) |
| Display CS | IO5 | LCD_SPI_Ce0 (Pin 24) |
| Touch CS | IO17 | LCD_SPI_Ce1 (Pin 26) |
| Display DC | IO16 | LCD_Dc (Pin 15) |
| Display Reset | IO4 | LCD_Rst (Pin 13) |
| Display-Taster BTN0 | IO32 | LCD_Button0 (Pin 12), nur Eingang mit Pull-up, ohne Funktion |
| nicht verwendet | IO39 (VN), IO33, IO34, IO25–IO27 | Touch-IRQ (Touch wird abgefragt), BTN1/BTN2 n.c., Modbus |

**Relaisansteuerung** (Steuerprint V2 und angepasstes EPLAN-Schema):

| MCP23008 | Adresse | Treiber | Klemmen |
|---|---|---|---|
| -A2 (U15) | 0x20 | -A5 (U19) ULN2803 | J9 (MN/MX2), J10 (MX1) |
| -A3 (U16) | 0x21 | -A6 (U20) ULN2803 | J11 (XF), J12 (MCH) |

| Ausgang | 24 VDC | 48 VDC | 230 VAC |
|---|---|---|---|
| MN / MX2 | K12 – J9.1 (-A2 GP0) | K11 – J9.2 (GP1) | K10 – J9.3 (GP2) |
| MX1 | K9 – J10.1 (-A2 GP4) | K8 – J10.2 (GP5) | K7 – J10.3 (GP6) |
| XF | K6 – J11.1 (-A3 GP0) | K5 – J11.2 (GP1) | K4 – J11.3 (GP2) |
| MCH | K3 – J12.1 (-A3 GP4) | K2 – J12.2 (GP5) | K1 – J12.3 (GP6) |

Pin 4 jeder Klemme (Bestückungsdruck «Custom», GP3/GP7) ist unbelegt und bleibt LOW. Die Zuordnung steht im Programm an genau einer Stelle: Tabelle `relayPin()` in `include/Outputs.h`.

---

## 2. Programmaufbau

```
DisplayUi ─┐                         Bedienung (Touch)
Shell ─────┼──► StateManager ──► McpManager ──► MCP23008 ──► ULN2803 ──► Relais
SelfTest ──┘    (Regeln)          (I²C, Kontrolle, Reparatur)
```

| Datei | Aufgabe |
|---|---|
| `platformio.ini` | Board, Bibliothek TFT_eSPI, Display-Konfiguration (Treiber ILI9341, Pins, SPI-Takt) |
| `include/Config.h` | Alle festen Werte: Pins, Adressen, Zeiten, Farben |
| `include/Outputs.h` | Ausgänge, Spannungen, Ansichten, Beschriftungen |
| `McpManager` | Einzige Stelle mit I²C-Zugriff: schreiben, zurücklesen, Störungen reparieren |
| `StateManager` | Logik und Sicherheitsregeln R1–R7, einzige Stelle, die Relais einschaltet |
| `DisplayUi` | Bildschirm zeichnen, Touch auswerten, Touch-Kalibrierung |
| `Shell` | Serielle Befehle, Ereignismeldungen |
| `SelfTest` | Relaistest (nur über die Shell) |
| `main.cpp` | Objekte erzeugen, Startreihenfolge, Hauptschleife, Watchdog |

Kein Modul blockiert die Hauptschleife. Einzige Ausnahme ist die Touch-Kalibrierung beim Start; sie läuft deshalb, bevor der Watchdog aktiv wird.

---

## 3. Sicherheitsregeln (StateManager)

| Regel | Inhalt |
|---|---|
| R1 | Pro Ausgang ist immer höchstens eine Spannung aktiv. Der McpManager prüft das vor jedem Schreiben zusätzlich. |
| R2 | Ein Ausgang geht frühestens 200 ms nach seinem letzten Ausschalten wieder EIN («Break before make»). |
| R3 | Verriegelung zwischen Ausschaltspule MX (bzw. A4) und Einschaltspule XF (bzw. A2). Die andere Spule geht sofort AUS, die gewünschte schaltet 200 ms später ein. Die Verriegelung ist nicht abschaltbar. |
| R4 | Wird die Spannung eines eingeschalteten Ausgangs geändert, geht er AUS. |
| R5 | In der Ansicht NSX haben A2, A4 und B4 (Motorantrieb MT, Ausgänge XF, MX, MCH) immer dieselbe Spannung. **Sicherheitsfunktion:** Im NSX-Prüfkabel sind die Rückleiter dieser drei Ausgänge auf A1 gebrückt. Unterschiedliche Spannungen würden sonst z. B. den Neutralleiter 230 VAC mit GND 24 VDC verbinden. |
| R6 | Ein Wechsel der Ansicht schaltet alle Ausgänge AUS und setzt alle Spannungen auf 24 V. |
| R7 | Bei einem nicht reparierbaren I/O-Fehler gehen alle Ausgänge AUS, und Einschalten ist gesperrt, bis die Hardware wieder antwortet. |
| R8 | **Prüfkabel bestätigen:** Einschalten ist erst möglich, wenn bestätigt ist, dass das Prüfkabel zur Ansicht passt. Beim Start und nach dem Selbsttest muss das Kabel gewählt werden. Ein Ansichtswechsel schaltet zuerst alles AUS und gilt erst nach der Bestätigung; bis dahin ist Einschalten gesperrt. |

Nach dem Einschalten oder einem Neustart (auch durch den Watchdog) sind alle Relais AUS und alle Ausgänge auf 24 V. Der Startbildschirm fragt, welches Prüfkabel eingesteckt ist (R8); erst danach kann geschaltet werden.

---

## 4. Störfestigkeit der I²C-Ansteuerung (McpManager)

Bei Tests am Prototyp haben Schaltvorgänge mit 230 VAC den I²C-Bus gestört; danach schaltete kein Relais mehr. V55 begegnet dem so:

1. **Sollzustand im Speicher.** Pro Baustein wird das ganze Byte (Register OLAT) in einem Telegramm geschrieben. Es gibt kein Lesen-Ändern-Schreiben, ein fehlerhaft gelesener Wert kann also keine Relais einschalten.
2. **Kontrolle nach jedem Schreiben und alle 100 ms.** IODIR muss 0x00 sein und OLAT dem Sollzustand entsprechen. Ein durch Störung zurückgesetzter Baustein (IODIR = 0xFF) wird so innerhalb von 100 ms erkannt.
3. **Reparatur.** Der Bus wird freigetaktet (9 Takte und Stopp-Bedingung), die Bausteine werden neu initialisiert und der Sollzustand neu geschrieben. Das wird bis zu 3-mal versucht.
4. **Gelingt die Reparatur nicht**, greift R7.
5. **Fehlerstatistik.** Der Befehl `fehler` zeigt sie an, `status` zeigt sie unten an. Damit lässt sich die Wirkung von EMV-Massnahmen messen.

---

## 5. Bedienung am Display

**Kopfzeile:**

- **MTZ/NW/NT** und **NSX** wählen die Ansicht. Ein Tipp auf den anderen Reiter schaltet sofort alles AUS und öffnet den Zwischenbildschirm «Ist das Prüfkabel für … eingesteckt?». **Ja** übernimmt die neue Ansicht (alle Spannungen 24 V, R6), **Abbrechen** kehrt zur bisherigen Ansicht zurück (Ausgänge bleiben AUS).
- **Alle: …** (nur MTZ/NW/NT) öffnet die Spannungsauswahl für alle Ausgänge. Die Taste hat die Farbe der gemeinsamen Spannung bzw. ist grau («div.»), wenn die Spannungen unterschiedlich sind.
- **Alle AUS** (gelb, beide Ansichten) schaltet sofort alle Ausgänge AUS.

**Je Ausgang eine Zeile** (Reihenfolge in beiden Ansichten: MN/MX2, XF, MX, MCH):

- Bezeichnung
- Spannungstaste: Farbe = Spannung, ein Tipp öffnet die Auswahl
- EIN/AUS-Taste: grau = AUS, grün = EIN

In der Ansicht NSX haben XF, MX und MCH (Motorantrieb MT) eine gemeinsame, über drei Zeilen reichende Spannungstaste **MT** (R5). MN/MX2 hat eine eigene Spannungstaste.

**Farben:** 24 V = blau, 48 V = orange, 230 V = rot, «Alle AUS» = gelb, aktive Ansicht = violett (inaktive fast schwarz).

**Bezeichnungen:**

| Ausgang (Shell-Nr.) | MTZ/NW/NT | NSX |
|---|---|---|
| 1 MN/MX2 | MN/MX2 – Unterspg. | MN/MX2 – Unterspg. |
| 2 XF | XF – Ein | XF – EIN (A2) |
| 3 MX1 | MX – Aus | MX – AUS (A4) |
| 4 MCH | MCH – Motor | MCH – Motor (B4) |

**NSX-Motorantrieb MT** (am Testschalter bestätigt): B4 speist den Motorantrieb; beim Zuschalten spannt der Motor die Feder. A2 schaltet EIN, A4 schaltet AUS. Bleibt B4 unter Spannung, spannt der Motor nach dem Ausschalten sofort wieder. Ist die Feder gespannt, lassen sich A2 und A4 auch ohne B4 bedienen. A2 und A4 sind gegenseitig verriegelt (R3). Wahlschalter am MT auf «auto». Höchstens 4 Schaltspiele pro Minute.

**NSX-Prüfkabel:** B4 → Block B Pin 7, A2 → Pin 9, A4 → Pin 8; die Rückleiter von MCH, XF und MX sind im Kabel gebrückt und gehen auf A1 (Block B Pin 3). MN/MX2 ist **nicht** in dieser Brücke: Block A Pin 9 (D1/C1) und Pin 7 (D2, MN) bzw. Pin 8 (C2, MX).
**Restrisiko:** Das Programm erkennt nicht selbst, welches Kabel eingesteckt ist. Die Abfrage beim Start und bei jedem Ansichtswechsel (R8) zwingt zur bewussten Bestätigung; eine falsche Bestätigung kann sie nicht verhindern. Ein Hinweis am Kabel bleibt deshalb sinnvoll.

**Startbildschirm** (nach dem Einschalten und nach dem Selbsttest): «Welches Prüfkabel ist eingesteckt?» mit den Tasten **MTZ/NW/NT** und **NSX**. Bis zur Auswahl bleiben alle Ausgänge AUS.

**Darstellung:** Der Bildschirm ist in fünf Bänder geteilt (Kopfzeile + vier Zeilen). Nur geänderte Bänder werden neu gezeichnet, und zwar zuerst unsichtbar im Zwischenspeicher (Sprite, 32 kB) und dann am Stück übertragen. Dadurch gibt es kein sichtbares Neuzeichnen und kein Flackern, auch nicht beim Öffnen und Schliessen der Auswahlliste.

**Statusbänder:**

- **Rot:** I/O-Fehler, alle Ausgänge AUS.
- **Orange:** Selbsttest läuft. Das Display ist dann gesperrt.

---

## 6. Serielle Kommandozeile (115200 Baud)

| Befehl | Wirkung |
|---|---|
| `status` | Zustand aller Ausgänge, I/O-Zustand, Fehlerstatistik |
| `ein <1-4>` | Ausgang einschalten (1 = MN/MX2, 2 = XF, 3 = MX, 4 = MCH); abgelehnt, solange das Prüfkabel nicht bestätigt ist |
| `aus <1-4\|alle>` | Ausgang bzw. alle ausschalten (bricht einen Selbsttest ab) |
| `spg <1-4\|alle> <24\|48\|230>` | Spannung wählen |
| `ansicht mtz\|nsx` | Ansichtswechsel anfordern: alles AUS, Einschalten gesperrt |
| `ansicht ja` / `ansicht nein` | Prüfkabel innerhalb 10 s bestätigen bzw. abbrechen |
| `selbsttest start` → `selbsttest ja` | Relaistest starten (Bestätigung innert 10 s) |
| `selbsttest stop` | Relaistest abbrechen |
| `fehler [reset]` | I²C-Fehlerstatistik anzeigen bzw. zurücksetzen |
| `kalibrieren` | Touch neu kalibrieren (ESP32 startet neu) |

> **Selbsttest nur ohne Prüfling und ohne Testkabel!** Jeder Ausgang erhält nacheinander 24 V, 48 V und 230 V.

Hinweis: Solange ein PC über USB angeschlossen ist, ist die Steuer-GND über den PC mit dem Schutzleiter verbunden. EMV-Tests deshalb ohne USB durchführen.

---

## 7. Inbetriebnahme

1. **Sicherung des alten Programms aufbewahren.** Die Datei `ESP32_Backup_Prototyp.bin` lässt sich bei Bedarf wieder aufspielen:
   ```
   pio pkg exec -p tool-esptoolpy -- esptool.py --chip esp32 --port COMx write_flash 0x0 ESP32_Backup_Prototyp.bin
   ```
2. **Projekt in VS Code/PlatformIO öffnen** und **24 V ausschalten**.
3. **USB anschliessen und hochladen** (Upload →).
4. **Touch kalibrieren.** Beim ersten Start fordert das Display dazu auf: die Markierungen in den vier Ecken antippen. Die Werte werden gespeichert.
5. **Selbsttest** ohne Prüfling: `selbsttest start`, dann `selbsttest ja`. Prüfen, ob jedes Relais anzieht und die Spannung an der richtigen Klemme anliegt.

**Falls die Anzeige nicht stimmt** (Einstellungen in `platformio.ini` bzw. `Config.h`):

| Beobachtung | Abhilfe |
|---|---|
| Bild steht auf dem Kopf | In `Config.h` `DisplayConfig::ROTATION = 3`, danach `kalibrieren` |
| Rot und Blau vertauscht | In `platformio.ini` `-D TFT_RGB_ORDER=TFT_BGR` ergänzen |
| Farben invertiert (Negativbild) | In `platformio.ini` `-D TFT_INVERSION_ON=1` ergänzen |
| Bildstörungen | `SPI_FREQUENCY` auf 10000000 senken |
| Display bleibt schwarz | Speisung 3,3 V (Pin 1, 17) und 5 V (Pin 2, 4) prüfen; Treiber ILI9341 bestätigen |

---

## 8. Prüfstand dieser Version

- Mit PlatformIO (espressif32 6.9.0, Arduino-Core 2.0.17, TFT_eSPI 2.5.43) fehlerfrei übersetzt, auch mit `-Wall -Wextra` ohne Warnungen. RAM 7,0 %, Flash 27 %.
- Die Logik von StateManager und SelfTest wurde auf dem PC mit einem simulierten McpManager getestet:
  - Sicherheitspause, Verriegelung (nie beide Spulen gleichzeitig), Spannungswechsel → AUS, NSX-Kopplung, Ansichtswechsel, Störungszustand und Wiederanlauf
  - Bitbelegung der MCP und Plausibilitätsprüfung
  - vollständiger Selbsttest (12 Relais, nie mehr als ein Relais gleichzeitig)
  - V55: Einschalten beim Start gesperrt, Ansichtswechsel anfordern/abbrechen/bestätigen, Zeitlimit 10 s für die Shell, Sperre nach dem Selbsttest
  - V53: Zuordnung aller 12 Relais nach der neuen Reihenfolge der Ausgänge, Ansichtswechsel (alles AUS, 24 V), NSX-Kopplung mit unabhängigem MN, Verriegelung A2/A4
- **V53 noch nicht auf der Hardware geprüft:** neue Display-Darstellung (Bänder, Sprite, Taste MT, «Alle AUS»).
- **Noch nicht auf der Hardware geprüft:** Display-Treiber und Farbreihenfolge, Touch-Kalibrierung, I²C-Reparatur unter realen Störungen. Der Treiber ILI9341 wurde aus der Firmware des Prototyps abgeleitet.

---

## 9. Änderungen gegenüber V51

- IO32 wird nicht mehr als Ausgang auf HIGH gesetzt. Gemäss Schema V2 (Blatt 3) und Steuerprint V2 ist IO32 mit dem Taster BTN0 des Displays (Pin 12, LCD_Button0) verbunden und nicht mit der Hintergrundbeleuchtung. Ein Ausgang mit HIGH-Pegel würde beim Betätigen des Tasters kurzgeschlossen. IO32 ist jetzt ein Eingang mit Pull-up und hat im Grundaufbau keine Funktion.
- Die Konstante `Pins::LCD_BACKLIGHT` heisst neu `Pins::LCD_BUTTON0`.
- Kommentare und Pintabelle ergänzt: IO39 (VN) Touch-IRQ, IO33/IO34 nicht angeschlossen, IO25–IO27 Modbus (nicht bestückt).
- Relaiszuordnung, Sicherheitsregeln, Display-Bedienung und Shell sind unverändert.

---

## 10. Änderungen gegenüber V52

- **Reihenfolge der Ausgänge** in beiden Ansichten und in der Shell: 1 = MN/MX2, 2 = XF, 3 = MX, 4 = MCH. Die Relaiszuordnung (Tabelle `relayPin()`) ist unverändert, nur die Reihenfolge der Zeilen.
- **Beschriftungen:** MTZ/NW/NT: MN/MX2 «Unterspg.», MCH «Motor». NSX: MN/MX2 «Unterspg.», XF «EIN (A2)», MX «AUS (A4)», MCH «Motor (B4)».
- **NSX-Ansicht:** gemeinsame Spannungstaste MT über die drei Motorzeilen, keine Taste «Alle».
- **Neue Taste «Alle AUS»** (gelb) in beiden Ansichten.
- **Ansichtswechsel** setzt zusätzlich alle Spannungen auf 24 V (R6).
- **R5** ist als Sicherheitsfunktion für die Brücke der Rückleiter im NSX-Prüfkabel dokumentiert.
- **Flackerfreie Anzeige:** Zeichnen in fünf Bändern über einen Zwischenspeicher (Sprite); die Auswahlliste löscht den Bildschirm nicht mehr. Kann der Zwischenspeicher nicht angelegt werden, zeichnet das Programm direkt (Notbetrieb mit Flackern).

---

## 11. Änderungen gegenüber V53

- Reiter der Ansicht auffälliger: aktiver Reiter violett mit weisser Schrift, inaktiver fast schwarz mit grauer Schrift (vorher hellgrau / dunkelgrau).

---

## 12. Änderungen gegenüber V54

- **Neue Regel R8 (Prüfkabel bestätigen):** Startbildschirm «Welches Prüfkabel ist eingesteckt?» beim Einschalten und nach dem Selbsttest; Zwischenbildschirm mit «Abbrechen» / «Ja» bei jedem Ansichtswechsel. Der Wechsel schaltet sofort alles AUS; bis zur Bestätigung ist Einschalten gesperrt.
- **Shell:** `ansicht mtz|nsx` fordert den Wechsel an, `ansicht ja` bestätigt innerhalb von 10 s, `ansicht nein` bricht ab. `status` zeigt den Zustand des Prüfkabels, Meldungen `[KABEL]` erscheinen unaufgefordert.
- **StateManager:** neue Funktionen `requestViewChange()`, `confirmViewChange()`, `cancelViewChange()`, `chooseView()`, `requireCableConfirmation()`; `setView()` entfällt.
- **Selbsttest:** Die Bestätigung «Prüfling und Testkabel abgehängt» gilt als Kabelauswahl; nach dem Test muss das Prüfkabel neu gewählt werden.
