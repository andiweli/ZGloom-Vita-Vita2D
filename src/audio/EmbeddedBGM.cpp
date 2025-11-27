#include "EmbeddedBGM.h"
#include "AtmosphereVolume.h"
#include <SDL.h>
#include <SDL_mixer.h>

// eingebettete MOD-Daten
#include "audio/atmosphere_bgm.h" // g_bgm_mod, g_bgm_mod_len

namespace {
    Mix_Music* sMusic = nullptr;
    bool       sPlaying = false;
}

void BGM::Init() {
    sMusic   = nullptr;
    sPlaying = false;
}

void BGM::Shutdown() {
    BGM::Stop();
}

void BGM::PlayLooping() {
    if (sPlaying) return;
    if (!g_bgm_mod || g_bgm_mod_len == 0) return;

    // Aus eingebettetem Speicher laden (SDL_mixer übernimmt die Lifetime durch RW_free=1)
    SDL_RWops* rw = SDL_RWFromConstMem((const void*)g_bgm_mod, (int)g_bgm_mod_len);
    if (!rw) return;

    // MOD/XM/etc. aus Speicher laden
    Mix_Music* mus = Mix_LoadMUS_RW(rw, 1);
    if (!mus) {
        // Fallback: RW manuell schließen, wenn Mix_LoadMUS_RW fehlschlägt.
        SDL_RWclose(rw);
        return;
    }

    sMusic = mus;
    Mix_VolumeMusic(AtmosphereVolume::ToSDLVolume(AtmosphereVolume::Get()));
    if (Mix_PlayMusic(sMusic, -1) == 0) {
        sPlaying = true;
    } else {
        Mix_FreeMusic(sMusic);
        sMusic = nullptr;
    }
}

void BGM::Stop() {
    if (!sPlaying) return;
    Mix_HaltMusic();
    if (sMusic) { Mix_FreeMusic(sMusic); sMusic = nullptr; }
    sPlaying = false;
}

void BGM::SetVolume9(int v) {
    Mix_VolumeMusic(AtmosphereVolume::ToSDLVolume(v));
}
