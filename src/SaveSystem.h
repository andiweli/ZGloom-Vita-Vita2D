#pragma once
#include <string>

struct Camera;
class GloomMap;

namespace SaveSystem {

struct SaveData {
    std::string levelPath;
    int x = 0;
    int z = 0;
    int rot = 0;     // 0..255
    int hp = 100;
    int weapon = 0;  // 0..4
    int reload = 0;  // fire/reload gate
    int reloadcnt = 0;
    int flat = -1;   // 0..9 (floor/roof set), -1 = unknown
};

// Call whenever a new map will be loaded by the script:
void SetCurrentLevelPath(const std::string& levelPath);
// Call whenever the script sets a flat (SOP_LOADFLAT):
void SetCurrentFlat(int flat);
int  GetCurrentFlat();

bool HasSaveForCurrentGame();

bool SavePosition(const Camera& cam, const GloomMap& gmap);
bool LoadFromDisk(SaveData& out);
bool ApplyToGame(const SaveData& d, Camera& cam, GloomMap& gmap);

} // namespace SaveSystem
