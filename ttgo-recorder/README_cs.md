# TTGO T-Camera Plus – Český přehled projektu

Tento dokument shrnuje klíčové části projektu `ttgo-recorder` v češtině, aby bylo snazší pochopit konfiguraci PlatformIO, strukturu zdrojových kódů i postup používání zařízení.

## Konfigurace PlatformIO (`platformio.ini`)
- **Platforma a deska:** `espressif32` + `esp32dev` s frameworkem Arduino.
- **Sériová linka:** monitor i bootlog běží na 115200 Bd, rychlost nahrávání je nastavena na 460800 Bd.
- **PSRAM:** příznaky `-DBOARD_HAS_PSRAM` a `-mfix-esp32-psram-cache-issue` aktivují a stabilizují externí PSRAM pro kameru a vyrovnávací paměti.
- **Displej ST7789:** sada `-DTFT_*` maker konfiguruje knihovnu TFT_eSPI bez nutnosti editace `User_Setup.h` (rozměry 240×240 px, ovládací piny MOSI/SCLK/CS/DC/BL atd.).
- **Souborový systém:** `board_build.filesystem = littlefs` povoluje nahrání webového UI na LittleFS (`pio run -t uploadfs`).
- **Knihovny:**
  - `ArduinoJson` pro práci s konfigurací ve formátu JSON.
  - `TFT_eSPI` pro ovládání displeje ST7789.
  - `SD` knihovna (Arduino) pro přístup k microSD kartě (audio, fotky, konfigurace).

## Struktura projektu
```
ttgo-recorder/
├── platformio.ini
├── README_cs.md   ← tento soubor
├── data/          ← HTML/JS/CSS dashboardu (LittleFS)
└── src/
    ├── main.cpp
    ├── audio_recorder.{h,cpp}
    ├── camera_module.{h,cpp}
    ├── display_ui.{h,cpp}
    ├── web_server.{h,cpp}
    ├── config_store.{h,cpp}
    └── power_manager.{h,cpp}
```

## Popis hlavních modulů
- **`main.cpp`** – inicializace periferií, montáž SD/LittleFS, načtení konfigurace, řízení režimů (nahrávání, streamy) a aktualizace displeje.
- **`audio_recorder.*`** – obsluha I2S mikrofonu (GPIO14/32/33), vytváření WAV souborů (16 kHz, 16 bit, mono) s VOX prahováním a bufferem pro živé streamování.
- **`camera_module.*`** – konfigurace kamery OV2640 s PSRAM, JPEG snímky na SD (`/photos`) a MJPEG stream pro web.
- **`display_ui.*`** – rozhraní TFT_eSPI s obrazovkami pro boot, stav Wi-Fi, indikaci nahrávání/streamů i zobrazení IP adresy a baterie.
- **`web_server.*`** – HTTP server (Async), REST API a dashboard: spouštění nahrávání, audio/video streamy, pořizování fotek, správa souborů, nastavení.
- **`config_store.*`** – práce s JSON konfigurací (`/config.json` na SD, záloha v NVS) – SSID/heslo, VOX práh, NTP, časové pásmo atd.
- **`power_manager.*`** – čtení IP5306 přes I²C a poskytování procent baterie + stav nabíjení pro UI a REST `/battery`.

## Webové rozhraní (`data/`)
LittleFS obsahuje tři soubory:
- `index.html` – hlavní dashboard s tlačítky (Start/Stop nahrávání, streamy, foto), stavovou kartou a formulářem nastavení.
- `app.js` – volání REST API (audio/video streamy, upload nastavení, správa souborů, update stavu v reálném čase).
- `style.css` – jednoduchý responzivní vzhled s tmavým motivem.
Po změně kterékoli položky v `data/` je potřeba znovu spustit `pio run -t uploadfs`.

## Postup sestavení a použití
1. **Sestavení:** `pio run` v kořenové složce projektu.
2. **Nahrání firmwaru:** `pio run -t upload` s připojeným T-Camera Plus přes USB.
3. **Nahrání webového UI:** `pio run -t uploadfs` po prvním flashování nebo po úpravách HTML/JS/CSS.
4. **MicroSD karta:** naformátovat na FAT32, vložit do zařízení; firmware vytvoří složky `/audio` a `/photos` automaticky.
5. **Wi-Fi:** zařízení se pokusí připojit k SSID z `config.json`. Pokud není dostupné, vytvoří AP `TCAMERA-RECORDER` s heslem `12345678` (IP `192.168.4.1`).
6. **Webové rozhraní:** v prohlížeči otevřete `http://<sta-ip>` nebo `http://192.168.4.1`. Dashboard umožňuje:
   - spustit/zastavit nahrávání WAV, audio stream a video stream,
   - pořídit fotografii a stáhnout ji ze seznamu souborů,
   - poslouchat `/audio_stream` přímo přes `<audio>` prvek,
   - spravovat soubory (stáhnout/smazat) na kartě,
   - uložit nové nastavení (SSID, heslo, VOX práh, NTP, časová zóna, název zařízení).
7. **Základní test:**
   - Po připojení k webu stiskněte „Start Audio Recording“ → na SD vznikne soubor `audio/YYYYMMDD_HHMMSS.wav`.
   - Otevřete `audio_stream` v přehrávači na dashboardu pro kontrolu živého zvuku.
   - Klikněte na „Take Photo“ a ověřte, že se v seznamu souborů objeví nová položka v `/photos`.
   - Ověřte video stream načtením `<img>` s MJPEG (`/video_stream`).

Tento český přehled můžete libovolně rozšiřovat nebo aktualizovat podle budoucích změn projektu.
