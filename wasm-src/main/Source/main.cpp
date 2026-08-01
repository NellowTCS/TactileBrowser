#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <emscripten.h>
#include <fdm.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static FdmSurface *g_surface = nullptr;
static char current_url[512] = "https://example.com";

static void update_js_status(const char *message, const char *color_hex) {
  (void)message;
  (void)color_hex;
  EM_ASM(
      {
        const msg = UTF8ToString($0);
        const col = UTF8ToString($1);
        if (window.TactileBrowserWasm && typeof window.TactileBrowserWasm.setStatus === 'function') {
          window.TactileBrowserWasm.setStatus(msg, col);
        }
      },
      message ? message : "", color_hex ? color_hex : "#FFD93D");
}

static void canvas_begin_frame(FdmSurface *surf, int width, int height) {
  (void)surf;
  (void)width;
  (void)height;
  EM_ASM(
      {
        const canvas = window.TactileBrowserCanvas;
        if (canvas) {
          const ctx = canvas.getContext('2d');
          ctx.clearRect(0, 0, $0, $1);
          ctx.fillStyle = '#1E1E1E';
          ctx.fillRect(0, 0, $0, $1);
        }
      },
      width, height);
}

static void canvas_end_frame(FdmSurface *surf) {
  (void)surf;
}

static void canvas_fill_rect(FdmSurface *surf, int x, int y, int w, int h, FdmColor color) {
  (void)surf;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  unsigned int r = (color >> 16) & 0xFF;
  unsigned int g = (color >> 8) & 0xFF;
  unsigned int b = color & 0xFF;
  char color_str[32];
  snprintf(color_str, sizeof(color_str), "rgb(%u,%u,%u)", r, g, b);

  EM_ASM(
      {
        const canvas = window.TactileBrowserCanvas;
        if (canvas) {
          const ctx = canvas.getContext('2d');
          ctx.fillStyle = UTF8ToString($4);
          ctx.fillRect($0, $1, $2, $3);
        }
      },
      x, y, w, h, color_str);
}

static void canvas_fill_rect_gradient(FdmSurface *surf, int x, int y, int w, int h, const FdmLinearGradient *gradient) {
  (void)surf;
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  if (!gradient || gradient->stop_count == 0) return;
  unsigned int r1 = (gradient->stops[0].color >> 16) & 0xFF;
  unsigned int g1 = (gradient->stops[0].color >> 8) & 0xFF;
  unsigned int b1 = gradient->stops[0].color & 0xFF;

  unsigned int r2 = (gradient->stops[gradient->stop_count - 1].color >> 16) & 0xFF;
  unsigned int g2 = (gradient->stops[gradient->stop_count - 1].color >> 8) & 0xFF;
  unsigned int b2 = gradient->stops[gradient->stop_count - 1].color & 0xFF;

  char c1[32], c2[32];
  snprintf(c1, sizeof(c1), "rgb(%u,%u,%u)", r1, g1, b1);
  snprintf(c2, sizeof(c2), "rgb(%u,%u,%u)", r2, g2, b2);

  EM_ASM(
      {
        const canvas = window.TactileBrowserCanvas;
        if (canvas) {
          const ctx = canvas.getContext('2d');
          const grad = ctx.createLinearGradient($0, $1, $0 + $2, $1 + $3);
          grad.addColorStop(0, UTF8ToString($4));
          grad.addColorStop(1, UTF8ToString($5));
          ctx.fillStyle = grad;
          ctx.fillRect($0, $1, $2, $3);
        }
      },
      x, y, w, h, c1, c2);
}

static void canvas_draw_text(FdmSurface *surf, const char *text, int x, int y, int max_width, int font_size, FdmColor color, int align, bool underline) {
  (void)surf;
  (void)x;
  (void)y;
  (void)max_width;
  (void)font_size;
  (void)underline;
  if (!text || text[0] == '\0') return;
  unsigned int r = (color >> 16) & 0xFF;
  unsigned int g = (color >> 8) & 0xFF;
  unsigned int b = color & 0xFF;
  char color_str[32];
  snprintf(color_str, sizeof(color_str), "rgb(%u,%u,%u)", r, g, b);

  const char *align_str = "left";
  if (align == FDM_ALIGN_CENTER) align_str = "center";
  else if (align == FDM_ALIGN_RIGHT) align_str = "right";
  (void)align_str;

  EM_ASM(
      {
        const canvas = window.TactileBrowserCanvas;
        if (canvas) {
          const ctx = canvas.getContext('2d');
          ctx.save();
          ctx.font = $5 + 'px sans-serif';
          ctx.fillStyle = UTF8ToString($6);
          ctx.textAlign = UTF8ToString($7);
          ctx.textBaseline = 'top';
          ctx.fillText(UTF8ToString($0), $1, $2);
          if ($8) {
            const metrics = ctx.measureText(UTF8ToString($0));
            const tw = metrics.width;
            ctx.strokeStyle = UTF8ToString($6);
            ctx.lineWidth = 1;
            ctx.beginPath();
            ctx.moveTo($1, $2 + $5 + 2);
            ctx.lineTo($1 + tw, $2 + $5 + 2);
            ctx.stroke();
          }
          ctx.restore();
        }
      },
      text, x, y, max_width, font_size, color_str, align_str, underline ? 1 : 0);
}

static int canvas_measure_text(FdmSurface *surf, const char *text, int font_size) {
  (void)surf;
  if (!text || text[0] == '\0') return 0;
  int width = EM_ASM_INT(
      {
        const canvas = window.TactileBrowserCanvas;
        if (canvas) {
          const ctx = canvas.getContext('2d');
          ctx.save();
          ctx.font = $1 + 'px sans-serif';
          const w = ctx.measureText(UTF8ToString($0)).width;
          ctx.restore();
          return Math.ceil(w);
        }
        return 0;
      },
      text, font_size);
  return width > 0 ? width : (int)(strlen(text) * (size_t)(font_size / 2));
}

static const FdmSurfaceOps canvas_surface_ops = {
    canvas_begin_frame,
    canvas_end_frame,
    canvas_fill_rect,
    canvas_fill_rect_gradient,
    canvas_draw_text,
    canvas_measure_text,
};

extern "C" {

EMSCRIPTEN_KEEPALIVE
void wasm_render_html(const char *url, const char *html) {
  if (!g_surface || !url || !html) return;
  size_t len = strlen(html);
  strncpy(current_url, url, sizeof(current_url) - 1);
  current_url[sizeof(current_url) - 1] = '\0';

  FdmResult res = fdm_render_html(g_surface, current_url, html, len, SCREEN_WIDTH, SCREEN_HEIGHT);
  if (res == FDM_OK) {
    update_js_status("Loaded", "#4CAF50");
  } else {
    update_js_status("Render failed", "#FF6B6B");
  }
}

EMSCRIPTEN_KEEPALIVE
void wasm_pointer_event(int x, int y, int action) {
  if (!g_surface) return;
  fdm_handle_pointer(g_surface, x, y, action);
}

EMSCRIPTEN_KEEPALIVE
void wasm_scroll_event(int delta_y) {
  if (!g_surface) return;
  fdm_scroll_by(g_surface, delta_y);
}

EMSCRIPTEN_KEEPALIVE
void start_app() {
  fdm_init();
  g_surface = (FdmSurface *)malloc(sizeof(FdmSurface));
  g_surface->ops = &canvas_surface_ops;
  g_surface->platform_data = nullptr;
  g_surface->user_data = nullptr;

  fdm_set_link_handler([](void *user_data, const char *url) {
    (void)user_data;
    if (!url || url[0] == '\0') return;
    EM_ASM(
        {
          if (window.TactileBrowserWasm && typeof window.TactileBrowserWasm.loadUrl === 'function') {
            window.TactileBrowserWasm.loadUrl(UTF8ToString($0));
          }
        },
        url);
  }, nullptr);

  update_js_status("Ready", "#FFD93D");
}

} // extern "C"

int main() {
  return 0;
}
