// Weak no-op fallback if a strong definition exists elsewhere.
namespace RendererHooks {
__attribute__((weak))
void setCameraMotion(float, float, float) {}
} // namespace RendererHooks
