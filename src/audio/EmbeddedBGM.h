#pragma once

namespace BGM {
    // Einmalig beim Programmstart/-ende
    void Init();
    void Shutdown();

    // Steuerung der Atmosphere-BGM
    void PlayLooping();     // nur Levels
    void Stop();
    void SetVolume9(int v); // 0..9
}
