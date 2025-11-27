
#include "vita/RendererHooks.h"

#include "hud.h"
#include "font.h"
struct MapObject;

#ifdef __vita__
#include <vita2d.h>
#endif

#include <vector>
#include <cstring>
#include <algorithm>

namespace RendererHooks {

static ::Hud*        g_deferHud  = nullptr;
static ::MapObject   g_deferObjSnapshot;
static ::MapObject*  g_deferObj  = nullptr;
static ::Font*       g_deferFont = nullptr;
static int           g_RW = 320; // render width (world)
static int           g_RH = 256; // render height (world)

static SDL_Surface*       g_hudSurf = nullptr;          // ARGB8888 @ HUD base size
static std::vector<uint32_t> g_rgbaBuf;                 // RGBA upload buffer
#ifdef __vita__
static vita2d_texture*    g_hudTex  = nullptr;          // RGBA vita2d texture
#endif

// HUD virtual base resolution (keeps 4:3 HUD/gun)
static const int   kHudBaseW   = 320;
static const int   kHudBaseH   = 256;

// X-only scaling factor (15% narrower)
static const float kHudNarrowX = 0.85f;

// HUD global scale factor (applied on top of Present's integer scale)
static const float kHudScale   = 1.50f;

// Call from game loop instead of direct hud.Render(...)
void DeferHudRender(::Hud* hud, ::MapObject& pobj, ::Font* font, int renderW, int renderH) {
    // Take a snapshot of the player object so we don't reference a stack object later.
    g_deferHud         = hud;
    g_deferObjSnapshot = pobj;
    g_deferObj         = &g_deferObjSnapshot;
    g_deferFont        = font;

    // Remember the current world render size for correct letterboxing of the HUD.
    g_RW = (renderW > 0 ? renderW : 320);
    g_RH = (renderH > 0 ? renderH : 256);
}

void EffectsDrawOverlaysVita2D(); // provided elsewhere

void EffectsDrawOverlaysVita2D_WithHud() {
    // 1) Original overlays (vignette/grain/scanlines) unchanged
    EffectsDrawOverlaysVita2D();

#ifdef __vita__
    // 2) Draw HUD last (on top)
    if (g_deferHud && g_deferObj && g_deferFont) {
        // Create/resize ARGB surface at a fixed HUD base resolution (4:3) so HUD/gun stay undistorted,
        // even if the world render width changes (e.g., 320 -> 452 for 16:9).
        const int hudW = kHudBaseW;
        const int hudH = kHudBaseH;
        if (!g_hudSurf || g_hudSurf->w != hudW || g_hudSurf->h != hudH) {
            if (g_hudSurf) { SDL_FreeSurface(g_hudSurf); g_hudSurf = nullptr; }
            g_hudSurf = SDL_CreateRGBSurfaceWithFormat(0, hudW, hudH, 32, SDL_PIXELFORMAT_ARGB8888);
        }
        if (g_hudSurf) {
            // Clear transparent and render HUD into it
            SDL_FillRect(g_hudSurf, nullptr, 0x00000000u);
            g_deferHud->Render(g_hudSurf, *g_deferObj, *g_deferFont);

            const int W = g_hudSurf->w, H = g_hudSurf->h;
            g_rgbaBuf.resize((size_t)W * (size_t)H);
            const uint8_t* src = (const uint8_t*)g_hudSurf->pixels;
            const int pitch = g_hudSurf->pitch;
            for (int y=0; y<H; ++y) {
                const uint32_t* srow = (const uint32_t*)(src + y * pitch);
                uint32_t* drow = &g_rgbaBuf[(size_t)y * (size_t)W];
                for (int x=0; x<W; ++x) {
                    uint32_t p = srow[x]; // ARGB
                    uint8_t a = (p >> 24) & 0xFF;
                    uint8_t r = (p >> 16) & 0xFF;
                    uint8_t g = (p >> 8)  & 0xFF;
                    uint8_t b = (p      ) & 0xFF;
                    drow[x] = (r << 24) | (g << 16) | (b << 8) | (a); // RGBA
                }
            }

        #ifdef __vita__
            // Create/resize texture and upload
            if (!g_hudTex || vita2d_texture_get_width(g_hudTex) != W || vita2d_texture_get_height(g_hudTex) != H) {
                if (g_hudTex) { vita2d_free_texture(g_hudTex); g_hudTex = nullptr; }
                g_hudTex = vita2d_create_empty_texture_format(W, H, SCE_GXM_TEXTURE_FORMAT_U8U8U8U8_RGBA);
            }
            if (g_hudTex) {
                void* tp = vita2d_texture_get_datap(g_hudTex);
                std::memcpy(tp, g_rgbaBuf.data(), (size_t)W * (size_t)H * 4);

                // Anisotropic integer scaling to match Present() exactly
                const int SW = 960, SH = 544;   // Vita display
                const int RW = g_RW, RH = g_RH; // render size
                int scx = SW / RW; if (scx < 1) scx = 1;
                int scy = SH / RH; if (scy < 1) scy = 1;
                // Base world scale (matches Present())
                const float sx_world  = (float)scx;
                const float sy_world  = (float)scy;
                // HUD global scale on top of world scale
                const float sx_full   = sx_world * kHudScale;
                const float sy_full   = sy_world * kHudScale;
                const float sx_narrow = sx_full * kHudNarrowX; // 15% narrower in X

                // Letterbox origin for full-width (unscaled X)
                int drawW = RW * scx;
                int drawH = RH * scy;
                int dx = (SW - drawW) / 2;
                int dy = (SH - drawH) / 2;

                // --- Split draw ---
                // Top-left HUD panel (e.g., 128x40) at absolute screen (0,0), narrower in X
                int TLW = std::min(128, W);
                int TLH = std::min(40,  H);
                if (TLW > 0 && TLH > 0) {
                    vita2d_draw_texture_part_scale(g_hudTex,
                                                   /*dest*/ 0.0f, 0.0f,
                                                   /*src*/  0.0f, 0.0f,
                                                   /*w,h*/  (float)TLW, (float)TLH,
                                                   /*scale*/ sx_narrow, sy_full);
                }

                // Right strip of top row (letterbox-aligned, full scale)
                if (W - TLW > 0 && TLH > 0) {
                    vita2d_draw_texture_part_scale(g_hudTex,
                                                   /*dest*/ (float)(dx + TLW * scx), (float)dy,
                                                   /*src*/  (float)TLW, 0.0f,
                                                   /*w,h*/  (float)(W - TLW), (float)TLH,
                                                   /*scale*/ sx_full, sy_full);
                }

                // Bottom area (full width below TLH): center horizontally within letterbox,
                // anchor to SCREEN bottom; X only narrower, Y scale unchanged.
                if (H - TLH > 0) {
                    const float bottom_w_px = (float)W * sx_narrow;
                    const float letter_w_px = (float)drawW;
                    const float dest_x = (float)dx + (letter_w_px - bottom_w_px) * 0.5f; // center horizontally
                    const float dest_y = (float)SH - (float)(H - TLH) * sy_full;          // flush to screen bottom

                    vita2d_draw_texture_part_scale(g_hudTex,
                                                   /*dest*/ dest_x, dest_y,
                                                   /*src*/  0.0f, (float)TLH,
                                                   /*w,h*/  (float)W, (float)(H - TLH),
                                                   /*scale*/ sx_narrow, sy_full);
                }
            }
        #endif
        }
    }
#endif

    // consume for next frame
    g_deferHud = nullptr; g_deferObj = nullptr; g_deferFont = nullptr;
}

} // namespace RendererHooks
