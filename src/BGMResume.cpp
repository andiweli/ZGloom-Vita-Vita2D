#include "BGMResume.h"

#if __has_include(<SDL_mixer.h>)
  #include <SDL_mixer.h>
  #define ZG_HAVE_SDL_MIXER 1
#else
  #define ZG_HAVE_SDL_MIXER 0
#endif

namespace BGM {

void ResumeAll() {
#if ZG_HAVE_SDL_MIXER
    // If level music was paused (e.g., when opening the pause menu), resume it.
    if (Mix_PausedMusic()) {
        Mix_ResumeMusic();
    }

    // Resume any paused SFX channels as well to ensure consistency.
    // Query how many channels are currently allocated without changing the value.
    int channels = Mix_AllocateChannels(-1);
    if (channels < 0) channels = 0;
    for (int ch = 0; ch < channels; ++ch) {
        if (Mix_Paused(ch)) {
            Mix_Resume(ch);
        }
    }
#else
    (void)0; // no-op if SDL_mixer is unavailable
#endif
}

void PauseAll() {
#if ZG_HAVE_SDL_MIXER
    // Pause all SFX channels and the music.
    Mix_Pause(-1);
    Mix_PauseMusic();
#else
    (void)0; // no-op
#endif
}

} // namespace BGM
