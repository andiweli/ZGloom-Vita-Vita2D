#include "menuscreen.h"
#include "config.h"
#include "ConfigOverlays.h"
#include "vita/RendererHooks.h"
#include "cheats/CheatSystem.h"
#include "EmbeddedBGMVolume.h"  // apply ATMOS immediately when changed

// --- Non-selectable label entries for menu headings ---
#ifndef ACTION_LABEL
#define ACTION_LABEL (MenuScreen::MenuEntryAction)99
#endif

// 60% Blend eines einfarbigen Balkens auf ein 32-bit SDL_Surface
static void FillRect75(SDL_Surface* surf, const SDL_Rect& rect, Uint8 rBg, Uint8 gBg, Uint8 bBg)
{
    if (!surf) return;

    SDL_Rect r = rect;

    // einfache Clipping-Sicherung
    if (r.x < 0) { r.w += r.x; r.x = 0; }
    if (r.y < 0) { r.h += r.y; r.y = 0; }
    if (r.x + r.w > surf->w) r.w = surf->w - r.x;
    if (r.y + r.h > surf->h) r.h = surf->h - r.y;
    if (r.w <= 0 || r.h <= 0) return;

    SDL_LockSurface(surf);

    Uint32* pixels = (Uint32*)surf->pixels;
    int pitchPixels = surf->pitch / 4;

    Uint8 rDst, gDst, bDst;
    for (int y = r.y; y < r.y + r.h; ++y) {
        Uint32* row = pixels + y * pitchPixels;
        for (int x = r.x; x < r.x + r.w; ++x) {
            Uint32 src = row[x];
            SDL_GetRGB(src, surf->format, &rDst, &gDst, &bDst);

            // erster Wert Hintergrund, zweiter Wert Transparenz
            rDst = (Uint8)(rDst * 0.35f + rBg * 0.65f);
            gDst = (Uint8)(gDst * 0.35f + gBg * 0.65f);
            bDst = (Uint8)(bDst * 0.35f + bBg * 0.65f);

            row[x] = SDL_MapRGB(surf->format, rDst, gDst, bDst);
        }
    }

    SDL_UnlockSurface(surf);
}

// --- Vignette Warmth mapping kept compatible ---
static int MenuGetVignetteWarmthBool() {
    int w = Config::GetVignetteWarmth();
    if (w >= 0 && w <= 5)      return (w >= 5) ? 1 : 0;
    if (w >= 0 && w <= 10)     return (w >= 6) ? 1 : 0;
    if (w == 0 || w == 1)      return w;
    if (w >= -100 && w <= 100) return (w >= 20) ? 1 : 0;
    return (w > 0) ? 1 : 0;
}
static void MenuSetVignetteWarmthBool(int on) {
    Config::SetVignetteWarmth(on ? 5 : 0);
    Config::Save();
}

// --- Helper: readable names for START WEAPON cheat (index 0..5, 5 = DEFAULT) ---
static const char* MenuGetWeaponNameFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return "PLASMA CANNON";
    case 1: return "BLASTER UPGRADE";
    case 2: return "BOOSTER UPGRADE";
    case 3: return "ION UPGRADE";
    case 4: return "PHOTON UPGRADE";
    case 5: return "GAME DEFAULT";
    default: return "UNKNOWN";
    }
}

// --- Cheats wrappers (logic stays 0..4 internally) ---
static int MenuGetCheatGodMode() { return Cheats::GetGodMode() ? 1 : 0; }
static void MenuSetCheatGodMode(int on) { Cheats::SetGodMode(on != 0); Cheats::Save(); }

static int MenuGetCheatOneHitKill() { return Cheats::GetOneHitKill() ? 1 : 0; }
static void MenuSetCheatOneHitKill(int on) { Cheats::SetOneHitKill(on != 0); Cheats::Save(); }

static int MenuGetCheatStartWeapon() { return Cheats::GetStartWeapon(); } // 0..5 (5 = DEFAULT)
static void MenuSetCheatStartWeapon(int w) { Cheats::SetStartWeapon(w); Cheats::Save(); }
static int MenuGetBoostW1() { return Cheats::GetBoostLevel(1); }
static void MenuSetBoostW1(int v) { Cheats::SetBoostLevel(1, v); Cheats::Save(); }
static int MenuGetBoostW3() { return Cheats::GetBoostLevel(3); }
static void MenuSetBoostW3(int v) { Cheats::SetBoostLevel(3, v); Cheats::Save(); }

// --- New Cheats wrappers for extras ---
static int MenuGetCheatThermo() { return Cheats::GetThermoGoggles() ? 1 : 0; }
static void MenuSetCheatThermo(int on) { Cheats::SetThermoGoggles(on != 0); Cheats::Save(); }

static int MenuGetCheatBouncy() { return Cheats::GetBouncyBullets() ? 1 : 0; }
static void MenuSetCheatBouncy(int on) { Cheats::SetBouncyBullets(on != 0); Cheats::Save(); }

static int MenuGetCheatInvis() { return Cheats::GetInvisibility() ? 1 : 0; }
static void MenuSetCheatInvis(int on) { Cheats::SetInvisibility(on != 0); Cheats::Save(); }

// --- ATMOS wrapper: config set + immediate apply to mixer ---
static int  MenuGetAtmosVolume() { return Config::GetAtmosVolume(); }
static void MenuSetAtmosVolume_Apply(int v) {
    Config::SetAtmosVolume(v);
    EmbeddedBGMVolume::ApplyFromConfig();
    Config::Save();
}

// --- Display Profiles: apply multiple options with one click ---
static void MenuApplyProfileClassic(int /*unused*/) {
    // 50 FPS, almost no post effects
    Config::SetMaxFpsBool(1);          // 50 FPS
    Config::SetBilinearFilter(0);
    Config::SetMuzzleFlash(0);
    Config::SetBlobShadows(0);

    Config::SetVignetteEnabled(0);
    Config::SetVignetteStrength(0);
    Config::SetVignetteRadius(0);
    Config::SetVignetteSoftness(0);
    Config::SetVignetteWarmth(0);

    Config::SetFilmGrain(0);
    Config::SetFilmGrainIntensity(0);
    Config::SetScanlines(0);
    Config::SetScanlineIntensity(0);

    Config::EffectsConfigSave();
}

static void MenuApplyProfileBalanced(int /*unused*/) {
    // 50 FPS with moderate eye-candy
    Config::SetMaxFpsBool(0);          // 50 FPS
    Config::SetBilinearFilter(1);
    Config::SetMuzzleFlash(1);
    Config::SetBlobShadows(1);

    Config::SetVignetteEnabled(1);
    Config::SetVignetteStrength(1);
    Config::SetVignetteRadius(5);
    Config::SetVignetteSoftness(5);
    Config::SetVignetteWarmth(0);

    Config::SetFilmGrain(1);
    Config::SetFilmGrainIntensity(2);
    Config::SetScanlines(0);
    Config::SetScanlineIntensity(0);

    Config::EffectsConfigSave();
}

static void MenuApplyProfileExtreme(int /*unused*/) {
    // 30 FPS and all effects ON
    Config::SetMaxFpsBool(0);          // 30 FPS
    Config::SetBilinearFilter(1);
    Config::SetMuzzleFlash(1);
    Config::SetBlobShadows(1);

    Config::SetVignetteEnabled(1);
    Config::SetVignetteStrength(4);
    Config::SetVignetteRadius(3);
    Config::SetVignetteSoftness(3);
    Config::SetVignetteWarmth(5);

    Config::SetFilmGrain(1);
    Config::SetFilmGrainIntensity(3);
    Config::SetScanlines(1);
    Config::SetScanlineIntensity(0);

    Config::EffectsConfigSave();
}


void MenuScreen::Render(SDL_Surface *src, SDL_Surface *dest, Font &font)
{
    SDL_BlitSurface(src, nullptr, dest, nullptr);
    bool flash = (timer / 5) & 1;

    int scale = dest->h / 256;
    if (scale < 1)
        scale = 1;

    if (status == MENUSTATUS_MAIN)
    {
        DisplayStandardMenu(mainmenu, flash, scale, dest, font);
    }
    else if (status == MENUSTATUS_SOUNDOPTIONS)
    {
        DisplayStandardMenu(soundmenu, flash, scale, dest, font);
    }
    else if (status == MENUSTATUS_CONTROLOPTIONS)
    {
        DisplayStandardMenu(controlmenu, flash, scale, dest, font);
    }
    else if (status == MENUSTATUS_DISPLAYOPTIONS)
    {
        DisplayStandardMenu(displaymenu, flash, scale, dest, font);
    }
    else if (status == MENUSTATUS_CHEATOPTIONS)
    {
        DisplayStandardMenu(cheatmenu, flash, scale, dest, font);
    }
}

MenuScreen::MenuScreen()
{
    status = MENUSTATUS_MAIN;
    selection = 0;
    timer = 0;

    Cheats::Init("ux0:/data/ZGloom/cheats.txt");

    mainmenu.push_back(MenuEntry("CONTINUE", ACTION_RETURN, MENURET_PLAY, nullptr, nullptr));
    mainmenu.push_back(MenuEntry("SAVE POSITION", ACTION_RETURN, MENURET_SAVE, nullptr, nullptr));
    //HALBEZEILE//
    mainmenu.push_back(MenuEntry("GRAPHICS PROFILES", ACTION_LABEL, 0, nullptr, nullptr));
    //HALBEZEILE//
    mainmenu.push_back(MenuEntry("CLASSIC", ACTION_PROFILE, 0, nullptr, MenuApplyProfileClassic));
    mainmenu.push_back(MenuEntry("BALANCED", ACTION_PROFILE, 0, nullptr, MenuApplyProfileBalanced));
    mainmenu.push_back(MenuEntry("EXTREME", ACTION_PROFILE, 0, nullptr, MenuApplyProfileExtreme));

    mainmenu.push_back(MenuEntry("GAME OPTIONS", ACTION_LABEL, 0, nullptr, nullptr));
    //HALBEZEILE//
    mainmenu.push_back(MenuEntry("CONTROL OPTIONS", ACTION_SWITCHMENU, MENUSTATUS_CONTROLOPTIONS, nullptr, nullptr));
    mainmenu.push_back(MenuEntry("SOUND AND FX OPTIONS", ACTION_SWITCHMENU, MENUSTATUS_SOUNDOPTIONS, nullptr, nullptr));
    mainmenu.push_back(MenuEntry("DISPLAY AND GRAPHICS OPTIONS", ACTION_SWITCHMENU, MENUSTATUS_DISPLAYOPTIONS, nullptr, nullptr));
    mainmenu.push_back(MenuEntry("CHEAT OPTIONS", ACTION_SWITCHMENU, MENUSTATUS_CHEATOPTIONS, nullptr, nullptr));
    //HALBEZEILE//
    mainmenu.push_back(MenuEntry("QUIT TO TITLE", ACTION_RETURN, MENURET_QUIT, nullptr, nullptr));

    soundmenu.push_back(MenuEntry("SOUND AND FX OPTIONS", ACTION_LABEL, 0, nullptr, nullptr));
    soundmenu.push_back(MenuEntry("RETURN", ACTION_SWITCHMENU, MENUSTATUS_MAIN, nullptr, nullptr));
    //HALBEZEILE//
    soundmenu.push_back(MenuEntry("SOUND FX VOLUME: ", ACTION_INT, 10, Config::GetSFXVol, Config::SetSFXVol));
    soundmenu.push_back(MenuEntry("MUSIC VOLUME: ", ACTION_INT, 10, Config::GetMusicVol, Config::SetMusicVol));
    soundmenu.push_back(MenuEntry("AMBIENCE VOLUME: ", ACTION_INT, 10, MenuGetAtmosVolume, MenuSetAtmosVolume_Apply));

    controlmenu.push_back(MenuEntry("CONTROL OPTIONS", ACTION_LABEL, 0, nullptr, nullptr));
    controlmenu.push_back(MenuEntry("RETURN", ACTION_SWITCHMENU, MENUSTATUS_MAIN, nullptr, nullptr));
    //HALBEZEILE//
    controlmenu.push_back(MenuEntry("RAPIDFIRE: ", ACTION_BOOL, 0, Config::GetAutoFire, Config::SetAutoFire));
    controlmenu.push_back(MenuEntry("RIGHT STICK SENSITIVITY: ", ACTION_INT, 10, Config::GetMouseSens, Config::SetMouseSens));

    displaymenu.push_back(MenuEntry("DISPLAY AND GRAPHICS OPTIONS", ACTION_LABEL, 0, nullptr, nullptr));
    displaymenu.push_back(MenuEntry("RETURN", ACTION_SWITCHMENU, MENUSTATUS_MAIN, nullptr, nullptr));
    //HALBEZEILE//
    displaymenu.push_back(MenuEntry("MAX. FPS 50: ", ACTION_BOOL, 0, Config::GetMaxFpsBool, Config::SetMaxFpsBool));
    displaymenu.push_back(MenuEntry("BILINEAR FILTER: ", ACTION_BOOL, 0, Config::GetBilinearFilter, Config::SetBilinearFilter));
    //HALBEZEILE//
    displaymenu.push_back(MenuEntry("BLOOD INTENSITY: ", ACTION_INT, 6, Config::GetBlood, Config::SetBlood));
    displaymenu.push_back(MenuEntry("MUZZLE FLASH AND REFLECTION: ", ACTION_BOOL, 0, Config::GetMuzzleFlash, Config::SetMuzzleFlash));
    displaymenu.push_back(MenuEntry("BLOB SHADOWS: ", ACTION_BOOL, 0, Config::GetBlobShadows, Config::SetBlobShadows));
    //HALBEZEILE//
    displaymenu.push_back(MenuEntry("ATMOSPHERIC VIGNETTE: ", ACTION_BOOL, 0, Config::GetVignetteEnabled, Config::SetVignetteEnabled));
    displaymenu.push_back(MenuEntry("VIGNETTE STRENGTH: ", ACTION_INT, 6, Config::GetVignetteStrength, Config::SetVignetteStrength));
    displaymenu.push_back(MenuEntry("VIGNETTE RADIUS: ", ACTION_INT, 6, Config::GetVignetteRadius, Config::SetVignetteRadius));
    displaymenu.push_back(MenuEntry("VIGNETTE SOFTNESS: ", ACTION_INT, 6, Config::GetVignetteSoftness, Config::SetVignetteSoftness));
    displaymenu.push_back(MenuEntry("VIGNETTE WARMTH: ", ACTION_BOOL, 0, MenuGetVignetteWarmthBool, MenuSetVignetteWarmthBool));
    //HALBEZEILE//
    displaymenu.push_back(MenuEntry("FILM GRAIN: ", ACTION_BOOL, 0, Config::GetFilmGrain, Config::SetFilmGrain));
    displaymenu.push_back(MenuEntry("FILM GRAIN INTENSITY: ", ACTION_INT, 6, Config::GetFilmGrainIntensity, Config::SetFilmGrainIntensity));
    //HALBEZEILE//
    displaymenu.push_back(MenuEntry("SCANLINES: ", ACTION_BOOL, 0, Config::GetScanlines, Config::SetScanlines));
    displaymenu.push_back(MenuEntry("SCANLINE INTENSITY: ", ACTION_INT, 6, Config::GetScanlineIntensity, Config::SetScanlineIntensity));

    // --- CHEAT MENU ----------------------------------------------------------
    cheatmenu.push_back(MenuEntry("CHEAT OPTIONS", ACTION_LABEL, 0, nullptr, nullptr));
    cheatmenu.push_back(MenuEntry("RETURN", ACTION_SWITCHMENU, MENUSTATUS_MAIN, nullptr, nullptr));

    cheatmenu.push_back(MenuEntry("GOD MODE: ", ACTION_BOOL, 0, MenuGetCheatGodMode, MenuSetCheatGodMode));
    cheatmenu.push_back(MenuEntry("ONE HIT KILL: ", ACTION_BOOL, 0, MenuGetCheatOneHitKill, MenuSetCheatOneHitKill));
    cheatmenu.push_back(MenuEntry("BOUNCY BULLETS: ", ACTION_BOOL, 0, MenuGetCheatBouncy, MenuSetCheatBouncy));
    cheatmenu.push_back(MenuEntry("THERMO GOGGLES: ", ACTION_BOOL, 0, MenuGetCheatThermo, MenuSetCheatThermo));
    cheatmenu.push_back(MenuEntry("INVISIBILITY: ", ACTION_BOOL, 0, MenuGetCheatInvis, MenuSetCheatInvis));
    cheatmenu.push_back(MenuEntry("WEAPON: ", ACTION_INT, 6, MenuGetCheatStartWeapon, MenuSetCheatStartWeapon));
}

void MenuScreen::HandleKeyMenu()
{
    int button = 0;

    if (Input::GetButtonDown(SCE_CTRL_UP))
        button = SCE_CTRL_UP;
    if (Input::GetButtonDown(SCE_CTRL_DOWN))
        button = SCE_CTRL_DOWN;
    if (Input::GetButtonDown(SCE_CTRL_LEFT))
        button = SCE_CTRL_LEFT;
    if (Input::GetButtonDown(SCE_CTRL_RIGHT))
        button = SCE_CTRL_RIGHT;

    if (Input::GetButtonDown(SCE_CTRL_CROSS))
        button = SCE_CTRL_CROSS;
    if (Input::GetButtonDown(SCE_CTRL_SQUARE))
        button = SCE_CTRL_SQUARE;
    if (Input::GetButtonDown(SCE_CTRL_TRIANGLE))
        button = SCE_CTRL_TRIANGLE;
    if (Input::GetButtonDown(SCE_CTRL_CIRCLE))
        button = SCE_CTRL_CIRCLE;

    if (Input::GetButtonDown(SCE_CTRL_RTRIGGER))
        button = SCE_CTRL_RTRIGGER;
    if (Input::GetButtonDown(SCE_CTRL_LTRIGGER))
        button = SCE_CTRL_LTRIGGER;

    Config::SetKey((Config::keyenum)selection, button);

    if (button != 0)
    {
        selection++;
    }

    if (selection == Config::KEY_END)
    {
        selection = 0;
        status = MENUSTATUS_MAIN;
    }
}

MenuScreen::MenuReturn MenuScreen::HandleStandardMenu(std::vector<MenuEntry> &menu)
{

/* removed: global label-skip to allow UP to wrap properly */

    if (Input::GetButtonDown(SCE_CTRL_UP))
    {
        int guard = 0;
        do {
            selection--;
            if (selection < 0)
                selection = (int)menu.size() - 1;
            guard++;
        } while (!menu.empty() && menu[selection].action == ACTION_LABEL && guard < (int)menu.size());
    }
    else if (Input::GetButtonDown(SCE_CTRL_DOWN))
    {
        int guard = 0;
        do {
            selection++;
            if (selection == (int)menu.size())
                selection = 0;
            guard++;
        } while (!menu.empty() && menu[selection].action == ACTION_LABEL && guard < (int)menu.size());
    }
    else if (Input::GetButtonDown(SCE_CTRL_CROSS))
    {
        switch (menu[selection].action)
        {
        case ACTION_BOOL:
        {
            menu[selection].setval(!menu[selection].getval());
            break;
        }
        case ACTION_INT:
        {
            int x = (menu[selection].getval() + 1);
            if (x >= menu[selection].arg)
                x = 0;
            menu[selection].setval(x);
            break;
        }
        case ACTION_SWITCHMENU:
        {
            status = (MENUSTATUS)menu[selection].arg;
            selection = 1; // focus first clickable entry (after label) so it blinks
            break;
        }
        case ACTION_RETURN:
        {
            return (MenuReturn)menu[selection].arg;
            break;
        }
        case ACTION_PROFILE:
        {
            if (menu[selection].setval)
                menu[selection].setval(menu[selection].arg);
            break;
        }
        default:
            break;
        }
    }
    else if (Input::GetButtonDown(SCE_CTRL_SQUARE))
    {
        if (menu[selection].action == ACTION_BOOL)
        {
            menu[selection].setval(!menu[selection].getval());
        }
        else if (menu[selection].action == ACTION_INT)
        {
            int x = menu[selection].getval() - 1;
            if (x < 0)
                x = menu[selection].arg - 1;
            menu[selection].setval(x);
        }
    }
    else if (Input::GetButtonDown(SCE_CTRL_CIRCLE))
    {
        if (status != MENUSTATUS_MAIN)
        {
            status = MENUSTATUS_MAIN;
            selection = 0;
        }
    }

    return MENURET_NOTHING;
}

MenuScreen::MenuReturn MenuScreen::Update()
{

    switch (status)
    {
    case MENUSTATUS_MAIN:
    {
        return HandleStandardMenu(mainmenu);
        break;
    }

    case MENUSTATUS_SOUNDOPTIONS:
    {
        HandleStandardMenu(soundmenu);
        break;
    }

    case MENUSTATUS_CONTROLOPTIONS:
    {
        HandleStandardMenu(controlmenu);
        break;
    }

    case MENUSTATUS_DISPLAYOPTIONS:
    {
        HandleStandardMenu(displaymenu);
        break;
    }

    case MENUSTATUS_CHEATOPTIONS:
    {
        HandleStandardMenu(cheatmenu);
        break;
    }

    default:
        break;
    }

    return MENURET_NOTHING;
}

void MenuScreen::DisplayStandardMenu(std::vector<MenuEntry> &menu, bool flash, int scale, SDL_Surface *dest, Font &font)
{
    int starty = 40 * scale;
    int yinc = 10 * scale;

    for (size_t i = 0; i < menu.size(); i++)
    {

// Non-selectable labels (headings)
if (menu[i].action == ACTION_LABEL) {
    int yinc = 10 * scale;

    // halbe Zeile Abstand nach oben
    starty += (yinc / 2);

    int labelY = starty;

    // Hintergrund-Balken: 1/4 Zeile über, 1/4 Zeile unter dem Label
    SDL_Rect bg;

    // 70 % der Bildschirmbreite
    bg.w = dest->w * 7 / 10;         // 0.7 * width (70%)

    // zentrieren
    bg.x = (dest->w - bg.w) / 2;

    // etwas höher: 1/4 + 1/8 Zeile über labelY = 3/8
    bg.y = labelY - (yinc * 3) / 8;
    bg.h = (yinc * 3) / 2;   // bleibt 1,5 Zeilen

    bg.h = (yinc * 3) / 2;             // 1,5 Zeilen insgesamt

    // 75% transparenter Hintergrund (siehe Helper unten)
    FillRect75(dest, bg, 65, 65, 0);

    // Überschrift zeichnen
    font.PrintMessage(menu[i].name, labelY, dest, scale);

    if (menu[i].name == std::string("GRAPHICS PROFILES") ||
        menu[i].name == std::string("GAME OPTIONS")) {
        // PROFILES: unten 1,5 Zeilen Abstand
        starty += (yinc * 3) / 2;
    } else {
    starty += yinc * ((i == 0) ? 2 : 1);

    }

    continue;
}

        if (menu[i].name == std::string("QUIT TO TITLE") ||
            menu[i].name == std::string("RAPIDFIRE: ") ||
            menu[i].name == std::string("SOUND FX VOLUME: ") ||
            menu[i].name == std::string("MAX. FPS 50: ") ||
            menu[i].name == std::string("BLOOD INTENSITY: ") ||
            menu[i].name == std::string("ATMOSPHERIC VIGNETTE: ") ||
            menu[i].name == std::string("FILM GRAIN: ") ||
            menu[i].name == std::string("SCANLINES: ") ||
            menu[i].name == std::string("GOD MODE: "))
        {
            int yinc = 10 * scale;
            starty += (yinc / 2);
        }

        if (menu[i].action == ACTION_INT)
        {
            if (flash || (selection != i))
            {
                std::string menustring = menu[i].name;
                int v = menu[i].getval();
                if (menu[i].arg == 1)
                {
                    menustring += (v ? "ON" : "OFF");
                }
                else
                {
                    // Custom text for START WEAPON cheat; other ints stay numeric
                    if (menustring.rfind("WEAPON", 0) == 0) {
                        menustring += MenuGetWeaponNameFromIndex(v);
                    } else {
                        menustring += std::to_string(v);
                    }
                }
                font.PrintMessage(menustring, starty, dest, scale);
            }
        }
        else if (menu[i].action == ACTION_BOOL)
        {
            if (flash || (selection != i))
            {
                std::string menustring = menu[i].name;
                if (menustring.rfind("VIGNETTE WARMTH", 0) == 0)
                    menustring += (menu[i].getval() ? "WARM" : "COLD");
                else
                    menustring += (menu[i].getval() ? "ON" : "OFF");
                font.PrintMessage(menustring, starty, dest, scale);
            }
        }
else
        {
            if (flash || (selection != i))
                font.PrintMessage(menu[i].name, starty, dest, scale);
        }

        // Normales Zeilen-Spacing
        starty += yinc * ((i == 0) ? 2 : 1);

        // EXTRA: halbe Zeile Abstand nach "SAVE POSITION"
        if (menu[i].name == std::string("SAVE POSITION")) {
            starty += yinc / 2;
        }
    }
}
