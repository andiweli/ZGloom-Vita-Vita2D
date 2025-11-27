#pragma once

// Minimal, safe helpers to resume/pause music using SDL_mixer if available.
// If SDL_mixer headers are not present, all functions become no-ops.

namespace BGM {
    // Resume paused music and any paused SFX channels.
    void ResumeAll();

    // Pause all channels and music (not used by the fix, but handy).
    void PauseAll();
}
