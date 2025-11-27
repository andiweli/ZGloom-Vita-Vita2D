#pragma once

namespace AtmosphereVolume {
    // 0..9 (persistiert in config.txt als ATMOS=)
    int  Get();
    void Set(int v);

    // Mapping auf SDL_mixer-Lautstärke (0..128)
    int  ToSDLVolume(int v);

    // optional; wird intern bei Set() genutzt
    void LoadFromConfig();  // liest einmalig ATMOS=, falls vorhanden
    void SaveToConfig();    // schreibt/aktualisiert ATMOS=
}

// Für MenuScreen-Einträge (Funktionszeiger-Signatur)
extern "C" {
    int  MenuGetAtmosphereVol();
    void MenuSetAtmosphereVol(int v);
}
