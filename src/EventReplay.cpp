#include "EventReplay.h"
#include "config.h"
#include "gloommap.h"
#include "gamelogic.h"

#include <vector>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <algorithm>

#if defined(_WIN32)
#include <direct.h>
#define mkdir(path,mode) _mkdir(path)
#else
#include <sys/stat.h>
#endif

namespace {

static void EnsureDir(const std::string& path) {
    ::mkdir(path.c_str(), 0777);
}

static std::string GameKey() {
    switch (Config::GetGameID()) {
        case 0: return "gloom";
        case 1: return "deluxe";
        case 2: return "gloom3";
        case 3: return "massacre";
        default: return "gloom";
    }
}

static std::string EventsPath() {
    std::string base = "ux0:/data/ZGloom";
    EnsureDir(base);
    EnsureDir(base + "/saves");
    std::string dir = base + "/saves/" + GameKey();
    EnsureDir(dir);
    return dir + "/last.events";
}

static bool s_loaded = false;
static bool s_seen[32] = {false};
static std::vector<int> s_events;

static void LoadOnce() {
    if (s_loaded) return;
    s_loaded = true;
    std::ifstream in(EventsPath());
    if (!in) return;
    std::string line;
    while (std::getline(in, line)) {
        std::istringstream is(line);
        int ev = -1;
        if (!(is >> ev)) continue;
        if (ev < 0 || ev >= 25) continue;
        if (ev >= 19) continue; // we only ever persisted one-shots
        if (!s_seen[ev]) {
            s_seen[ev] = true;
            s_events.push_back(ev);
        }
    }
}

static void AppendToFile(int ev) {
    std::FILE* f = std::fopen(EventsPath().c_str(), "a");
    if (!f) return;
    std::fprintf(f, "%d\n", ev);
    std::fclose(f);
}

} // namespace

namespace EventReplay {

void Record(uint32_t ev) {
    if (ev >= 19) return;        // only one-shots are persisted
    if (ev >= 25) return;
    LoadOnce();
    if (s_seen[ev]) return;
    s_seen[ev] = true;
    s_events.push_back((int)ev);
    AppendToFile((int)ev);
}

void Replay(GloomMap& gmap, GameLogic& logic) {
    LoadOnce();
    bool gotele = false;
    Teleport tele;
    for (int ev : s_events) {
        gotele = false;
        gmap.ExecuteEvent((uint32_t)ev, gotele, tele);
        logic.MarkEventHit(ev);
    }
}

void Clear() {
    s_loaded = true;
    std::fill(std::begin(s_seen), std::end(s_seen), false);
    s_events.clear();
    std::remove(EventsPath().c_str());
}

} // namespace EventReplay
