# thesada-fw

Custom firmware for Thesada property monitoring nodes.

Built on C++17 / Arduino framework / PlatformIO. Module Registry + Event Bus architecture with embedded Lua scripting.

## Licence

SPDX-License-Identifier: GPL-3.0-only

## Structure

See [thesada.cloud/firmware](https://thesada.cloud/firmware) for full architecture documentation.

## Quick Start

1. Copy `data/config.json` and fill in your values
2. Edit `config.h` to enable required modules
3. `pio run -e esp32dev` — compile
4. `pio run -e esp32dev --target upload` — flash
5. `pio run -e esp32dev --target uploadfs` — upload LittleFS (config + scripts)
