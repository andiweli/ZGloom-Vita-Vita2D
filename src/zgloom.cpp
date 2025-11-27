// zgloom.cpp : PS Vita main (ZGloom-Vita)
// Drop-in generated: 2025-11-10 (ZM save/load fix: play-index spooling + name/play alignment)
// - Resume/Load spools strictly by play index (spoolByPlayIndex=true) for Zombie Massacre
// - Level names aligned to actual playPaths length (fallback from basename)
// - Preserves all prior features (EventReplay, EmbeddedBGMVolume, vita2d present, etc.)

#include <psp2/sysmodule.h>
#include <psp2/kernel/clib.h>
#include <psp2/apputil.h>
#include <psp2/ctrl.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/clib.h>
#include <psp2/io/dirent.h>

#include <stdio.h>
#include <cctype>
#include <string>
#include <vector>
#include <cstdarg>
#include <xmp.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

#include "config.h"
#include "vita/RendererHooks.h"
#include "gloommap.h"
#include "script.h"
#include "crmfile.h"
#include "iffhandler.h"
#include "renderer.h"
#include "objectgraphics.h"
#include <iostream>
#include "gamelogic.h"
#include "soundhandler.h"
#include "font.h"
#include "titlescreen.h"
#include "SaveSystem.h"
#include "EventReplay.h"
#include "menuscreen.h"
#include "hud.h"
#include <fstream>
#include "vita/RendererVita2D.h"
#include "effects/MuzzleFlashFX.h"
#include "BootGameSelect.h"
#include "audio/EmbeddedBGM.h"
#include "audio/AtmosphereVolume.h"
#include "EmbeddedBGMVolume.h"

#ifndef ZGLOOM_LEVELLOG
#define ZGLOOM_LEVELLOG 0
#endif

// --------- helpers: resolve level index from saved path ----------
static std::string ToLower(std::string s) {
    for (auto& c : s) { c = (char)std::tolower((unsigned char)c); }
    return s;
}

static int FindLevelIndexFromPath(const std::string& fullPath, const std::vector<std::string>& levelnames) {
    // basename
    std::string base = fullPath;
    size_t p = base.find_last_of("/\\");
    if (p != std::string::npos) base = base.substr(p + 1);

    std::string fullLow = ToLower(fullPath);
    std::string baseLow = ToLower(base);

    // 1) Exact basename match
    for (size_t i = 0; i < levelnames.size(); ++i) {
        if (baseLow == ToLower(levelnames[i])) return (int)i;
    }
    // 2) Ends-with match (full path ends with level name)
    for (size_t i = 0; i < levelnames.size(); ++i) {
        auto nameLow = ToLower(levelnames[i]);
        if (fullLow.size() >= nameLow.size() &&
            fullLow.compare(fullLow.size() - nameLow.size(), nameLow.size(), nameLow) == 0)
            return (int)i;
    }
    // 3) Substring match as last resort
    for (size_t i = 0; i < levelnames.size(); ++i) {
        if (fullLow.find(ToLower(levelnames[i])) != std::string::npos) return (int)i;
    }
    return 0; // fallback
}

static int MapIndexFromSavedPath(const std::string& saved, const std::vector<std::string>& levelnames) {
    // Use the original logic: exact match against Config::GetLevelDir() + levelname,
    // then an "ends-with levelname" check. Fallback to heuristic if needed.
    const std::string prefix = Config::GetLevelDir();
    for (size_t i = 0; i < levelnames.size(); ++i) {
        const std::string p = prefix + levelnames[i];
        if (saved == p) return (int)i;
        if (saved.size() >= levelnames[i].size() &&
            saved.compare(saved.size() - levelnames[i].size(), levelnames[i].size(), levelnames[i]) == 0)
            return (int)i;
    }
    return FindLevelIndexFromPath(saved, levelnames);
}

// --- lightweight file logger ---
#if ZGLOOM_LEVELLOG
static void levellog(const char* fmt, ...) {
    FILE* f = fopen("ux0:/data/ZGloom/levellog.txt", "a");
    if (!f) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\\n', f);
    fclose(f);
}
#else
#define levellog(...) ((void)0)
#endif

// --- collect ordered list of playable level paths (from SOP_PLAY) at startup
static int iequals_char(int a, int b) { return std::tolower((unsigned char)a) == std::tolower((unsigned char)b); }
static bool ends_with_case_insensitive(const std::string& s, const std::string& suffix) {
    if (suffix.size() > s.size()) return false;
    return std::equal(s.end()-suffix.size(), s.end(), suffix.begin(), iequals_char);
}
// --- directory-based map ordering (ux0:/data/ZGloom/maps) ---
#include <psp2/io/fcntl.h>
#include <algorithm>

static int g_pendingApplyDelayFrames = 0; // global: delay save-apply frames

struct MapEntry { int ep; int idx; std::string nameNoExt; };

static bool parse_map_name(const std::string& n, int& ep, int& idx, std::string& nameNoExt) {
    std::string s = n;
    size_t dot = s.find_last_of('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    if (s.size() < 4) return false;
    if (!((s[0]=='m'||s[0]=='M') && (s[1]=='a'||s[1]=='A') && (s[2]=='p'||s[2]=='P'))) return false;
    std::string rest = s.substr(3);
    if (rest.empty()) return false;
    size_t us = rest.find('_');
    if (us != std::string::npos) {
        std::string a = rest.substr(0, us);
        std::string b = rest.substr(us+1);
        if (a.empty() || b.empty()) return false;
        ep = 0; idx = 0;
        for (char c: a) { if (c<'0'||c>'9') return false; ep = ep*10 + (c-'0'); }
        for (char c: b) { if (c<'0'||c>'9') return false; idx = idx*10 + (c-'0'); }
        nameNoExt = s;
        return true;
    }
    size_t pos = 0;
    ep = 0; idx = 0;
    while (pos < rest.size() && rest[pos] >= '0' && rest[pos] <= '9') {
        ep = ep*10 + (rest[pos]-'0');
        ++pos;
    }
    if (pos == 0 || pos >= rest.size()) return false;
    char lc = (char)std::tolower((unsigned char)rest[pos]);
    if (lc < 'a' || lc > 'z') return false;
    idx = (lc - 'a') + 1;
    nameNoExt = s;
    return true;
}

static void CollectPlayPathsFromMapsDir(std::vector<std::string>& out) {
    out.clear();
    std::string dir = Config::GetLevelDir(); // e.g., ux0:/data/ZGloom/deluxe/maps/
    levellog("[SCAN MAPS] dir=%s", dir.c_str());
    int d = sceIoDopen(dir.c_str());
    if (d >= 0) {
        SceIoDirent ent; memset(&ent, 0, sizeof(ent));
        while (sceIoDread(d, &ent) > 0) {
            std::string nm = ent.d_name;
            if (nm == "." || nm == "..") { memset(&ent, 0, sizeof(ent)); continue; }
            int ep=0, idx=0; std::string noext;
            if (parse_map_name(nm, ep, idx, noext)) {
                out.push_back(dir + noext);
                levellog("[SCAN MAPS] %s -> add %s", nm.c_str(), (dir + noext).c_str());
            }
            memset(&ent, 0, sizeof(ent));
        }
        sceIoDclose(d);
    }
    if (out.empty()) {
        levellog("[SCAN MAPS] empty dir or no matches -> fallback to Script scan");
        Script scan;
        std::string s;
        while (true) {
            Script::ScriptOp op = scan.NextLine(s);
            if (op == Script::SOP_END) break;
            if (op == Script::SOP_PLAY) {
                std::string p = s;
                p.insert(0, Config::GetLevelDir());
                out.push_back(p);
                levellog("[SCAN PLAY] add %s", p.c_str());
            }
        }
    }
    levellog("[SCAN MAPS] collected %d entries", (int)out.size());
}

// Replace GetPlayIndexFromPath to be extension/case tolerant on basename
static std::string basename_noext(const std::string& p) {
    std::string s = p;
    size_t slash = s.find_last_of("/\\");
    if (slash != std::string::npos) s = s.substr(slash+1);
    size_t dot = s.find_last_of('.');
    if (dot != std::string::npos) s = s.substr(0, dot);
    for (auto& c: s) c = (char)std::tolower((unsigned char)c);
    return s;
}
static int GetPlayIndexFromPath(const std::vector<std::string>& playPaths, const std::string& fullPath) {
    std::string target = basename_noext(fullPath);
    for (size_t i=0;i<playPaths.size();++i) {
        if (basename_noext(playPaths[i]) == target) return (int)i;
    }
    return -1;
}

// -----------------------------------------------------------------

Uint32 my_callbackfunc(Uint32 interval, void *param)
{
    SDL_Event event;
    SDL_UserEvent userevent;
    userevent.type = SDL_USEREVENT;
    userevent.code = 0;
    userevent.data1 = NULL;
    userevent.data2 = NULL;
    event.type = SDL_USEREVENT;
    event.user = userevent;
    SDL_PushEvent(&event);
    return (interval);
}

static void fill_audio(void *udata, Uint8 *stream, int len)
{
    (void)xmp_play_buffer((xmp_context)udata, stream, len, 0);
}

void LoadPic(std::string name, SDL_Surface *render8)
{
    std::vector<uint8_t> pic;
    CrmFile picfile;
    CrmFile palfile;

    picfile.Load(name.c_str());
    palfile.Load((name + ".pal").c_str());

    SDL_FillRect(render8, nullptr, 0);

    for (uint32_t c = 0; c < palfile.size / 4; c++)
    {
        SDL_Color col;
        col.a = 0xFF;
        col.r = palfile.data[c * 4 + 0] & 0xf;
        col.g = palfile.data[c * 4 + 1] >> 4;
        col.b = palfile.data[c * 4 + 1] & 0xF;

        col.r <<= 4; col.g <<= 4; col.b <<= 4;

        col.r |= palfile.data[c * 4 + 2] & 0xf;
        col.g |= palfile.data[c * 4 + 3] >> 4;
        col.b |= palfile.data[c * 4 + 3] & 0xF;

        SDL_SetPaletteColors(render8->format->palette, &col, c, 1);
    }

    uint32_t width = 0;
    IffHandler::DecodeIff(picfile.data, pic, width);

    if (width == (uint32_t)render8->w)
    {
        if (pic.size() > (size_t)(render8->w * render8->h))
            pic.resize(render8->w * render8->h);
        std::copy(pic.begin(), pic.begin() + pic.size(), (uint8_t *)(render8->pixels));
    }
    else
    {
        uint32_t p = 0, y = 0;
        if (pic.size() > (width * (uint32_t)render8->h)) pic.resize(width * render8->h);
        while (p < pic.size())
        {
            std::copy(pic.begin() + p, pic.begin() + p + render8->w, (uint8_t *)(render8->pixels) + y * render8->pitch);
            p += width; y++;
        }
    }
}


// Helper: add subtle 16:9-style sidebars for 320x256 title/intermission
// src320: 320x256 ARGB surface containing the title/intermission
// dst   : render surface (renderwidth x renderheight)
// center: rectangle where the centered 4:3 image was drawn
static void AddSidebarsFrom320(SDL_Surface* src320, SDL_Surface* dst, const SDL_Rect& center)
{
    if (!src320 || !dst) return;

    const int renderW = dst->w;

    SDL_Rect dstLeft{0, center.y, center.x, center.h};
    SDL_Rect dstRight{center.x + center.w, center.y,
                      renderW - (center.x + center.w), center.h};

    if (dstLeft.w <= 0 && dstRight.w <= 0)
        return;

    const int stripW = 8; // schmale Streifen → Verzerrung weniger auffällig
    SDL_Rect srcLeft{0, 0, stripW, src320->h};
    SDL_Rect srcRight{src320->w - stripW, 0, stripW, src320->h};

    // Seiten nur sehr leicht abdunkeln → ca. 90% Helligkeit
    SDL_SetSurfaceColorMod(src320, 230, 230, 230);

    if (dstLeft.w > 0)
        SDL_BlitScaled(src320, &srcLeft, dst, &dstLeft);
    if (dstRight.w > 0)
        SDL_BlitScaled(src320, &srcRight, dst, &dstRight);

    SDL_SetSurfaceColorMod(src320, 255, 255, 255);
}


enum GameState { STATE_PLAYING, STATE_PARSING, STATE_SPOOLING, STATE_WAITING, STATE_MENU, STATE_TITLE };

int main(int argc, char *argv[])
{
    sceClibPrintf("\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n\\n");
    char buffer[2048]; memset(buffer, 0, sizeof(buffer));

    SceAppUtilInitParam initParam = {}; sceClibMemset(&initParam, 0, sizeof(initParam));
    SceAppUtilBootParam bootParam = {}; sceClibMemset(&bootParam, 0, sizeof(bootParam));
    sceAppUtilInit(&initParam, &bootParam);
    SceAppUtilAppEventParam eventParam; sceClibMemset(&eventParam, 0, sizeof(SceAppUtilAppEventParam));
    sceAppUtilReceiveAppEvent(&eventParam);
    sceClibPrintf("\\nEventType:%d\\n",eventParam.type);
    if (eventParam.type == 0x05)
    {
        sceAppUtilAppEventParseLiveArea(&eventParam, buffer);
        if (strstr(buffer, "deluxe"))      { Config::SetGame(Config::GameTitle::DELUXE); }
        else if (strstr(buffer, "gloom3")) { Config::SetGame(Config::GameTitle::GLOOM3); }
        else if (strstr(buffer, "massacre")) { Config::SetGame(Config::GameTitle::MASSACRE); }
    }
    else { Config::SetGame(Config::GameTitle::GLOOM); }

    if (int dirID = sceIoDopen((Config::GetGamePath()).c_str()) >= 0) { sceIoDclose(dirID); }
    else { sceIoDclose(dirID); return 0; }

    if (strstr(buffer, "massacre")) {
        if (FILE *file = fopen("ux0:/data/ZGloom/massacre/stuf/stages", "r")) { Config::SetZM(true); fclose(file); }
    }

    if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
    { std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl; return 1; }

    Config::Init();

    // --- Pre-Boot game selection (PS Vita) ---
#ifdef __vita__
    {
        std::string _sel = bootselect::Run(nullptr);
        if (!_sel.empty()) {
            Config::GameTitle t = Config::GLOOM;
            if (_sel == "deluxe")      t = Config::DELUXE;
            else if (_sel == "gloom")  t = Config::GLOOM;
            else if (_sel == "gloom3") t = Config::GLOOM3;
            else if (_sel == "massacre") t = Config::MASSACRE;
            Config::SetGame(t);
            Config::SetZM(t == Config::MASSACRE);
            Config::Init();
        }
    }
#endif
    // --- End pre-boot selection ---

    GloomMap gmap;
    Script script;
    TitleScreen titlescreen;
    MenuScreen menuscreen;
    GameState state = STATE_TITLE;
    xmp_context ctx = xmp_create_context();
    Config::RegisterMusContext(ctx);

    int renderwidth, renderheight, windowwidth, windowheight;
    Config::GetRenderSizes(renderwidth, renderheight, windowwidth, windowheight);

    CrmFile titlemusic, intermissionmusic, ingamemusic, titlepic;
    titlemusic.Load(Config::GetMusicFilename(0).c_str());
    intermissionmusic.Load(Config::GetMusicFilename(1).c_str());

    SoundHandler::Init();

    SDL_Window *win = SDL_CreateWindow("ZGloom", 100, 100, windowwidth, windowheight, SDL_WINDOW_SHOWN | (Config::GetFullscreen() ? SDL_WINDOW_FULLSCREEN : 0));
    if (!win) { std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl; return 1; }
    Config::RegisterWin(win);

#ifndef __vita__
    SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | (Config::GetVSync() ? SDL_RENDERER_PRESENTVSYNC : 0));
    if (!ren) { std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl; return 1; }
#else
    SDL_Renderer *ren = nullptr; // Vita: vita2d presents
#endif

    RendererHooks::init(ren, windowwidth, windowheight);
    RendererHooks::setTargetFps(Config::GetMaxFps());
            RendererHooks::setRenderSize(renderwidth, renderheight);

#ifndef __vita__
    SDL_Texture *rendertex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, renderwidth, renderheight);
    if (!rendertex) { std::cout << "SDL_CreateTexture Error: " << SDL_GetError() << std::endl; return 1; }
#else
    SDL_Texture *rendertex = nullptr;
#endif

    SDL_ShowCursor(SDL_DISABLE);

    SDL_Surface *render8 = SDL_CreateRGBSurface(0, 320, 256, 8, 0,0,0,0);
    SDL_Surface *intermissionscreen = SDL_CreateRGBSurface(0, 320, 256, 8, 0,0,0,0);
    SDL_Surface *titlebitmap = SDL_CreateRGBSurface(0, 320, 256, 8, 0,0,0,0);
    SDL_Surface *render32 = SDL_CreateRGBSurface(0, renderwidth, renderheight, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
    SDL_Surface *screen32 = SDL_CreateRGBSurface(0, 320, 256, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

    ObjectGraphics objgraphics;
    Renderer renderer;
    GameLogic logic;
    Camera cam;
    Hud hud;

    logic.Init(&objgraphics);
    SDL_AddTimer(1000 / 25, my_callbackfunc, NULL);

    SDL_Event sEvent;
    bool notdone = true;

    Font smallfont, bigfont;
    CrmFile fontfile;
    fontfile.Load((Config::GetMiscDir() + "bigfont2.bin").c_str());
    if (fontfile.data) { bigfont.Load2(fontfile); smallfont.Load2(fontfile); }
    else {
        fontfile.Load((Config::GetMiscDir() + "smallfont.bin").c_str());
        if (fontfile.data) smallfont.Load(fontfile);
        fontfile.Load((Config::GetMiscDir() + "bigfont.bin").c_str());
        if (fontfile.data) bigfont.Load(fontfile);
    }

    bool success = titlepic.Load((Config::GetPicsDir() + "title").c_str());
    if (titlepic.data) { LoadPic(Config::GetPicsDir() + "title", titlebitmap); }
    else { LoadPic(Config::GetPicsDir() + "spacehulk", titlebitmap); }

    if (titlemusic.data) {
        if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size)) { std::cout << "music error"; }
        if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
        Mix_HookMusic(fill_audio, ctx);
        Config::SetMusicVol(Config::GetMusicVol());
    }

    std::string intermissiontext;
    bool intermissionmusplaying = false;
    bool haveingamemusic = false;
    bool printscreen = false;
    int screennum = 0;
    uint32_t fps = 0, fpscounter = 0;

    Mix_Volume(-1, Config::GetSFXVol() * 12);
    Mix_VolumeMusic(Config::GetMusicVol() * 12);

    SDL_Rect blitrect;
    int screenscale = renderheight / 256;
    blitrect.w = 320 * screenscale; blitrect.h = 256 * screenscale;
    blitrect.x = (renderwidth - blitrect.w) / 2;
    blitrect.y = (renderheight - blitrect.h) / 2;

    SDL_SetRelativeMouseMode(SDL_TRUE);

    std::vector<std::string> levelnames;
    std::vector<std::string> playPaths;

    // ZM-safe order: collect playable paths first, then names, then align
    CollectPlayPathsFromMapsDir(playPaths);
    script.GetLevelNames(levelnames);
    if (levelnames.size() != playPaths.size()) {
        levelnames.resize(playPaths.size());
        for (size_t i = 0; i < levelnames.size(); ++i) {
            if (levelnames[i].empty()) levelnames[i] = basename_noext(playPaths[i]);
        }
    }
    titlescreen.SetLevels(levelnames);

    int levelselect = 0;
    int currentCampaignIndex = 0;             // reliably tracks campaign index of the *played* level
    bool loadedFromSave = false;              // single-use flag to spool to next level after a loaded game
    std::string currentLevelPath;

    // Pending save application after script-driven SOP_PLAY load (fixes movers/rotators & collision desync)
    static bool g_hasPendingSaveApply = false;
    static SaveSystem::SaveData g_sdPending;
    static int  g_pendingPlayIndex = -1;

    bool spoolByPlayIndex = false;
    int  spoolTargetIndex = -1;
    int  spoolPlayCounter = -1;               // tracks the actually loaded level path

    sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);
    SceCtrlData inputData;

    Input::Init();
    while (notdone)
    {
        RendererHooks::beginFrame();
        sceCtrlPeekBufferPositive(0, &inputData, 1);

        if ((state == STATE_PARSING) || (state == STATE_SPOOLING))
        {
            std::string scriptstring;
            Script::ScriptOp sop = script.NextLine(scriptstring);
            switch (sop)
            {
            case Script::SOP_SETPICT:
                scriptstring.insert(0, Config::GetPicsDir());
                LoadPic(scriptstring, intermissionscreen);
                SDL_SetPaletteColors(render8->format->palette, intermissionscreen->format->palette->colors, 0, 256);
                break;
            case Script::SOP_SONG:
                scriptstring.insert(0, Config::GetMusicDir());
                ingamemusic.Load(scriptstring.c_str());
                haveingamemusic = (ingamemusic.data != nullptr);
                break;
            case Script::SOP_LOADFLAT:
                gmap.SetFlat(scriptstring[0] - '0');
                SaveSystem::SetCurrentFlat(scriptstring[0] - '0');
                break;
            case Script::SOP_TEXT:
                intermissiontext = scriptstring;
                if (state == STATE_SPOOLING)
                {
                    levellog("[SPOOL TEXT] text='%s' target='%s' spoolByPlayIndex=%d",
                             intermissiontext.c_str(), levelnames[levelselect].c_str(), (int)spoolByPlayIndex);
                    if (!spoolByPlayIndex && (intermissiontext == levelnames[levelselect]))
                    {
                        // lock in campaign index at the selected level while spooling
                        currentCampaignIndex = levelselect;
                        if (intermissionmusic.data)
                        {
                            if (xmp_load_module_from_memory(ctx, intermissionmusic.data, intermissionmusic.size)) { std::cout << "music error"; }
                            if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
                            Mix_HookMusic(fill_audio, ctx);
                            Config::SetMusicVol(Config::GetMusicVol());
                            intermissionmusplaying = true;
                        }
                        state = STATE_PARSING;
                    }
                }
                break;
            case Script::SOP_DRAW:
                if (state == STATE_PARSING)
                {
                    if (intermissionmusic.data)
                    {
                        if (xmp_load_module_from_memory(ctx, intermissionmusic.data, intermissionmusic.size)) { std::cout << "music error"; }
                        if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
                        Mix_HookMusic(fill_audio, ctx);
                        Config::SetMusicVol(Config::GetMusicVol());
                        intermissionmusplaying = true;
                    }
                }
                break;
            case Script::SOP_WAIT:
                if (state == STATE_PARSING)
                {
                    state = STATE_WAITING;
                    SDL_SetPaletteColors(render8->format->palette, smallfont.GetPalette()->colors, 0, 16);
                    SDL_BlitSurface(intermissionscreen, NULL, render8, NULL);
                    smallfont.PrintMultiLineMessage(intermissiontext, 245, render8);
                }
                break;
            case Script::SOP_PLAY:
                // If we are spooling by play-index: count plays and flip to PARSING at the target
                if (state == STATE_SPOOLING) {
                    levellog("[SPOOL PLAY] counter(before)=%d target=%d totalPlays=%d",
                             spoolPlayCounter, spoolTargetIndex, (int)playPaths.size());
                    ++spoolPlayCounter;
                    if (spoolByPlayIndex && spoolPlayCounter == spoolTargetIndex) {
                        levellog("[SPOOL PLAY] HIT target at %d -> switch to PARSING", spoolPlayCounter);
                        state = STATE_PARSING;
                        spoolByPlayIndex = false; // one-shot
                    }
                }

                if (state == STATE_PARSING)
                {
                    cam.x.SetInt(0); cam.y = 120; cam.z.SetInt(0); cam.rotquick.SetInt(0);
                    scriptstring.insert(0, Config::GetLevelDir());

                    // Bullet-proof: recompute index from actual script path
                    {
                        int idxFromPlay = GetPlayIndexFromPath(playPaths, scriptstring);
                        levellog("[SOP_PLAY DIR] script=%s -> playIdx=%d", scriptstring.c_str(), idxFromPlay);
                        if (idxFromPlay >= 0) { currentCampaignIndex = idxFromPlay; levelselect = idxFromPlay; }
                    }

                    SaveSystem::SetCurrentLevelPath(scriptstring);
                    currentLevelPath = scriptstring;

                    gmap.Load(scriptstring.c_str(), &objgraphics);
                    renderer.Init(render32, &gmap, &objgraphics);
                    logic.InitLevel(&gmap, &cam, &objgraphics);
                    // Clear event replay log on fresh level starts
                    if (!(g_hasPendingSaveApply && (g_pendingPlayIndex == currentCampaignIndex))) {
                        EventReplay::Clear();
                    }
                    state = STATE_PLAYING;
                    BGM::PlayLooping(); BGM::SetVolume9(AtmosphereVolume::Get());
                    EmbeddedBGMVolume::ApplyFromConfig(); // ensure volume is correct now

                    if (g_hasPendingSaveApply && (g_pendingPlayIndex == currentCampaignIndex)) {
                        if (g_sdPending.flat >= 0) { gmap.SetFlat((char)g_sdPending.flat); }
                        SaveSystem::ApplyToGame(g_sdPending, cam, gmap);
                        EventReplay::Replay(gmap, logic);
                        levellog("[SOP_PLAY] applied pending save at playIdx=%d", g_pendingPlayIndex);
                        g_hasPendingSaveApply = false;
                        loadedFromSave = true;
                    }

                    if (haveingamemusic)
                    {
                        if (xmp_load_module_from_memory(ctx, ingamemusic.data, ingamemusic.size)) { std::cout << "music error"; }
                        if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
                        Mix_HookMusic(fill_audio, ctx);
                        Config::SetMusicVol(Config::GetMusicVol());
                    }
                }
                break;
            case Script::SOP_END:
                BGM::Stop(); // stop on SOP_END
                state = STATE_TITLE;
                if (intermissionmusic.data && intermissionmusplaying)
                {
                    Mix_HookMusic(nullptr, nullptr);
                    xmp_end_player(ctx);
                    xmp_release_module(ctx);
                    intermissionmusplaying = false;
                }
                if (titlemusic.data)
                {
                    if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size)) { std::cout << "music error"; }
                    if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
                    Mix_HookMusic(fill_audio, ctx);
                    Config::SetMusicVol(Config::GetMusicVol());
                }
                break;
            }
        }

        if (state == STATE_TITLE)
        {
            SDL_SetPaletteColors(render8->format->palette, smallfont.GetPalette()->colors, 0, 16);
            SDL_SetPaletteColors(render8->format->palette, titlebitmap->format->palette->colors, 17, 256);
            titlescreen.Render(titlebitmap, render8, smallfont);
        }

        while ((state != STATE_SPOOLING) && SDL_PollEvent(&sEvent))
        {
            Input::Update();

            if (sEvent.type == SDL_WINDOWEVENT && sEvent.window.event == SDL_WINDOWEVENT_CLOSE) { notdone = false; }

            if (Input::GetButtonDown(SCE_CTRL_CROSS))
            {
                if (state == STATE_WAITING)
                {
                    state = STATE_PARSING;
                    if (intermissionmusic.data)
                    {
                        Mix_HookMusic(nullptr, nullptr);
                        xmp_end_player(ctx);
                        xmp_release_module(ctx);
                        intermissionmusplaying = false;
                    }
                }
            }

            if (state == STATE_TITLE)
            {
                switch (titlescreen.Update(levelselect))
                {
                case TitleScreen::TITLERET_PLAY:
                    state = STATE_PARSING;
                    BGM::Stop(); // ensure stopped on TITLERET_PLAY
                    logic.Init(&objgraphics);
                    if (titlemusic.data) { Mix_HookMusic(nullptr, nullptr); xmp_end_player(ctx); xmp_release_module(ctx); }
                    break;

                case TitleScreen::TITLERET_RESUME:
                {
                    SaveSystem::SaveData sd;
                    if (SaveSystem::LoadFromDisk(sd)) {
                        int playIdx = GetPlayIndexFromPath(playPaths, sd.levelPath);
                        levellog("[RESUME->SPOOL] path=%s playIdx=%d", sd.levelPath.c_str(), playIdx);
                        if (playIdx >= 0) {
                            g_sdPending            = sd;
                            g_pendingPlayIndex     = playIdx;
                            g_hasPendingSaveApply  = true;

                            currentCampaignIndex = playIdx;
                            levelselect          = playIdx;

                            // ZM fix: spool strictly by play index
                            spoolByPlayIndex = true;
                            spoolTargetIndex = playIdx;
                            spoolPlayCounter = -1;

                            script.Reset();
                            state = STATE_SPOOLING;

                            if (titlemusic.data) {
                                Mix_HookMusic(nullptr, nullptr);
                                xmp_end_player(ctx);
                                xmp_release_module(ctx);
                            }
                        } else {
                            // Fallback: direct load (should be rare)
                            levellog("[RESUME->SPOOL] invalid playIdx -> direct load fallback");
                            if (sd.flat >= 0) { gmap.SetFlat((char)sd.flat); }
                            gmap.Load(sd.levelPath.c_str(), &objgraphics);
                            renderer.Init(render32, &gmap, &objgraphics);
                            logic.InitLevel(&gmap, &cam, &objgraphics);
                            // Clear event replay log on fresh level starts
                            if (!(g_hasPendingSaveApply && (g_pendingPlayIndex == currentCampaignIndex))) {
                                EventReplay::Clear();
                            }
                            SaveSystem::SetCurrentLevelPath(sd.levelPath);
                            currentLevelPath = sd.levelPath;

                            levelselect = GetPlayIndexFromPath(playPaths, sd.levelPath);
                            currentCampaignIndex = levelselect;

                            SaveSystem::ApplyToGame(sd, cam, gmap);
                            EventReplay::Replay(gmap, logic);
                            state = STATE_PLAYING;
                            EmbeddedBGMVolume::ApplyFromConfig();
                            loadedFromSave = true;

                            if (titlemusic.data) {
                                Mix_HookMusic(nullptr, nullptr);
                                xmp_end_player(ctx);
                                xmp_release_module(ctx);
                            }
                        }
                    }
                    break;
                }

                case TitleScreen::TITLERET_SELECT:
                    state = STATE_SPOOLING;
                    logic.Init(&objgraphics);
                    if (titlemusic.data) { Mix_HookMusic(nullptr, nullptr); xmp_end_player(ctx); xmp_release_module(ctx); }
                    break;

                case TitleScreen::TITLERET_QUIT:
                    notdone = false;
                    break;

                default: break;
                }
            }

            if (state == STATE_MENU)
            {
                switch (menuscreen.Update())
                {
                case MenuScreen::MENURET_PLAY:
                    state = STATE_PLAYING;
                    EmbeddedBGMVolume::ApplyFromConfig();
                    break;

                case MenuScreen::MENURET_SAVE:
                    SaveSystem::SavePosition(cam, gmap);
                    state = STATE_PLAYING;
                    EmbeddedBGMVolume::ApplyFromConfig();
                    break;

                case MenuScreen::MENURET_LOAD:
                {
                    SaveSystem::SaveData sd;
                    if (SaveSystem::LoadFromDisk(sd)) {
                        int playIdx = GetPlayIndexFromPath(playPaths, sd.levelPath);
                        levellog("[LOAD->SPOOL] path=%s playIdx=%d", sd.levelPath.c_str(), playIdx);
                        if (playIdx >= 0) {
                            g_sdPending            = sd;
                            g_pendingPlayIndex     = playIdx;
                            g_hasPendingSaveApply  = true;

                            currentCampaignIndex = playIdx;
                            levelselect          = playIdx;

                            // ZM fix: spool strictly by play index
                            spoolByPlayIndex = true;
                            spoolTargetIndex = playIdx;
                            spoolPlayCounter = -1;

                            script.Reset();
                            state = STATE_SPOOLING;
                        } else {
                            // Fallback: direct load
                            levellog("[LOAD->SPOOL] invalid playIdx -> direct load fallback");
                            if (!sd.levelPath.empty()) {
                                if (sd.flat >= 0) { gmap.SetFlat((char)sd.flat); }
                                gmap.Load(sd.levelPath.c_str(), &objgraphics);
                                renderer.Init(render32, &gmap, &objgraphics);
                                logic.InitLevel(&gmap, &cam, &objgraphics);
                                // Clear event replay log on fresh level starts
                                if (!(g_hasPendingSaveApply && (g_pendingPlayIndex == currentCampaignIndex))) {
                                    EventReplay::Clear();
                                }
                                SaveSystem::SetCurrentLevelPath(sd.levelPath);
                                currentLevelPath = sd.levelPath;

                                levelselect = GetPlayIndexFromPath(playPaths, sd.levelPath);
                                currentCampaignIndex = levelselect;
                            }
                            SaveSystem::ApplyToGame(sd, cam, gmap);
                            EventReplay::Replay(gmap, logic);
                            state = STATE_PLAYING;
                            EmbeddedBGMVolume::ApplyFromConfig();
                            loadedFromSave = true;
                        }
                    }
                    break;
                }

                case MenuScreen::MENURET_QUIT:
                    script.Reset();
                    state = STATE_TITLE;
                    BGM::Stop(); // ensure stopped on MENU->TITLE
                    if (titlemusic.data) {
                        if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size)) { std::cout << "music error"; }
                        if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
                        Mix_HookMusic(fill_audio, ctx);
                        Config::SetMusicVol(Config::GetMusicVol());
                    }
                    break;

                default: break;
                }
            }

            if ((state == STATE_PLAYING) && Input::GetButtonDown(SCE_CTRL_START)) { state = STATE_MENU; }
            if ((state == STATE_PLAYING) && Input::GetButtonDown(SCE_CTRL_SELECT)) { Config::SetDebug(!Config::GetDebug()); }

            if (sEvent.type == SDL_USEREVENT)
            {
                if (state == STATE_PLAYING)
                {
                    if (logic.Update(&cam))
                    {
                        BGM::Stop(); // stop Atmosphere BGM on level end
                        if (haveingamemusic)
                        {
                            Mix_HookMusic(nullptr, nullptr);
                            xmp_end_player(ctx);
                            xmp_release_module(ctx);
                            intermissionmusplaying = false;
                        }
                        // Progression:
                        levellog("[LEVEL END] loadedFromSave=%d currentIdx=%d", (int)loadedFromSave, currentCampaignIndex);
                        if (loadedFromSave)
                        {
                            if (playPaths.empty()) { levellog("[LEVEL END] playPaths empty -> normal parse"); loadedFromSave = false; state = STATE_PARSING; }
                            else {
                                // One-time: spool directly to the NEXT level from the loaded index
                                loadedFromSave = false;
                                int nextIndex = currentCampaignIndex + 1;
                                levellog("[LEVEL END] nextIndex=%d", nextIndex);
                                if (nextIndex >= (int)playPaths.size())
                                {
                                    // End of campaign -> back to title
                                    state = STATE_TITLE;
                                    if (titlemusic.data)
                                    {
                                        if (xmp_load_module_from_memory(ctx, titlemusic.data, titlemusic.size)) { std::cout << "music error"; }
                                        if (xmp_start_player(ctx, 44100, 0)) { std::cout << "music error"; }
                                        Mix_HookMusic(fill_audio, ctx);
                                        Config::SetMusicVol(Config::GetMusicVol());
                                    }
                                }
                                else
                                {
                                    currentCampaignIndex = nextIndex;
                                    levelselect = nextIndex;
                                    script.Reset();
                                    state = STATE_SPOOLING;
                                }
                            }
                        }
                        else
                        {
                            // Normal flow: continue parsing to the next intermission
                            levellog("[LEVEL END] normal parse flow");
                            state = STATE_PARSING;
                        }
                    }
                }
                if (state == STATE_TITLE) { titlescreen.Clock(); }
                if (state == STATE_MENU)  { menuscreen.Clock(); }

                fpscounter++;
                if (fpscounter >= 25) { Config::SetFPS(fps); fpscounter = 0; fps = 0; }
            }
        }

        SDL_FillRect(render32, NULL, 0);

        if (state == STATE_PLAYING)
        {
            renderer.SetTeleEffect(logic.GetTeleEffect());
            renderer.SetPlayerHit(logic.GetPlayerHit());
            renderer.SetThermo(logic.GetThermo());
            renderer.Render(&cam);
            MapObject pobj = logic.GetPlayerObj();
            RendererHooks::DeferHudRender(&hud, pobj, &smallfont, renderwidth, renderheight);

            fps++;
        }
        if (state == STATE_MENU)
        {
            renderer.Render(&cam);
            menuscreen.Render(render32, render32, smallfont);
        }

        if ((state == STATE_WAITING) || (state == STATE_TITLE))
        {
            // 320x256 title/intermission -> centered 4:3 in render32
            SDL_BlitSurface(render8, NULL, screen32, NULL);
            SDL_BlitScaled(screen32, NULL, render32, &blitrect);

            // Fill left/right areas with subtle stretched background strips for a soft 16:9 look
            AddSidebarsFrom320(screen32, render32, blitrect);
        }

        if (printscreen)
        {
            std::string filename = "img" + std::to_string(screennum) + ".bmp";
            screennum++;
            SDL_SaveBMP(render32, filename.c_str());
            printscreen = false;
        }

        if (state != STATE_SPOOLING)
        {
            using zgloom_vita::RendererVita2D;
            static bool s_vitaInit = false;
            if (!s_vitaInit) {
                RendererVita2D::Get().Init(renderwidth, renderheight, 960, 544);
                RendererVita2D::Get().SetIntegerScaling(true);
                RendererVita2D::Get().SetClearColor(0xFF000000);
                RendererVita2D::Get().SetTargetFps(50);
                s_vitaInit = true;
            }
            {
                static uint32_t s_last = SDL_GetTicks();
                uint32_t now = SDL_GetTicks();
                uint32_t dt = now - s_last; s_last = now;
                MuzzleFlashFX::Get().ApplyToSurface(render32);
                MuzzleFlashFX::Get().Update((float)dt);
            }
            RendererVita2D::Get().PresentARGB8888WithHook(render32->pixels, render32->pitch, RendererHooks::EffectsDrawOverlaysVita2D_WithHud);
        }
    }

    xmp_free_context(ctx);
    Config::Save();
    SoundHandler::Quit();

    SDL_FreeSurface(render8);
    SDL_FreeSurface(render32);
    SDL_FreeSurface(screen32);
    SDL_FreeSurface(intermissionscreen);
    SDL_FreeSurface(titlebitmap);
#ifndef __vita__
    SDL_DestroyRenderer(ren);
#endif
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
