#include "SaveSystem.h"
#include "objectgraphics.h"
#include "config.h"
#include "renderer.h"
#include "gloommap.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace SaveSystem {

static std::string s_currentLevelPath;
static int s_currentFlat = -1;

void SetCurrentLevelPath(const std::string& levelPath) {
    s_currentLevelPath = levelPath;
}
void SetCurrentFlat(int flat) {
    if (flat < 0) flat = 0;
    if (flat > 9) flat = 9;
    s_currentFlat = flat;
}
int GetCurrentFlat() { return s_currentFlat; }

static std::string GetGameKey() {
    int id = Config::GetGameID();
    switch (id) {
        case 0: return "gloom";
        case 1: return "deluxe";
        case 2: return "gloom3";
        case 3: return "massacre";
        default: return "gloom";
    }
}

static void EnsureDir(const std::string& path) {
    ::mkdir(path.c_str(), 0777);
}

static std::string GetSaveDir() {
    std::string dir = std::string("ux0:/data/ZGloom/saves/") + GetGameKey();
    EnsureDir("ux0:/data/ZGloom");
    EnsureDir("ux0:/data/ZGloom/saves");
    EnsureDir(dir);
    return dir;
}
static std::string GetSavePath() {
    return GetSaveDir() + "/last.sav";
}

bool HasSaveForCurrentGame() {
    std::FILE* f = std::fopen(GetSavePath().c_str(), "rb");
    if (!f) return false;
    std::fclose(f);
    return true;
}

static inline int clampi(int v, int lo, int hi) { if (v<lo) return lo; if (v>hi) return hi; return v; }

static bool Capture(const Camera& cam, const GloomMap& gmap, SaveData& out) {
    out.levelPath = s_currentLevelPath;
    // Quick::GetInt is non-const; read via const_cast
    out.x   = const_cast<Quick&>(cam.x).GetInt();
    out.z   = const_cast<Quick&>(cam.z).GetInt();
    out.rot = const_cast<Quick&>(cam.rotquick).GetInt() & 0xFF;

    // Defaults
    out.hp = 100;
    out.weapon = 0;
    out.reload = 0;
    out.reloadcnt = 0;

    // Best-effort extraction of player entity state:
    for (auto const& o : const_cast<GloomMap&>(gmap).GetMapObjects()) {
        if (o.t == ObjectGraphics::OLT_PLAYER1) {
            out.hp = o.data.ms.hitpoints;
            out.weapon = o.data.ms.weapon;
            out.reload = o.data.ms.reload;
            out.reloadcnt = o.data.ms.reloadcnt;
            break;
        }
    }

    // Flat/roof set from script hook
    out.flat = s_currentFlat;

    return !out.levelPath.empty();
}

bool SavePosition(const Camera& cam, const GloomMap& gmap) {
    SaveData d;
    if (!Capture(cam, gmap, d)) return false;
    std::string path = GetSavePath();
    std::ofstream ofs(path.c_str(), std::ios::binary | std::ios::trunc);
    if (!ofs) return false;
    ofs << "level="     << d.levelPath << "\n";
    ofs << "x="         << d.x << "\n";
    ofs << "z="         << d.z << "\n";
    ofs << "rot="       << d.rot << "\n";
    ofs << "hp="        << d.hp << "\n";
    ofs << "weapon="    << d.weapon << "\n";
    ofs << "reload="    << d.reload << "\n";
    ofs << "reloadcnt=" << d.reloadcnt << "\n";
    ofs << "flat="      << d.flat << "\n";
    ofs.close();
    return true;
}

static bool ParseLine(const std::string& line, std::string& key, std::string& val) {
    auto pos = line.find('=');
    if (pos == std::string::npos) return false;
    key = line.substr(0, pos);
    val = line.substr(pos + 1);
    return true;
}

bool LoadFromDisk(SaveData& out) {
    std::ifstream ifs(GetSavePath().c_str(), std::ios::binary);
    if (!ifs) return false;
    std::string line, k, v;
    out = SaveData{};
    while (std::getline(ifs, line)) {
        if (!ParseLine(line, k, v)) continue;
        if (k == "level") out.levelPath = v;
        else if (k == "x") out.x = std::atoi(v.c_str());
        else if (k == "z") out.z = std::atoi(v.c_str());
        else if (k == "rot") out.rot = std::atoi(v.c_str()) & 0xFF;
        else if (k == "hp") out.hp = std::atoi(v.c_str());
        else if (k == "weapon") out.weapon = std::atoi(v.c_str());
        else if (k == "reload") out.reload = std::atoi(v.c_str());
        else if (k == "reloadcnt") out.reloadcnt = std::atoi(v.c_str());
        else if (k == "flat") out.flat = std::atoi(v.c_str());
    }
    // Propagate flat value into the session hook so subsequent saves inherit it
    if (out.flat >= 0) s_currentFlat = out.flat;
    return !out.levelPath.empty();
}

bool ApplyToGame(const SaveData& d, Camera& cam, GloomMap& gmap) {
    // If we have a flat value, ensure ceiling/floor textures are loaded.
    if (d.flat >= 0) { gmap.SetFlat((char)d.flat); }

    // re-position the camera/player
    cam.x.SetInt(d.x);
    cam.z.SetInt(d.z);
    cam.rotquick.SetInt(d.rot & 0xFF);

    // Update the actual player object as well.
    for (auto & o : gmap.GetMapObjects()) {
        if (o.t == ObjectGraphics::OLT_PLAYER1) {
            o.x.SetInt(d.x);
            o.y.SetInt(0);
            o.z.SetInt(d.z);
            o.data.ms.rotquick.SetInt(d.rot & 0xFF);
            o.data.ms.hitpoints = clampi(d.hp, 1, 32767);
            o.data.ms.weapon = clampi(d.weapon, 0, 4);
            o.data.ms.reload = clampi(d.reload, 0, 5);
            o.data.ms.reloadcnt = clampi(d.reloadcnt, 0, o.data.ms.reload * 2);
            break;
        }
    }
    return true;
}

} // namespace SaveSystem
