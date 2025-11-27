#pragma once
#include <stdint.h>

namespace zgloom_vita {

class RendererVita2D {
public:
    void SetBilinear(bool on);
    bool GetBilinear() const;
    enum class ScaleMode {
        Auto,
        Integer,
        Fit,
        Fill,
        Cover
    };

    static RendererVita2D& Get();

    // Initializes vita2d and backbuffer; src is your software buffer size
    bool Initialize(int screenW, int screenH);
    bool Init(int srcW, int srcH, int screenW, int screenH);
    void Shutdown();

    // Present the ARGB8888 software buffer (pitch in bytes)
    void PresentARGB8888(const void* src_pixels, int src_pitch);
    void PresentARGB8888WithHook(const void* src_pixels, int src_pitch);
    void PresentARGB8888WithHook(const void* src_pixels, int src_pitch, void (*overlayCallback)());

    void SetIntegerScaling(bool enable);
    void SetClearColor(uint32_t argb);
    void SetScaleMode(ScaleMode m);
    void SetSourceIsABGR(bool is_abgr);

    // Target FPS pacing. Supported: 60, 50, 30. Default is 50.
    void SetTargetFps(int fps);

    bool IsInitialized() const { return initialized_; }

private:
        bool bilinear_enabled_ = false;
RendererVita2D() = default;
    ~RendererVita2D() = default;
    RendererVita2D(const RendererVita2D&) = delete;
    RendererVita2D& operator=(const RendererVita2D&) = delete;

    void ensure_textures(int w, int h);
    void upload_to_texture(int idx, const void* src_pixels, int src_pitch);
    void draw_current_texture();
    void pace_timer();

private:
    bool initialized_ = false;

    int src_w_ = 320;
    int src_h_ = 256;

    int screen_w_ = 960;
    int screen_h_ = 544;

    static const int kNumTex = 3;
    void* tex_[kNumTex] = {nullptr, nullptr, nullptr};
    int   cur_tex_ = 0;
    int   tex_w_ = 0;
    int   tex_h_ = 0;

    bool integer_scaling_ = true;
    uint32_t clear_color_ = 0xFF000000;

    bool source_is_abgr_ = false;
    int target_fps_ = 50;   // default capped at 50

    // Always fullscreen Fill (no bars)
    bool force_fill_ = true;
    ScaleMode scale_mode_ = ScaleMode::Fill; // default Fill

    // Timer-based pacing (microseconds)
    uint64_t next_tick_us_ = 0;
    uint64_t period_us_    = 20000; // 50 Hz default
};

} // namespace zgloom_vita
