#include "AtmosphereVolume.h"
#include "EmbeddedBGM.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>

namespace {
    const char* kCfgPath = "ux0:/data/ZGloom/config.txt";
    int s_atmos = -1; // -1 = noch nicht geladen

    static bool keyEquals(const char* a, const char* b) {
        while (*a && *b) {
            char ca = (*a>='a'&&*a<='z') ? *a-32 : *a;
            char cb = (*b>='a'&&*b<='z') ? *b-32 : *b;
            if (ca != cb) return false;
            ++a; ++b;
        }
        return *a==0 && *b==0;
    }

    static int clamp09(int v) { return v < 0 ? 0 : (v > 9 ? 9 : v); }

    static int parseInt(const char* s) { return std::atoi(s); }

    static void splitLines(const std::string& txt, std::vector<std::string>& lines) {
        size_t start = 0;
        while (start <= txt.size()) {
            size_t nl = txt.find_first_of("\r\n", start);
            if (nl == std::string::npos) {
                lines.push_back(txt.substr(start));
                break;
            }
            lines.push_back(txt.substr(start, nl - start));
            // CRLF/CR/LF überspringen
            size_t next = nl + 1;
            if (next < txt.size() && (txt[next] == '\n' || txt[next] == '\r')) ++next;
            start = next;
        }
    }

    static std::string joinLines(const std::vector<std::string>& lines) {
        std::string out;
        out.reserve(lines.size() * 16);
        for (size_t i=0;i<lines.size();++i) {
            out += lines[i];
            out += "\n";
        }
        return out;
    }
}

namespace AtmosphereVolume {

int Get() {
    if (s_atmos < 0) {
        LoadFromConfig();
        if (s_atmos < 0) s_atmos = 7; // moderater Default, falls kein Eintrag existiert
    }
    return s_atmos;
}

void Set(int v) {
    s_atmos = clamp09(v);
    SaveToConfig();
    // Laufende Musik live anpassen
    BGM::SetVolume9(s_atmos);
}

int ToSDLVolume(int v) {
    v = clamp09(v);
    // 0..9 -> 0..128 (linear)
    return (v * 128) / 9;
}

void LoadFromConfig() {
    if (s_atmos >= 0) return;
    FILE* f = std::fopen(kCfgPath, "rb");
    if (!f) { s_atmos = 7; return; }
    std::string txt;
    char buf[512];
    while (std::fgets(buf, sizeof(buf), f)) { txt += buf; }
    std::fclose(f);

    std::vector<std::string> lines;
    splitLines(txt, lines);

    for (const auto& L : lines) {
        auto pos = L.find('=');
        if (pos == std::string::npos) continue;
        std::string key = L.substr(0, pos);
        std::string val = L.substr(pos+1);
        // Whitespace trim
        auto trim = [](std::string& s){
            size_t a = 0; while (a < s.size() && (s[a]==' ' || s[a]=='\t')) ++a;
            size_t b = s.size(); while (b> a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r')) --b;
            s = s.substr(a, b-a);
        };
        trim(key); trim(val);

        if ( keyEquals(key.c_str(), "ATMOS") ||
             keyEquals(key.c_str(), "ATMOSPHERE VOLUME") ||
             keyEquals(key.c_str(), "ATMOSVOL") ||
             keyEquals(key.c_str(), "ATMOSPHEREVOLUME") ) {
            s_atmos = clamp09(parseInt(val.c_str()));
            return;
        }
    }
    // Nichts gefunden -> bleibt -1 (wird bei Get() auf Default gesetzt)
}

void SaveToConfig() {
    // Alte Datei lesen (wenn vorhanden)
    std::vector<std::string> lines;
    {
        FILE* f = std::fopen(kCfgPath, "rb");
        if (f) {
            std::string txt; char buf[512];
            while (std::fgets(buf, sizeof(buf), f)) { txt += buf; }
            std::fclose(f);
            splitLines(txt, lines);
        }
    }

    auto isAtmosKey = [](const std::string& key)->bool{
        return keyEquals(key.c_str(), "ATMOS") ||
               keyEquals(key.c_str(), "ATMOSPHERE VOLUME") ||
               keyEquals(key.c_str(), "ATMOSVOL") ||
               keyEquals(key.c_str(), "ATMOSPHEREVOLUME");
    };

    bool updated = false;
    for (auto& L : lines) {
        auto pos = L.find('=');
        if (pos == std::string::npos) continue;
        std::string key = L.substr(0, pos);
        auto trim = [](std::string& s){
            size_t a = 0; while (a < s.size() && (s[a]==' ' || s[a]=='\t')) ++a;
            size_t b = s.size(); while (b> a && (s[b-1]==' '||s[b-1]=='\t'||s[b-1]=='\r')) --b;
            s = s.substr(a, b-a);
        };
        trim(key);
        if (isAtmosKey(key)) {
            L = "ATMOS=" + std::to_string(clamp09(s_atmos));
            updated = true;
        }
    }
    if (!updated) {
        lines.push_back("ATMOS=" + std::to_string(clamp09(s_atmos)));
    }

    std::string out = joinLines(lines);
    if (FILE* f = std::fopen(kCfgPath, "wb")) {
        std::fwrite(out.data(), 1, out.size(), f);
        std::fclose(f);
    }
}

} // namespace AtmosphereVolume

// ---- Menü-Glue (für MenuEntry Funktionszeiger) ----
extern "C" int MenuGetAtmosphereVol() {
    return AtmosphereVolume::Get();
}
extern "C" void MenuSetAtmosphereVol(int v) {
    AtmosphereVolume::Set(v);
}
