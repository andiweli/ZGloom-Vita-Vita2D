// src/ConfigEffectsFallback.cpp — stub implementations for effects config
// Provides missing symbols referenced by menus & save code when the full effects system is not compiled.
// Safe to drop in: contains only in-memory state (no file I/O).

#include <algorithm>

namespace Config {

static inline int clampi(int v, int lo, int hi){ return v < lo ? lo : (v > hi ? hi : v); }
static inline int clamp01(int v){ return v ? 1 : 0; }
static inline int clamp05(int v){ return clampi(v, 0, 5); }
static inline int clampm100_100(int v){ return clampi(v, -100, 100); }

// --- Stored as simple integers; defaults OFF/0 ---
static int gVignetteEnabled     = 0;
static int gVignetteStrength    = 0; // 0..5
static int gVignetteRadius      = 0; // 0..5
static int gVignetteSoftness    = 0; // 0..5
static int gVignetteWarmth      = 0; // -100..100

static int gFilmGrain           = 0; // 0/1
static int gFilmGrainIntensity  = 0; // 0..5

static int gScanlines           = 0; // 0/1
static int gScanlineIntensity   = 0; // 0..5

static int gMuzzleFlash         = 0; // 0/1
static int gBilinearFilter      = 0; // 0/1

// NEW: Blob shadows (0/1)
static int gBlobShadows         = 0; // 0/1

// --- Effects config lifecycle stubs (no-op here) ---
void EffectsConfigInit()  {}
void EffectsConfigSave()  {}

// --- Vignette ---
void SetVignetteEnabled(int v)     { gVignetteEnabled  = clamp01(v); }
int  GetVignetteEnabled()          { return gVignetteEnabled; }

void SetVignetteStrength(int v)    { gVignetteStrength = clamp05(v); }
int  GetVignetteStrength()         { return gVignetteStrength; }

void SetVignetteRadius(int v)      { gVignetteRadius   = clamp05(v); }
int  GetVignetteRadius()           { return gVignetteRadius; }

void SetVignetteSoftness(int v)    { gVignetteSoftness = clamp05(v); }
int  GetVignetteSoftness()         { return gVignetteSoftness; }

void SetVignetteWarmth(int v)      { gVignetteWarmth   = clampm100_100(v); }
int  GetVignetteWarmth()           { return gVignetteWarmth; }

// --- Film grain ---
void SetFilmGrain(int v)           { gFilmGrain = clamp01(v); }
int  GetFilmGrain()                { return gFilmGrain; }

void SetFilmGrainIntensity(int v)  { gFilmGrainIntensity = clamp05(v); }
int  GetFilmGrainIntensity()       { return gFilmGrainIntensity; }

// --- Scanlines ---
void SetScanlines(int v)           { gScanlines = clamp01(v); }
int  GetScanlines()                { return gScanlines; }

void SetScanlineIntensity(int v)   { gScanlineIntensity = clamp05(v); }
int  GetScanlineIntensity()        { return gScanlineIntensity; }

// --- Other display flags already stubbed here ---
int  GetMuzzleFlash()              { return gMuzzleFlash; }
void SetMuzzleFlash(int on)        { gMuzzleFlash = clamp01(on); }

int  GetBilinearFilter()           { return gBilinearFilter; }
void SetBilinearFilter(int on)     { gBilinearFilter = clamp01(on); }

// --- NEW: Blob shadows ON/OFF ---
int  GetBlobShadows()              { return gBlobShadows; }
void SetBlobShadows(int on)        { gBlobShadows = clamp01(on); }

} // namespace Config

// Menu adapters used by menuscreen to map warmth to boolean UI toggles in some builds.
namespace MenuAdapters {
int GetWarmth01() {
    return Config::GetVignetteWarmth() > 0 ? 1 : 0;
}
void SetWarmth01(int on) {
    Config::SetVignetteWarmth( on ? 1 : 0 );
}
} // namespace MenuAdapters
