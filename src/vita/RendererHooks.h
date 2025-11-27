#pragma once
#include <SDL2/SDL.h>

// Use global project types (do NOT nest them in RendererHooks)
class Hud;
class Font;
struct MapObject;

namespace RendererHooks {
// Existing APIs (kept for compatibility; do not remove)
bool init(SDL_Renderer* ren, int screenW, int screenH);
void shutdown();
void setTargetFps(int fps);
void setRenderSize(int w, int h);
void beginFrame();
void markWorldFrame();
void endFramePresent();
void setCameraMotion(float dx, float dy, float yawRate);
void EffectsDrawOverlaysVita2D(); // original overlay entry

// FIX: DeferHudRender now takes MapObject& (matches call site: pobj)
void DeferHudRender(::Hud* hud, ::MapObject& pobj, ::Font* font, int renderW, int renderH);

// HUD overlay wrapper that draws HUD last, over vignette/grain/scanlines
void EffectsDrawOverlaysVita2D_WithHud();
} // namespace RendererHooks
