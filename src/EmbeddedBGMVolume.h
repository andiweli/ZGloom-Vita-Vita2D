#pragma once
#include "config.h"
#ifndef ZG_HAS_INCLUDE
  #ifdef __has_include
    #define ZG_HAS_INCLUDE(header) __has_include(header)
  #else
    #define ZG_HAS_INCLUDE(header) 0
  #endif
#endif
#if ZG_HAS_INCLUDE(<SDL_mixer.h>)
  #include <SDL_mixer.h>
  #define ZG_HAVE_MIXER 1
#else
  #define ZG_HAVE_MIXER 0
#endif
namespace EmbeddedBGMVolume {
static inline int Map0to9_ToMixVol(int v){ if(v<0)v=0; if(v>9)v=9; return (v*128)/9; }
static inline void ApplyLevel(int level0to9){
#if ZG_HAVE_MIXER
    Mix_VolumeMusic(Map0to9_ToMixVol(level0to9));
#else
    (void)level0to9;
#endif
}
static inline void ApplyFromConfig(){ ApplyLevel(Config::GetAtmosVolume()); }
} // namespace EmbeddedBGMVolume
