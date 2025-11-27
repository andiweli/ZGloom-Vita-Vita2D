#include "vita/RendererHooks.h"
#include <vita2d.h>
#include <vector>
#include <cstring>

namespace RendererHooks {

// Small capture buffer (RGBA), capture size, base render size
static std::vector<uint32_t> g_hudBuf;
static int g_hudW = 0, g_hudH = 0;
static int g_hudBaseW = 320, g_hudBaseH = 256;

// Texture reused for HUD sprite
static vita2d_texture* g_hudTex = nullptr;

// Provide a no-op fallback for setCameraMotion (in case your main .cpp doesn't define it)
void setCameraMotion(float, float, float) {}

// Capture a sub-rect from the ARGB8888 SDL surface and store as RGBA
void SetHudCapture(SDL_Surface* surf, int x, int y, int w, int h) {
    if (!surf || !surf->pixels || w <= 0 || h <= 0) {
        g_hudW = g_hudH = 0; g_hudBuf.clear();
        return;
    }
    if (x < 0) x = 0; if (y < 0) y = 0;
    if (x + w > surf->w) w = surf->w - x;
    if (y + h > surf->h) h = surf->h - y;
    if (w <= 0 || h <= 0) {
        g_hudW = g_hudH = 0; g_hudBuf.clear();
        return;
    }

    g_hudW = w; g_hudH = h;
    g_hudBaseW = surf->w; g_hudBaseH = surf->h;
    g_hudBuf.resize((size_t)w * (size_t)h);

    const uint8_t* src = (const uint8_t*)surf->pixels;
    const int pitch = surf->pitch;
    for (int row = 0; row < h; ++row) {
        const uint32_t* srow = (const uint32_t*)(src + (y + row) * pitch) + x;
        uint32_t* drow = &g_hudBuf[(size_t)row * (size_t)w];
        for (int col = 0; col < w; ++col) {
            uint32_t p = srow[col];          // ARGB
            uint8_t a = (p >> 24) & 0xFF;
            uint8_t r = (p >> 16) & 0xFF;
            uint8_t g = (p >> 8)  & 0xFF;
            uint8_t b = (p      ) & 0xFF;
            drow[col] = (r << 24) | (g << 16) | (b << 8) | (a); // RGBA
        }
    }
}

// Wrapper: draw your original overlays first, then draw HUD LAST on top
void EffectsDrawOverlaysVita2D_HUDWrapper() {
    // 1) keep existing behavior
    EffectsDrawOverlaysVita2D();

#ifdef __vita__
    // 2) draw captured HUD last (letterbox-aware for Vita 960x544, integer scaling)
    if (g_hudW > 0 && g_hudH > 0 && !g_hudBuf.empty()) {
        const int SW = 960, SH = 544;
        const int RW = (g_hudBaseW > 0 ? g_hudBaseW : 320);
        const int RH = (g_hudBaseH > 0 ? g_hudBaseH : 256);

        int sx = SW / RW; if (sx < 1) sx = 1;
        int sy = SH / RH; if (sy < 1) sy = 1;
        int sc = (sx < sy) ? sx : sy;

        int drawW = RW * sc;
        int drawH = RH * sc;
        int dx = (SW - drawW) / 2;
        int dy = (SH - drawH) / 2;

        // Ensure texture
        if (!g_hudTex || vita2d_texture_get_width(g_hudTex) != g_hudW || vita2d_texture_get_height(g_hudTex) != g_hudH) {
            if (g_hudTex) { vita2d_free_texture(g_hudTex); g_hudTex = nullptr; }
            g_hudTex = vita2d_create_empty_texture_format(g_hudW, g_hudH, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA);
        }
        if (g_hudTex) {
            void* tp = vita2d_texture_get_datap(g_hudTex);
            std::memcpy(tp, g_hudBuf.data(), (size_t)g_hudW * (size_t)g_hudH * 4);
            vita2d_draw_texture_scale(g_hudTex, (float)dx, (float)dy, (float)sc, (float)sc);
        }
    }
#endif
}

} // namespace RendererHooks
