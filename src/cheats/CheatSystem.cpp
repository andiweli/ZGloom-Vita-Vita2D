#include "CheatSystem.h"
#include <cstdio>
#include <cstring>
#include <algorithm>

namespace Cheats {

static State g_state;
static std::string g_path = "ux0:/data/ZGloom/cheats.txt";

static inline std::string trim(const std::string& s) {
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    while (b > a && (s[b-1] == ' ' || s[b-1] == '\t' || s[b-1] == '\r' || s[b-1] == '\n')) --b;
    return s.substr(a, b - a);
}

static inline std::string upper(std::string s) {
    for (auto& c : s) c = (char)toupper((unsigned char)c);
    return s;
}

static void clamp_all() {
    if (g_state.start_weapon < 0) g_state.start_weapon = 0;
    if (g_state.start_weapon > 4) g_state.start_weapon = 4;
    for (int i=0;i<5;i++) {
        if (g_state.boost_level[i] < 0) g_state.boost_level[i] = 0;
        if (g_state.boost_level[i] > 4) g_state.boost_level[i] = 4;
    }
}

static bool parse_bool(const std::string& v, bool defv) {
    auto u = upper(trim(v));
    if (u == "1" || u == "ON" || u == "TRUE" || u == "YES") return true;
    if (u == "0" || u == "OFF"|| u == "FALSE"|| u == "NO")  return false;
    return defv;
}

static int parse_num_compat_0_to_4(const std::string& v) {
    // Accept 0..4 and also 1..5 values (compat); values >4 treated as (n-1)
    int n = std::atoi(v.c_str());
    if (n >= 0 && n <= 4) return n;
    if (n >= 1 && n <= 5) return n - 1;
    if (n < 0) return 0;
    return 4;
}

void Init(const char* path) {
    if (path && *path) g_path = path;
    Reload();
}

void Reload() {
    // defaults
    g_state = State{};
    clamp_all();

    FILE* f = std::fopen(g_path.c_str(), "rb");
    if (!f) return; // keep defaults

    char line[512];
    while (std::fgets(line, sizeof(line), f)) {
        std::string s(line);
        // strip comments (';' or '#')
        size_t semi = s.find(';');
        size_t hash = s.find('#');
        size_t cut = std::min(semi == std::string::npos ? s.size() : semi,
                              hash == std::string::npos ? s.size() : hash);
        s = s.substr(0, cut);
        s = trim(s);
        if (s.empty()) continue;

        size_t eq = s.find('=');
        if (eq == std::string::npos) continue;
        std::string key = upper(trim(s.substr(0, eq)));
        std::string val = trim(s.substr(eq + 1));

        if (key == "GODMODE") {
            g_state.godmode = parse_bool(val, g_state.godmode);
        } else if (key == "ONE_HIT_KILL" || key == "ONEHITKILL") {
            g_state.one_hit_kill = parse_bool(val, g_state.one_hit_kill);
        } else if (key == "START_WEAPON" || key == "STARTWEAPON") {
            g_state.start_weapon = parse_num_compat_0_to_4(val);
        } else if (key == "BOOST_LEVEL" || key == "BOOSTLEVEL") {
            int v_internal = parse_num_compat_0_to_4(val);
            for (int i=0;i<5;i++) g_state.boost_level[i] = v_internal;
        } else if (key.rfind("BOOST_LEVEL", 0) == 0) {
            std::string suffix = key.substr(std::string("BOOST_LEVEL").size());
            if (!suffix.empty() && suffix[0] == '_') suffix.erase(0,1);
            int idx = std::atoi(suffix.c_str());
            if (idx >=0 && idx < 5) {
                g_state.boost_level[idx] = parse_num_compat_0_to_4(val);
            }
        }
    }
    std::fclose(f);
    clamp_all();
}

bool Save() {
    clamp_all();
    FILE* f = std::fopen(g_path.c_str(), "wb");
    if (!f) return false;
    std::fprintf(f, "GODMODE=%d\n", g_state.godmode ? 1 : 0);
    std::fprintf(f, "ONE_HIT_KILL=%d\n", g_state.one_hit_kill ? 1 : 0);
    std::fprintf(f, "START_WEAPON=%d\n", g_state.start_weapon); // persist 0..4
    for (int i=0;i<5;i++) std::fprintf(f, "BOOST_LEVEL%d=%d\n", i, g_state.boost_level[i]); // persist 0..4
    std::fclose(f);
    return true;
}

State Get() { return g_state; }

bool GetGodMode() { return g_state.godmode; }
void SetGodMode(bool v) { g_state.godmode = v; }

bool GetOneHitKill() { return g_state.one_hit_kill; }
void SetOneHitKill(bool v) { g_state.one_hit_kill = v; }

int  GetStartWeapon() { return g_state.start_weapon; }
void SetStartWeapon(int w) {
    if (w < 0) w = 0;
    if (w > 4) w = 4;
    g_state.start_weapon = w;
}

int  GetBoostLevel(int weaponIndex) {
    if (weaponIndex < 0 || weaponIndex > 4) return 0;
    return g_state.boost_level[weaponIndex];
}
void SetBoostLevel(int weaponIndex, int lv) {
    if (weaponIndex < 0 || weaponIndex > 4) return;
    if (lv < 0) lv = 0;
    if (lv > 4) lv = 4;
    g_state.boost_level[weaponIndex] = lv;
}

int FilterDamageToPlayer(int damage) {
    if (g_state.godmode) return 0;
    return damage;
}

int AmplifyPlayerOutgoingDamage(int baseDamage) {
    if (g_state.one_hit_kill) {
        long long d = (long long)baseDamage * 100LL;
        if (d > 32767) d = 32767;
        if (d < 1) d = 1;
        return (int)d;
    }
    return baseDamage;
}

int BoostLevelToReload(int boostLevel) {
    if (boostLevel < 0) boostLevel = 0;
    if (boostLevel > 4) boostLevel = 4;
    static const int map[5] = {5,4,3,2,1};
    return map[boostLevel];
}

int GetCheatReloadForWeapon(int weaponIndex, int fallbackReload) {
    if (weaponIndex < 0 || weaponIndex > 4) return fallbackReload;
    int lv = GetBoostLevel(weaponIndex);
    return BoostLevelToReload(lv);
}

} // namespace Cheats
