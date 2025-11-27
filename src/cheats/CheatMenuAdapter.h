#pragma once
#include <string>
#include "CheatSystem.h"

namespace CheatsMenu {

inline void ReloadFromDisk() { Cheats::Reload(); }
inline bool SaveToDisk() { return Cheats::Save(); }

inline const char* GodModeValueText() {
    return Cheats::GetGodMode() ? "ON" : "OFF";
}
inline void ToggleGodMode() {
    Cheats::SetGodMode(!Cheats::GetGodMode());
}

inline const char* OneHitKillValueText() {
    return Cheats::GetOneHitKill() ? "ON" : "OFF";
}
inline void ToggleOneHitKill() {
    Cheats::SetOneHitKill(!Cheats::GetOneHitKill());
}

inline const char* StartWeaponValueText() {
    static std::string s;
    s = std::to_string(Cheats::GetStartWeapon()); // 0..4
    return s.c_str();
}
inline void CycleStartWeapon(int dir) {
    int w = Cheats::GetStartWeapon();
    w += (dir<0?-1:1);
    if (w < 0) w = 4;
    if (w > 4) w = 0;
    Cheats::SetStartWeapon(w);
}

inline const char* BoostLevelValueText() {
    static std::string s;
    s = std::to_string(Cheats::GetBoostLevel()); // 0..4
    return s.c_str();
}
inline void CycleBoostLevel(int dir) {
    int lv = Cheats::GetBoostLevel();
    lv += (dir<0?-1:1);
    if (lv < 0) lv = 4;
    if (lv > 4) lv = 0;
    Cheats::SetBoostLevel(lv);
}

} // namespace CheatsMenu
