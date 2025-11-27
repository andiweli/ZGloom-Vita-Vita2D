#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>

// Legacy, compact cheat implementation used by CMake (src/Cheats.cpp is compiled).
// Compatible with the public API declared in cheats/CheatSystem.h.
// Non-invasive extensions: THERMO, BOUNCY, INVIS persistent toggles.
// Note: BOOST levels remain runtime-only (not persisted), per prior project decision.

namespace Cheats {

static bool s_inited = false;
static std::string s_path = "ux0:/data/ZGloom/cheats.txt";

// Core toggles
static bool s_god = false;
static bool s_onehit = false;
static int  s_startWeapon = 0; // 0..5 (0..4 fixed, 5 = DEFAULT)

// Boost levels per weapon (0..4). Not written to disk by Save().
static int  s_boost[5] = {0,0,0,0,0};

// New extras (persisted): THERMO GOGGLES / BOUNCY BULLETS / INVISIBILITY
static bool s_thermo = false;
static bool s_bouncy = false;
static bool s_invis  = false;

// ---- helpers ---------------------------------------------------------------
static inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (unsigned char)s[a] <= 32) ++a;
    while (b > a && (unsigned char)s[b-1] <= 32) --b;
    return s.substr(a, b - a);
}

static void clamp_all() {
    if (s_startWeapon < 0) s_startWeapon = 0;
    if (s_startWeapon > 5) s_startWeapon = 5; // 5 == DEFAULT (no forced weapon)
    for (int i=0;i<5;i++) {
        if (s_boost[i] < 0) s_boost[i] = 0;
        if (s_boost[i] > 4) s_boost[i] = 4;
    }
}

static bool parse_bool(const std::string& v, bool defv=false) {
    std::string t = v; for (auto& c: t) c = (char)toupper((unsigned char)c);
    if (t == "1" || t == "TRUE" || t == "ON" || t == "YES")  return true;
    if (t == "0" || t == "FALSE"|| t == "OFF"|| t == "NO")    return false;
    return defv;
}

static int parse_int(const std::string& v, int defv=0) {
    char *e=nullptr; long x = std::strtol(v.c_str(), &e, 10);
    if (e && *e == 0) return (int)x;
    return defv;
}

static void kv_set(const std::string& k, const std::string& v) {
    std::string key = k; for (auto& c: key) c = (char)toupper((unsigned char)c);
    if (key == "GOD")            s_god = parse_bool(v, false);
    else if (key == "ONEHIT")    s_onehit = parse_bool(v, false);
    else if (key == "STARTWEAPON" || key == "WEAPON") s_startWeapon = parse_int(v, 0);
    // BOOST0..4: we accept reading but do not write them back
    else if (key.rfind("BOOST",0)==0 && key.size()==6) {
        int idx = key[5]-'0'; if (idx>=0 && idx<5) s_boost[idx] = parse_int(v, 0);
    }
    // New extras
    else if (key == "THERMO" || key == "THERMO_GOGGLES") s_thermo = parse_bool(v, false);
    else if (key == "BOUNCY" || key == "BOUNCY_BULLETS") s_bouncy = parse_bool(v, false);
    else if (key == "INVIS"  || key == "INVISIBILITY")   s_invis  = parse_bool(v, false);
}

static void loadKV() {
    FILE* f = std::fopen(s_path.c_str(), "rb");
    if (!f) return;
    char line[512];
    while (std::fgets(line, sizeof(line), f)) {
        std::string s(line);
        // strip comments ; or #
        size_t p = s.find_first_of(";#");
        if (p != std::string::npos) s.erase(p);
        p = s.find_first_of("=:");
        if (p == std::string::npos) continue;
        std::string k = trim(s.substr(0,p));
        std::string v = trim(s.substr(p+1));
        if (!k.empty()) kv_set(k, v);
    }
    std::fclose(f);
    clamp_all();
}

static bool saveKV() {
    // Write only stable keys; keep format simple & comment header.
    FILE* f = std::fopen(s_path.c_str(), "wb");
    if (!f) return false;
    auto put = [&](const char* k, const char* v){
        std::fputs(k, f); std::fputc('=', f); std::fputs(v, f); std::fputc('\n', f);
    };
    std::fputs("; ZGloom Vita Cheats\n", f);
    std::fputs("; NOTE: BOOST* levels are runtime-only and are not persisted.\n", f);
    put("GOD",     s_god     ? "1" : "0");
    put("ONEHIT",  s_onehit  ? "1" : "0");
    {
        char buf[16]; std::snprintf(buf, sizeof(buf), "%d", s_startWeapon);
        put("STARTWEAPON", buf);
    }
    // New extras
    put("THERMO",  s_thermo  ? "1" : "0");
    put("BOUNCY",  s_bouncy  ? "1" : "0");
    put("INVIS",   s_invis   ? "1" : "0");

    std::fclose(f);
    return true;
}

// ---- Public API (compatible) ----------------------------------------------

void Init(const char* path) {
    if (path && *path) s_path = path;
    if (s_inited) return;
    s_inited = true;
    // defaults are already set; try load
    loadKV();
}

void Reload() {
    // Reset to defaults then reload from disk
    s_god = false; s_onehit = false; s_startWeapon = 0;
    for (int i=0;i<5;i++) s_boost[i] = 0;
    s_thermo = s_bouncy = s_invis = false;
    clamp_all();
    loadKV();
}

bool Save() { return saveKV(); }

bool GetGodMode() { return s_god; }
void SetGodMode(bool on) { s_god = on; }

bool GetOneHitKill() { return s_onehit; }
void SetOneHitKill(bool on) { s_onehit = on; }

int  GetStartWeapon() { return s_startWeapon; } // 0..5 (0..4 fixed, 5 = DEFAULT)
void SetStartWeapon(int w) {
    s_startWeapon = std::max(0, std::min(5, w));
}

int  GetBoostLevel(int weaponIndex) {
    if (weaponIndex < 0 || weaponIndex > 4) return 0;
    return s_boost[weaponIndex];
}
void SetBoostLevel(int weaponIndex, int lv) {
    if (weaponIndex < 0 || weaponIndex > 4) return;
    s_boost[weaponIndex] = std::max(0, std::min(4, lv));
}

// New extras (persisted)
bool GetThermoGoggles() { return s_thermo; }
void SetThermoGoggles(bool on) { s_thermo = on; }

bool GetBouncyBullets() { return s_bouncy; }
void SetBouncyBullets(bool on) { s_bouncy = on; }

bool GetInvisibility() { return s_invis; }
void SetInvisibility(bool on) { s_invis = on; }

// Filter incoming damage based on cheats
int FilterDamageToPlayer(int damage) {
    if (s_god) return 0;
    return (damage < 0 ? 0 : damage);
}

// One Hit Kill: multiply outgoing damage
int AmplifyPlayerOutgoingDamage(int base){
    if (s_onehit) {
        long long v = (long long)base * 100LL;
        if (v > 0x7FFFFFFF) v = 0x7FFFFFFF;
        if (v < 1) v = 1;
        return (int)v;
    }
    return base;
}

// Map boost level to reload (lower is faster). Conservative 15% per level.
int BoostLevelToReload(int boostLevel) {
    if (boostLevel < 0) boostLevel = 0;
    if (boostLevel > 4) boostLevel = 4;
    // Using fixed-point arithmetic to avoid libm
    static const int pct[5] = {100, 85, 70, 55, 40}; // percent
    return pct[boostLevel];
}

// Apply boost to a given fallback reload value (integer-based, min 1).
int GetCheatReloadForWeapon(int weaponIndex, int fallbackReload) {
    int lv = GetBoostLevel(weaponIndex);
    int pct = BoostLevelToReload(lv); // 100..40
    long long v = (long long)fallbackReload * pct;
    v /= 100;
    if (v < 1) v = 1;
    if (v > 32767) v = 32767;
    return (int)v;
}

} // namespace Cheats
