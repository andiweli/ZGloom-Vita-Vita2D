#include "vita/RendererVita2D.h"
#include "vita/RendererHooks.h"

#ifdef __vita__
#include <vita2d.h>
#include <psp2/gxm.h>
#include <psp2/display.h>
#include <psp2/kernel/clib.h>
#include <psp2/kernel/threadmgr.h> // sceKernelDelayThread
#include <psp2/kernel/processmgr.h>
#include "config.h"
#endif

#include <string.h>

namespace zgloom_vita {

static inline uint64_t get_time_us() {
#ifdef __vita__
    // Returns time in microseconds since boot
    return sceKernelGetProcessTimeWide();
#else
    return 0;
#endif
}

RendererVita2D& RendererVita2D::Get() {
    static RendererVita2D s;
    return s;
}

bool RendererVita2D::Initialize(int screenW, int screenH) {
#ifdef __vita__
    if (!vita2d_init()) {
        return false;
    }
    screen_w_ = screenW;
    screen_h_ = screenH;
    for (int i=0;i<kNumTex;++i) tex_[i] = nullptr;
    cur_tex_ = 0;
    tex_w_ = tex_h_ = 0;
    // enforce fullscreen Fill by default
    force_fill_ = true;
    scale_mode_ = ScaleMode::Fill;
    // setup timer pacing
    period_us_ = 20000; // 50 Hz
    next_tick_us_ = get_time_us() + period_us_;
    initialized_ = true;
    return true;
#else
    (void)screenW; (void)screenH;
    initialized_ = true;
    return true;
#endif
}

bool RendererVita2D::Init(int srcW, int srcH, int screenW, int screenH) {
    src_w_ = srcW;
    src_h_ = srcH;
    return Initialize(screenW, screenH);
}

void RendererVita2D::Shutdown() {
#ifdef __vita__
    for (int i=0;i<kNumTex;++i){
        if (tex_[i]) {
            vita2d_free_texture((vita2d_texture*)tex_[i]);
            tex_[i] = nullptr;
        }
    }
    if (initialized_) {
        vita2d_fini();
    }
#endif
    initialized_ = false;
    tex_w_ = tex_h_ = 0;
}

void RendererVita2D::ensure_textures(int w, int h) {
#ifdef __vita__
    if (tex_w_ == w && tex_h_ == h && tex_[0]) return;
    for (int i=0;i<kNumTex;++i){
        if (tex_[i]) {
            vita2d_free_texture((vita2d_texture*)tex_[i]);
            tex_[i] = nullptr;
        }
    }
    for (int i=0;i<kNumTex;++i){
        tex_[i] = vita2d_create_empty_texture_format(w, h, SCE_GXM_TEXTURE_FORMAT_A8B8G8R8);
        vita2d_texture_set_filters((vita2d_texture*)tex_[i],
                                   SCE_GXM_TEXTURE_FILTER_POINT,
                                   SCE_GXM_TEXTURE_FILTER_POINT);
    }
    tex_w_ = w;
    tex_h_ = h;
#else
    (void)w; (void)h;
#endif
}

void RendererVita2D::SetIntegerScaling(bool enable) {
    integer_scaling_ = enable;
}

void RendererVita2D::SetClearColor(uint32_t argb) {
    clear_color_ = argb;
}

void RendererVita2D::SetScaleMode(ScaleMode m) {
    // If force_fill_ is on, keep Fill to avoid black bars
    if (force_fill_) {
        scale_mode_ = ScaleMode::Fill;
    } else {
        scale_mode_ = m;
    }
}

void RendererVita2D::SetSourceIsABGR(bool is_abgr) {
    source_is_abgr_ = is_abgr;
}

void RendererVita2D::SetTargetFps(int fps) {
    if (fps == 60) { period_us_ = 16666; target_fps_ = 60; }
    else if (fps == 50) { period_us_ = 20000; target_fps_ = 50; }
    else if (fps == 30) { period_us_ = 33333; target_fps_ = 30; }
    else return;
    // realign next tick from now to avoid drift
    next_tick_us_ = get_time_us() + period_us_;
}

static inline uint32_t swizzle_argb_to_abgr(uint32_t p) {
    uint32_t r = (p & 0x00FF0000) >> 16;
    uint32_t g = (p & 0x0000FF00);
    uint32_t b = (p & 0x000000FF) << 16;
    return 0xFF000000u | r | g | b;
}

void RendererVita2D::upload_to_texture(int idx, const void* src_pixels, int src_pitch) {
#ifdef __vita__
    auto* t = (vita2d_texture*)tex_[idx];
    const int W = vita2d_texture_get_width(t);
    const int H = vita2d_texture_get_height(t);
    uint32_t* dst = (uint32_t*)vita2d_texture_get_datap(t);
    const int dst_stride = vita2d_texture_get_stride(t) / 4;

    const uint8_t* srow = (const uint8_t*)src_pixels;

    if (src_pitch == W * 4 && dst_stride == W) {
        const uint32_t* sp = (const uint32_t*)srow;
        const int count = W * H;
        if (source_is_abgr_) {
            sceClibMemcpy(dst, sp, (size_t)count * 4);
            return;
        } else {
            for (int i=0;i<count;++i) dst[i] = swizzle_argb_to_abgr(sp[i]);
            return;
        }
    }

    for (int y=0; y<H; ++y) {
        const uint32_t* sp = (const uint32_t*)(srow + y * src_pitch);
        uint32_t* dp = dst + y * dst_stride;
        if (source_is_abgr_) {
            sceClibMemcpy(dp, sp, (size_t)W * 4);
        } else {
            for (int x=0; x<W; ++x) dp[x] = swizzle_argb_to_abgr(sp[x]);
        }
    }
#else
    (void)idx; (void)src_pixels; (void)src_pitch;
#endif
}

void RendererVita2D::draw_current_texture() {
#ifdef __vita__
    // Always Fill when forced (no bars)
    if (force_fill_ || scale_mode_ == ScaleMode::Fill) {
        float scale_x = (float)screen_w_ / (float)tex_w_;
        float scale_y = (float)screen_h_ / (float)tex_h_;
                {
            SceGxmTextureFilter f = (Config::GetBilinearFilter() != 0) ? SCE_GXM_TEXTURE_FILTER_LINEAR : SCE_GXM_TEXTURE_FILTER_POINT;
            vita2d_texture_set_filters((vita2d_texture*)tex_[cur_tex_], f, f);
        }
vita2d_draw_texture_scale((vita2d_texture*)tex_[cur_tex_], 0.0f, 0.0f, scale_x, scale_y);
        return;
    }

    // Other modes (kept for completeness)
    int dst_w = screen_w_;
    int dst_h = screen_h_;
    int dst_x = 0;
    int dst_y = 0;

    auto mode = scale_mode_;
    if (mode == ScaleMode::Auto) {
        mode = integer_scaling_ ? ScaleMode::Integer : ScaleMode::Fit;
    }

    if (mode == ScaleMode::Cover || mode == ScaleMode::Fit) {
        float s = (float)screen_w_ / (float)tex_w_;
        if (mode == ScaleMode::Fit) {
            if ((int)(tex_h_ * s) > screen_h_) s = (float)screen_h_ / (float)tex_h_;
        } else {
            if ((int)(tex_h_ * s) < screen_h_) s = (float)screen_h_ / (float)tex_h_;
        }
        dst_w = (int)(tex_w_ * s);
        dst_h = (int)(tex_h_ * s);
        dst_x = (screen_w_ - dst_w) / 2;
        dst_y = (screen_h_ - dst_h) / 2;   // FIX
        float scale_x = (float)dst_w / (float)tex_w_;
        float scale_y = (float)dst_h / (float)tex_h_;
        vita2d_draw_texture_scale((vita2d_texture*)tex_[cur_tex_], (float)dst_x, (float)dst_y, scale_x, scale_y);
        return;
    }

    // Integer scaling (crisp)
    int sx = screen_w_ / tex_w_;
    int sy = screen_h_ / tex_h_;
    if (sx < 1) sx = 1;
    if (sy < 1) sy = 1;
    dst_w = tex_w_ * sx;
    dst_h = tex_h_ * sy;
    if (dst_w > screen_w_) dst_w = screen_w_;
    if (dst_h > screen_h_) dst_h = screen_h_;  // FIX
    dst_x = (screen_w_ - dst_w) / 2;
    dst_y = (screen_h_ - dst_h) / 2;

    float scale_x = (float)dst_w / (float)tex_w_;
    float scale_y = (float)dst_h / (float)tex_h_;
    vita2d_draw_texture_scale((vita2d_texture*)tex_[cur_tex_], (float)dst_x, (float)dst_y, scale_x, scale_y);
#endif
}

void RendererVita2D::pace_timer() {
#ifdef __vita__
    // Timer-driven pacing; rely on swap to vsync to the next vblank.
    if (period_us_ == 0) return;
    uint64_t now = get_time_us();
    // If we're early, sleep in small chunks
    while ((int64_t)(next_tick_us_ - now) > 1500) { // >1.5ms lead
        sceKernelDelayThread(1000); // sleep 1ms
        now = get_time_us();
    }
    // Spin for the last ~1ms window
    while ((int64_t)(next_tick_us_ - now) > 0) {
        now = get_time_us();
    }
    // Schedule next tick; avoid spiral if behind
    next_tick_us_ += period_us_;
    uint64_t max_catchup = now + period_us_;
    if (next_tick_us_ < now) next_tick_us_ = max_catchup;
#endif
}

void RendererVita2D::PresentARGB8888(const void* src_pixels, int src_pitch) {
    if (!initialized_ || !src_pixels) return;
#ifdef __vita__
    ensure_textures(src_w_, src_h_);

    pace_timer();

    int next = (cur_tex_ + 1) % kNumTex;
    upload_to_texture(next, src_pixels, src_pitch);
    cur_tex_ = next;

    vita2d_start_drawing();
    draw_current_texture();
    vita2d_end_drawing();
    vita2d_swap_buffers(); // vsync here
#else
    (void)src_pixels; (void)src_pitch;
#endif
}

void RendererVita2D::PresentARGB8888WithHook(const void* src_pixels, int src_pitch) {
    if (!initialized_ || !src_pixels) return;
#ifdef __vita__
    ensure_textures(src_w_, src_h_);

    pace_timer();

    int next = (cur_tex_ + 1) % kNumTex;
    upload_to_texture(next, src_pixels, src_pitch);
    cur_tex_ = next;

    vita2d_start_drawing();
    draw_current_texture();
    RendererHooks::EffectsDrawOverlaysVita2D();
    vita2d_end_drawing();
    vita2d_swap_buffers(); // vsync here
#else
    (void)src_pixels; (void)src_pitch;
#endif
}

void RendererVita2D::PresentARGB8888WithHook(const void* src_pixels, int src_pitch, void (*overlayCallback)()) {
    if (!initialized_ || !src_pixels) return;
#ifdef __vita__
    ensure_textures(src_w_, src_h_);

    pace_timer();

    int next = (cur_tex_ + 1) % kNumTex;
    upload_to_texture(next, src_pixels, src_pitch);
    cur_tex_ = next;

    vita2d_start_drawing();
    draw_current_texture();
    if (overlayCallback) overlayCallback();
    vita2d_end_drawing();
    vita2d_swap_buffers(); // vsync here
#else
    (void)src_pixels; (void)src_pitch; (void)overlayCallback;
#endif
}

} // namespace zgloom_vita
