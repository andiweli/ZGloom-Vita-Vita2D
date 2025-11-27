
#include "BootGameSelect.h"
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <SDL2/SDL.h>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include "config.h"
#include "vita/RendererVita2D.h"
#include "../assets/boot_bg_embed.h"

namespace bootselect {

// --- lightweight logger to ux0:/data/ZGloom/bootselect_log.txt ---
static void logf(const char* fmt, ...){
    char buf[512];
    va_list ap; va_start(ap, fmt);
    std::vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    const char* path = "ux0:/data/ZGloom/bootselect_log.txt";
    SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0666);
    if (fd >= 0){
        sceIoWrite(fd, buf, (SceSize)std::strlen(buf));
        const char nl = '\n';
        sceIoWrite(fd, &nl, 1);
        sceIoClose(fd);
    }
}

struct Option { const char* key; const char* label; const char* dir; };
static Option kOptions[] = {
    {"deluxe",   "GLOOM DELUXE",    "deluxe"},
    {"gloom",    "GLOOM CLASSIC",   "gloom"},
    {"gloom3",   "GLOOM 3",         "gloom3"},
    {"massacre", "ZOMBIE MASSACRE", "massacre"}, // also checks zombiemassacre
};

static const char* kBaseDir = "ux0:/data/ZGloom/";

// 8x8 uppercase bitmap font (thin I & thin T)
struct Glyph8 { char c; unsigned char r[8]; };
static const Glyph8 FONT8[] = {
{' ', {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}},
{'A',{0x18,0x24,0x42,0x7E,0x42,0x42,0x42,0x00}},
{'B',{0x7C,0x42,0x42,0x7C,0x42,0x42,0x7C,0x00}},
{'C',{0x3C,0x42,0x40,0x40,0x40,0x42,0x3C,0x00}},
{'D',{0x78,0x44,0x42,0x42,0x42,0x44,0x78,0x00}},
{'E',{0x7E,0x40,0x40,0x7C,0x40,0x40,0x7E,0x00}},
{'F',{0x7E,0x40,0x40,0x7C,0x40,0x40,0x40,0x00}},
{'G',{0x3C,0x42,0x40,0x4E,0x42,0x42,0x3C,0x00}},
{'H',{0x42,0x42,0x42,0x7E,0x42,0x42,0x42,0x00}},
{'I',{0x38,0x10,0x10,0x10,0x10,0x10,0x38,0x00}}, // thin I
{'J',{0x02,0x02,0x02,0x02,0x42,0x42,0x3C,0x00}},
{'K',{0x42,0x44,0x48,0x70,0x48,0x44,0x42,0x00}},
{'L',{0x40,0x40,0x40,0x40,0x40,0x40,0x7E,0x00}},
{'M',{0x42,0x66,0x5A,0x42,0x42,0x42,0x42,0x00}},
{'N',{0x42,0x62,0x52,0x4A,0x46,0x42,0x42,0x00}},
{'O',{0x3C,0x42,0x42,0x42,0x42,0x42,0x3C,0x00}},
{'P',{0x7C,0x42,0x42,0x7C,0x40,0x40,0x40,0x00}},
{'Q',{0x3C,0x42,0x42,0x42,0x4A,0x44,0x3A,0x00}},
{'R',{0x7C,0x42,0x42,0x7C,0x48,0x44,0x42,0x00}},
{'S',{0x3C,0x42,0x40,0x3C,0x02,0x42,0x3C,0x00}},
{'T',{0x7E,0x10,0x10,0x10,0x10,0x10,0x10,0x00}}, // thin T
{'U',{0x42,0x42,0x42,0x42,0x42,0x42,0x3C,0x00}},
{'V',{0x42,0x42,0x42,0x24,0x24,0x18,0x18,0x00}},
{'W',{0x42,0x42,0x42,0x5A,0x5A,0x66,0x42,0x00}},
{'X',{0x42,0x24,0x18,0x18,0x18,0x24,0x42,0x00}},
{'Y',{0x42,0x24,0x18,0x18,0x18,0x18,0x18,0x00}},
{'Z',{0x7E,0x04,0x08,0x10,0x20,0x40,0x7E,0x00}},
{'0',{0x3C,0x46,0x4A,0x52,0x62,0x46,0x3C,0x00}},
{'1',{0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}},
{'2',{0x3C,0x42,0x02,0x1C,0x20,0x40,0x7E,0x00}},
{'3',{0x3C,0x42,0x02,0x1C,0x02,0x42,0x3C,0x00}},
{'4',{0x04,0x0C,0x14,0x24,0x44,0x7E,0x04,0x00}},
{'5',{0x7E,0x40,0x7C,0x02,0x02,0x42,0x3C,0x00}},
{'6',{0x1C,0x20,0x40,0x7C,0x42,0x42,0x3C,0x00}},
{'7',{0x7E,0x02,0x04,0x08,0x10,0x20,0x20,0x00}},
{'8',{0x3C,0x42,0x42,0x3C,0x42,0x42,0x3C,0x00}},
{'9',{0x3C,0x42,0x42,0x3E,0x02,0x04,0x38,0x00}},
};

static const unsigned char* getRows(char c){
    if (c >= 'a' && c <= 'z') c = (char)(c - 32);
    for (size_t i=0; i<sizeof(FONT8)/sizeof(FONT8[0]); ++i){
        if (FONT8[i].c == c) return FONT8[i].r;
    }
    return FONT8[0].r; // space fallback
}

static inline void putpx(SDL_Surface* s,int x,int y,unsigned argb){
    if ((unsigned)x >= (unsigned)s->w || (unsigned)y >= (unsigned)s->h) return;
    ((unsigned*)((unsigned char*)s->pixels + y * s->pitch))[x] = argb;
}

static void drawGlyph8(SDL_Surface* s, int x, int y, char c, unsigned col, int scale=1){
    const unsigned char* rows = getRows(c);
    for (int row=0; row<8; ++row){
        unsigned char bits = rows[row];
        for (int colx=0; colx<8; ++colx){
            if (bits & (0x80 >> colx)){
                for (int dy=0; dy<scale; ++dy)
                    for (int dx=0; dx<scale; ++dx)
                        putpx(s, x + colx*scale + dx, y + row*scale + dy, col);
            }
        }
    }
}

// ---- Outline helpers (Variant 1 base style) ----
static void drawText8Outline(SDL_Surface* s, int x, int y, const char* t, unsigned col, int scale=1){
    const unsigned outline = 0x90000000;
    int ox[4] = {-1, 1, 0, 0};
    int oy[4] = {0, 0, -1, 1};
    for (int i=0;i<4;i++){
        int cx = x + ox[i];
        int cy = y + oy[i];
        for (const char* p=t; *p; ++p){
            if (*p=='\n'){ cy += 8*scale + 2; cx = x + ox[i]; continue; }
            drawGlyph8(s, cx, cy, *p, outline, scale);
            cx += 8*scale;
        }
    }
    for (const char* p=t; *p; ++p){
        if (*p=='\n'){ y += 8*scale + 2; x = 0; continue; }
        drawGlyph8(s, x, y, *p, col, scale);
        x += 8*scale;
    }
}

// TRUE bold title (extra white fill at x+1)
static void drawText8BoldTitle(SDL_Surface* s, int x, int y, const char* t, unsigned col, int scale=1){
    drawText8Outline(s, x,   y, t, col, scale);
    for (const char* p=t; *p; ++p){
        if (*p=='\n'){ y += 8*scale + 2; continue; }
        drawGlyph8(s, x+1, y, *p, col, scale);
        x += 8*scale;
    }
}

static int textWidth8_plain(const char* t, int scale=1){
    int n=0; for (const char* p=t; *p; ++p) if (*p!='\n') ++n;
    return n * 8 * scale;
}

// ---- Fractional tracking (average +0.5 px between letters) ----
static int textWidth8_tracked_half(const char* t, int scale, int num, int den){
    int n=0; for (const char* p=t; *p; ++p) if (*p!='\n') ++n;
    if (n <= 1) return n * 8 * scale;
    int gaps = n - 1;
    int extra = (gaps * num) / den; // floor
    return n * 8 * scale + extra;
}

static void drawText8OutlineTrackedHalf(SDL_Surface* s, int x, int y, const char* t, unsigned col, int scale, int num, int den){
    const unsigned outline = 0x90000000;
    int ox[4] = {-1, 1, 0, 0};
    int oy[4] = {0, 0,-1, 1};
    // Outline pass with fractional spacing
    for (int d=0; d<4; ++d){
        int cx = x + ox[d];
        int cy = y + oy[d];
        int acc = 0;
        for (const char* p=t; *p; ++p){
            if (*p=='\n'){ cy += 8*scale + 2; cx = x + ox[d]; acc = 0; continue; }
            drawGlyph8(s, cx, cy, *p, outline, scale);
            cx += 8*scale;
            acc += num;
            if (acc >= den){ cx += 1; acc -= den; }
        }
    }
    // Fill pass
    int cx = x, cy = y, acc = 0;
    for (const char* p=t; *p; ++p){
        if (*p=='\n'){ cy += 8*scale + 2; cx = x; acc = 0; continue; }
        drawGlyph8(s, cx, cy, *p, col, scale);
        cx += 8*scale;
        acc += num;
        if (acc >= den){ cx += 1; acc -= den; }
    }
}

static void fill(SDL_Surface* s, unsigned argb){
    SDL_LockSurface(s);
    for (int y=0;y<s->h;y++){
        unsigned* row=(unsigned*)((unsigned char*)s->pixels+y*s->pitch);
        for (int x=0;x<s->w;x++) row[x]=argb;
    }
    SDL_UnlockSurface(s);
}

static bool dirExists(const char* path){
    SceIoStat st; std::memset(&st,0,sizeof(st));
    return sceIoGetstat(path, &st) >= 0 && (st.st_mode & SCE_S_IFDIR);
}

static void ensureDir(const char* path){
    if (!dirExists(path)){
        int r = sceIoMkdir(path, 0777);
        logf("mkdir %s -> %d", path, r);
    }
}

// Create canonical game dirs if missing (no file checks)
static void ensureCanonicalGameDirs(){
    const char* base = kBaseDir;
    ensureDir(base);
    char buf[256];
    const char* canon[] = {"deluxe","gloom","gloom3","massacre"};
    for (int i=0;i<4;i++){
        std::snprintf(buf, sizeof(buf), "%s%s", base, canon[i]);
        ensureDir(buf);
    }
}

// ** RUN BEFORE main() ** — ensures folders exist even if Config::Init runs early
__attribute__((constructor))
static void ZGloom_PreInit_CreateGameDirs(){
    ensureCanonicalGameDirs();
    logf("preinit: ensured base + canonical game dirs");
}

static bool dirExistsNonEmpty(const char* path){
    SceUID d = sceIoDopen(path);
    if (d < 0) return false;
    SceIoDirent ent; std::memset(&ent, 0, sizeof(ent));
    int count = 0;
    while (sceIoDread(d, &ent) > 0){
        if (ent.d_name[0] != '.'){ count++; if (count >= 1) break; }
        std::memset(&ent, 0, sizeof(ent));
    }
    sceIoDclose(d);
    return count >= 1;
}

static bool gameAvailable(const char* dirSeg){
    char buf[256];
    std::snprintf(buf, sizeof(buf), "%s%s", kBaseDir, dirSeg);
    bool ok = dirExistsNonEmpty(buf);
    if (!ok && std::strcmp(dirSeg, "massacre")==0){
        std::snprintf(buf, sizeof(buf), "%szombiemassacre", kBaseDir);
        ok = dirExistsNonEmpty(buf);
    }
    logf("scan: %s -> %s", buf, ok ? "OK" : "MISSING");
    return ok;
}

static void ensureVitaInit(){
    using zgloom_vita::RendererVita2D;
    static bool inited = false;
    if (!inited){
        const int srcW = gBootBG_w;
        const int srcH = gBootBG_h;
        RendererVita2D::Get().Init(srcW, srcH, 960, 544);
        RendererVita2D::Get().SetIntegerScaling(false); // 1:1 pixel, no additional integer upscale
        RendererVita2D::Get().SetClearColor(0xFF000000);
        inited = true;
        logf("BootSelect: vita2d init (%dx%d -> 960x544)", srcW, srcH);
    }
}


static SDL_Surface* loadEmbeddedBG(){
    SDL_Surface* wrap = SDL_CreateRGBSurfaceWithFormatFrom((void*)gBootBG_ARGB, gBootBG_w, gBootBG_h, 32, gBootBG_w*4, SDL_PIXELFORMAT_ARGB8888);
    SDL_Surface* copy = SDL_ConvertSurfaceFormat(wrap, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(wrap);
    return copy;
}

static void drawMenuPage(SDL_Surface* s, SDL_Surface* bg, bool* avail, int idx, bool hideHint){
    int surfW = s->w, surfH = s->h;
    const int scale   = 2;           // 2x Font, weiterhin 1:1 native Pixels
    const int glyphH  = 8 * scale;
    const int lineH   = glyphH + 6;  // etwas Luft
    const char* title = "SELECT GAME";
    const int titleW  = (int)std::strlen(title) * 8 * scale + 1; // +1 für Bold-Offset
    const int titleGap= 10;

    // Hintergrund
    if (bg){
        SDL_Rect src{0,0,bg->w,bg->h};
        SDL_Rect dst{0,0,surfW,surfH};
        SDL_BlitScaled(bg, &src, s, &dst);
    } else {
        fill(s, 0xFF000000);
    }

    // Titel & Einträge zeichnen
    int blockH = glyphH + titleGap + 4*lineH;
    int blockY = (surfH - blockH) / 2;
    int titleX = (surfW - titleW) / 2;
    drawText8BoldTitle(s, titleX, blockY, title, 0xFFFFFFFF, scale);

    int y = blockY + glyphH + titleGap;
    for (int i=0;i<4;i++){
        const char* txt = kOptions[i].label;
        int wt = textWidth8_tracked_half(txt, scale, 1, 2);
        int x  = (surfW - wt) / 2;
        unsigned col = avail[i] ? ( (i==idx) ? 0xFFFFFF00 : 0xFFE8E8E8 )
                                : 0xFF606060; // missing -> gray
        drawText8OutlineTrackedHalf(s, x, y, txt, col, scale, 1, 2);
        y += lineH;
    }

    // Hint unten
    if (!hideHint){
        const char* hint  = "PRESS X TO START";
        int hintW = textWidth8_plain(hint, scale);
        int hintX = (surfW - hintW) / 2;
        int hintY = surfH - glyphH - 6; // 6 px bottom margin
        drawText8Outline(s, hintX, hintY, hint, 0xFFE8E8E8, scale);
    }
}

static void fadeOut(SDL_Surface* s, SDL_Surface* bg, bool* avail, int idx, bool hideHint){
    // Schnelleres ~0,2s Fade (12 Frames) – wir zeichnen jede Frame neu und dimmen sie dann
    const int frames = 12;
    for (int step=0; step<frames; ++step){
        drawMenuPage(s, bg, avail, idx, hideHint); // UI im "eingefrorenen" Zustand
        int factor = (step * 255) / (frames - 1);  // 0..255

        SDL_LockSurface(s);
        for (int yy=0; yy<s->h; ++yy){
            uint32_t* row = (uint32_t*)((uint8_t*)s->pixels + yy * s->pitch);
            for (int xx=0; xx<s->w; ++xx){
                uint32_t p = row[xx];
                uint32_t a = (p >> 24) & 0xFF;
                uint32_t r = (p >> 16) & 0xFF;
                uint32_t g = (p >>  8) & 0xFF;
                uint32_t b = (p      ) & 0xFF;
                r = (r * (255 - factor)) / 255;
                g = (g * (255 - factor)) / 255;
                b = (b * (255 - factor)) / 255;
                row[xx] = (a<<24) | (r<<16) | (g<<8) | b;
            }
        }
        SDL_UnlockSurface(s);

        using zgloom_vita::RendererVita2D;
        RendererVita2D::Get().PresentARGB8888(s->pixels, s->pitch);
        sceKernelDelayThread(16*1000); // ~60 fps
    }
}

// ---- Main selector ----
std::string Run(const char* preselectKey){
    logf("--------------------------------------------------");
    logf("BootSelect start");

    // Ensure dirs exist very early (also done via constructor, but harmless here)
    const char* base = kBaseDir; (void)base;

    SDL_Surface* s  = SDL_CreateRGBSurfaceWithFormat(0, gBootBG_w, gBootBG_h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!s){ logf("SDL_CreateRGBSurface failed"); return std::string("gloom"); }

    // Availability scan
    bool avail[4] = {false,false,false,false};
    for (int i=0;i<4;i++) avail[i] = gameAvailable(kOptions[i].dir);
    bool anyAvail = avail[0] || avail[1] || avail[2] || avail[3];

    // pick initial index
    int idx = 0;
    if (preselectKey){
        for (int i=0;i<4;i++) if (std::strcmp(kOptions[i].key, preselectKey)==0 && avail[i]){ idx=i; break; }
    }
    for (int i=0;i<4;i++) if (avail[i]){ idx=i; break; }

    ensureVitaInit();

#ifdef SCE_CTRL_MODE_ANALOG
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
#else
    sceCtrlSetSamplingMode(SCE_CTRL_MODE_DIGITAL);
#endif

    SDL_Surface* bg = loadEmbeddedBG();
    unsigned prevButtons = 0;
    bool running = true;
    bool hideHint = false;

    while (running){
        drawMenuPage(s, bg, avail, idx, hideHint);

        using zgloom_vita::RendererVita2D;
        RendererVita2D::Get().PresentARGB8888(s->pixels, s->pitch);

        // Input
        SceCtrlData pad; std::memset(&pad,0,sizeof(pad));
        sceCtrlPeekBufferPositive(0, &pad, 1);
        unsigned justPressed = (pad.buttons) & (~prevButtons);
        prevButtons = pad.buttons;
        if (justPressed & SCE_CTRL_UP){
            int i = idx, tries = 0;
            do { i = (i + 3) % 4; tries++; } while (!avail[i] && tries < 4);
            idx = i;
        }
        if (justPressed & SCE_CTRL_DOWN){
            int i = idx, tries = 0;
            do { i = (i + 1) % 4; tries++; } while (!avail[i] && tries < 4);
            idx = i;
        }
        if (justPressed & SCE_CTRL_CROSS){
            if (anyAvail && avail[idx]){
                hideHint = true;        // sofort ausblenden
                drawMenuPage(s, bg, avail, idx, true);
                using zgloom_vita::RendererVita2D;
                RendererVita2D::Get().PresentARGB8888(s->pixels, s->pitch);
                // Garantierter FadeOut 0,4s
                fadeOut(s, bg, avail, idx, true);
                running = false;
                logf("start: %s", kOptions[idx].key);
            } else {
                logf("start ignored: none available or selected missing");
            }
        }

        // Optional: Keyboard
        SDL_Event e;
        while (SDL_PollEvent(&e)){
            if (e.type == SDL_KEYDOWN){
                if (e.key.keysym.sym == SDLK_UP){
                    int i = idx, tries = 0;
                    do { i = (i + 3) % 4; tries++; } while (!avail[i] && tries < 4);
                    idx = i;
                } else if (e.key.keysym.sym == SDLK_DOWN){
                    int i = idx, tries = 0;
                    do { i = (i + 1) % 4; tries++; } while (!avail[i] && tries < 4);
                    idx = i;
                } else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_SPACE){
                    if (anyAvail && avail[idx]){
                        hideHint = true;
                        drawMenuPage(s, bg, avail, idx, true);
                        using zgloom_vita::RendererVita2D;
                        RendererVita2D::Get().PresentARGB8888(s->pixels, s->pitch);
                        fadeOut(s, bg, avail, idx, true);
                        running = false;
                        logf("start (kbd): %s", kOptions[idx].key);
                    } else {
                        logf("start (kbd) ignored: none available or selected missing");
                    }
                }
            }
        }

        sceKernelDelayThread(16*1000); // ~60 fps
    }

    if (bg) SDL_FreeSurface(bg);
    std::string key = anyAvail ? kOptions[idx].key : std::string("gloom");
    SDL_FreeSurface(s);
    logf("BootSelect end -> %s", key.c_str());
    return key;
}

} // namespace bootselect
