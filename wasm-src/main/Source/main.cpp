#include <algorithm>
#include <cstdio>
#include <cstring>
#include <emscripten.h>
#include <emscripten/html5.h>
#include <lv_sdl_keyboard.h>
#include <lv_sdl_mouse.h>
#include <lv_sdl_mousewheel.h>
#include <lv_sdl_window.h>
#include <lvgl.h>
#include <string>
#include <tactilebrowser_core.h>
#include <unordered_map>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

static lv_display_t *display = nullptr;
static lv_indev_t *mouse_indev = nullptr;
static lv_indev_t *keyboard_indev = nullptr;
static lv_indev_t *wheel_indev = nullptr;
static lv_obj_t *content_area = nullptr;
static char current_url[512] = "https://example.com";
static double last_frame_time_ms = 0.0;
static std::unordered_map<lv_obj_t *, std::string> link_targets;
struct LabelSelectionState {
  uint32_t anchor = LV_LABEL_TEXT_SELECTION_OFF;
  bool active = false;
};
static std::unordered_map<lv_obj_t *, LabelSelectionState>
    label_selection_states;

static bool has_http_scheme(const char *url) {
  if (!url)
    return false;
  return strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0;
}

static std::string extract_origin_from_url(const char *url) {
  if (!url || url[0] == '\0')
    return "";
  std::string value(url);
  size_t scheme_pos = value.find("://");
  if (scheme_pos == std::string::npos)
    return "";
  size_t host_start = scheme_pos + 3;
  size_t path_pos = value.find('/', host_start);
  if (path_pos == std::string::npos) {
    return value;
  }
  return value.substr(0, path_pos);
}

static std::string resolve_relative_url(const char *candidate,
                                        const char *base_url) {
  if (!candidate || candidate[0] == '\0')
    return "";
  if (has_http_scheme(candidate)) {
    return std::string(candidate);
  }

  if (candidate[0] == '/') {
    if (!base_url || base_url[0] == '\0') {
      return "";
    }
    std::string origin = extract_origin_from_url(base_url);
    if (origin.empty()) {
      return "";
    }
    return origin + candidate;
  }

  return std::string(candidate);
}

extern "C" void load_url(const char *url);

static bool label_index_from_event(lv_obj_t *label, lv_event_t *event,
                                   uint32_t *out_index) {
  if (!label || !event || !out_index)
    return false;
  lv_indev_t *indev = lv_event_get_indev(event);
  if (!indev)
    return false;

  if (lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER) {
    return false;
  }

  lv_point_t global_point;
  lv_indev_get_point(indev, &global_point);

  lv_area_t coords;
  lv_obj_get_coords(label, &coords);

  lv_point_t local_point;
  local_point.x = global_point.x - coords.x1;
  local_point.y = global_point.y - coords.y1;

  uint32_t index = lv_label_get_letter_on(label, &local_point, true);
  if (index == LV_LABEL_TEXT_SELECTION_OFF) {
    const char *text = lv_label_get_text(label);
    index = text ? (uint32_t)strlen(text) : 0;
  }

  *out_index = index;
  return true;
}

static bool label_has_active_selection(lv_obj_t *label) {
  if (!label)
    return false;
  uint32_t start = lv_label_get_text_selection_start(label);
  uint32_t end = lv_label_get_text_selection_end(label);
  if (start == LV_LABEL_TEXT_SELECTION_OFF ||
      end == LV_LABEL_TEXT_SELECTION_OFF) {
    return false;
  }
  return start != end;
}

static void label_selection_event_cb(lv_event_t *event) {
  if (!event)
    return;
  lv_obj_t *label = static_cast<lv_obj_t *>(lv_event_get_target(event));
  if (!label)
    return;

  lv_event_code_t code = lv_event_get_code(event);
  if (code == LV_EVENT_DELETE) {
    label_selection_states.erase(label);
    return;
  }

  if (code == LV_EVENT_PRESSED) {
    uint32_t index = 0;
    if (!label_index_from_event(label, event, &index))
      return;
    LabelSelectionState &state = label_selection_states[label];
    state.anchor = index;
    state.active = true;
    lv_label_set_text_selection_start(label, index);
    lv_label_set_text_selection_end(label, index);
    return;
  }

  auto it = label_selection_states.find(label);
  if (it == label_selection_states.end())
    return;

  if (code == LV_EVENT_PRESSING) {
    if (!it->second.active)
      return;
    uint32_t index = 0;
    if (!label_index_from_event(label, event, &index))
      return;
    uint32_t start = std::min(it->second.anchor, index);
    uint32_t end = std::max(it->second.anchor, index);
    lv_label_set_text_selection_start(label, start);
    lv_label_set_text_selection_end(label, end);
    return;
  }

  if (code == LV_EVENT_RELEASED || code == LV_EVENT_PRESS_LOST) {
    it->second.active = false;
  }
}

static void update_js_status(const char *message, const char *color_hex) {
  EM_ASM(
      {
        const msg = UTF8ToString($0);
        const col = UTF8ToString($1);
        if (window.TactileBrowserWasm &&typeof window.TactileBrowserWasm
                    .setStatus === 'function') {
          window.TactileBrowserWasm.setStatus(msg, col);
        }
      },
      message ? message : "", color_hex ? color_hex : "#FFD93D");
}

static lv_obj_t *resolve_target(Renderer *renderer) {
  if (renderer && renderer->platform_data) {
    return static_cast<lv_obj_t *>(renderer->platform_data);
  }
  if (content_area) {
    return content_area;
  }
  return lv_screen_active();
}

static lv_obj_t *lvgl_create_text_widget(Renderer *renderer, const char *value,
                                         const char *placeholder, int x, int y,
                                         int width, int height,
                                         bool multiline) {
  lv_obj_t *parent = resolve_target(renderer);
  if (!parent)
    return nullptr;

  lv_obj_t *textarea = lv_textarea_create(parent);
  lv_obj_set_pos(textarea, x, y);
  lv_obj_set_size(textarea, width > 0 ? width : (multiline ? 320 : 240),
                  height > 0 ? height : (multiline ? 120 : 40));
  lv_textarea_set_one_line(textarea, !multiline);
  lv_textarea_set_text(textarea, value ? value : "");
  if (placeholder && placeholder[0] != '\0') {
    lv_textarea_set_placeholder_text(textarea, placeholder);
  }
  lv_obj_set_scrollbar_mode(textarea, LV_SCROLLBAR_MODE_AUTO);
  lv_obj_set_style_bg_color(textarea, lv_color_hex(0x141414), 0);
  lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(textarea, lv_color_hex(0x2D333B), 0);
  lv_obj_set_style_border_width(textarea, 1, 0);
  lv_obj_set_style_pad_all(textarea, 6, 0);
  return textarea;
}

static lv_grad_dir_t gradient_dir_from_angle(float angle_deg) {
  float normalized = angle_deg;
  while (normalized < 0.0f)
    normalized += 360.0f;
  while (normalized >= 360.0f)
    normalized -= 360.0f;

  if ((normalized >= 45.0f && normalized < 135.0f) ||
      (normalized >= 225.0f && normalized < 315.0f)) {
    return LV_GRAD_DIR_HOR;
  }
  return LV_GRAD_DIR_VER;
}

static void show_message(const char *text, lv_color_t color) {
  if (!content_area)
    return;
  lv_obj_clean(content_area);
  lv_obj_t *label = lv_label_create(content_area);
  lv_label_set_text(label, text ? text : "No content");
  lv_obj_set_style_text_color(label, color, 0);
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(label, lv_pct(100));
  lv_obj_center(label);
}

static bool lvgl_renderer_init(Renderer * /*renderer*/) { return true; }
static void lvgl_renderer_cleanup(Renderer * /*renderer*/) {}

static void *lvgl_create_label(Renderer *renderer, const char *text, int x,
                               int y) {
  lv_obj_t *parent = resolve_target(renderer);
  lv_obj_t *label = lv_label_create(parent);
  lv_label_set_text(label, text ? text : "");
  lv_obj_set_pos(label, x, y);
  lv_obj_set_width(label, lv_pct(100));
  lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_color(label, lv_color_hex(0xF0F6FC), 0);
  lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(label, label_selection_event_cb, LV_EVENT_ALL, nullptr);
  return label;
}

static void *lvgl_create_button(Renderer *renderer, const char *text, int x,
                                int y) {
  lv_obj_t *parent = resolve_target(renderer);
  lv_obj_t *btn = lv_btn_create(parent);
  lv_obj_set_pos(btn, x, y);
  lv_obj_set_size(btn, 90, 40);
  lv_obj_t *label = lv_label_create(btn);
  lv_label_set_text(label, text ? text : "");
  lv_obj_center(label);
  return btn;
}

static void *lvgl_create_text_input(Renderer *renderer, const char *value,
                                    const char *placeholder, int x, int y,
                                    int width, int height) {
  return lvgl_create_text_widget(renderer, value, placeholder, x, y, width,
                                 height, false);
}

static void *lvgl_create_text_area(Renderer *renderer, const char *value, int x,
                                   int y, int width, int height) {
  return lvgl_create_text_widget(renderer, value, nullptr, x, y, width, height,
                                 true);
}

static void *lvgl_create_container(Renderer *renderer, int x, int y, int width,
                                   int height) {
  lv_obj_t *parent = resolve_target(renderer);
  lv_obj_t *container = lv_obj_create(parent);
  lv_obj_set_pos(container, x, y);
  lv_obj_set_size(container, width, height);
  lv_obj_set_style_bg_color(container, lv_color_hex(0x1E1E1E), 0);
  lv_obj_set_style_border_width(container, 0, 0);
  lv_obj_set_style_pad_all(container, 8, 0);
  return container;
}

static void lvgl_set_text_color(Renderer * /*renderer*/, void *widget,
                                uint32_t color) {
  if (!widget)
    return;
  lv_obj_set_style_text_color(static_cast<lv_obj_t *>(widget),
                              lv_color_hex(color), 0);
}

static void lvgl_set_bg_color(Renderer * /*renderer*/, void *widget,
                              uint32_t color) {
  if (!widget)
    return;
  lv_obj_t *obj = static_cast<lv_obj_t *>(widget);
  lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

static void lvgl_set_bg_gradient(Renderer * /*renderer*/, void *widget,
                                 const LinearGradientFill *gradient) {
  if (!widget || !gradient || gradient->stop_count == 0)
    return;
  lv_obj_t *obj = static_cast<lv_obj_t *>(widget);
  uint32_t start_color = gradient->stops[0].color;
  uint32_t end_color = gradient->stops[gradient->stop_count - 1].color;
  lv_obj_set_style_bg_color(obj, lv_color_hex(start_color), 0);
  lv_obj_set_style_bg_grad_color(obj, lv_color_hex(end_color), 0);
  lv_obj_set_style_bg_grad_dir(obj,
                               gradient_dir_from_angle(gradient->angle_deg), 0);
  lv_obj_set_style_bg_main_stop(obj, 0, 0);
  lv_obj_set_style_bg_grad_stop(obj, 255, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
}

static void lvgl_set_text_align(Renderer * /*renderer*/, void *widget,
                                int align) {
  if (!widget)
    return;
  lv_text_align_t lv_align = LV_TEXT_ALIGN_LEFT;
  if (align == 1)
    lv_align = LV_TEXT_ALIGN_CENTER;
  else if (align == 2)
    lv_align = LV_TEXT_ALIGN_RIGHT;
  lv_obj_set_style_text_align(static_cast<lv_obj_t *>(widget), lv_align, 0);
}

static void lvgl_clear_container(Renderer * /*renderer*/, void *container) {
  if (!container)
    return;
  lv_obj_clean(static_cast<lv_obj_t *>(container));
}

static int lvgl_get_height(Renderer * /*renderer*/, void *widget) {
  if (!widget)
    return 0;
  return lv_obj_get_height(static_cast<lv_obj_t *>(widget));
}

static void link_event_handler(lv_event_t *event) {
  lv_event_code_t code = lv_event_get_code(event);
  lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(event));
  if (code == LV_EVENT_DELETE) {
    link_targets.erase(target);
    return;
  }

  if (code == LV_EVENT_CLICKED) {
    if (label_has_active_selection(target)) {
      return;
    }
    auto it = link_targets.find(target);
    if (it != link_targets.end() && !it->second.empty()) {
      load_url(it->second.c_str());
    }
  }
}

static void lvgl_register_link(Renderer * /*renderer*/, void *widget,
                               const char *url) {
  if (!widget || !url || url[0] == '\0')
    return;
  lv_obj_t *obj = static_cast<lv_obj_t *>(widget);
  link_targets[obj] = url;
  lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(obj, link_event_handler, LV_EVENT_ALL, NULL);
  lv_obj_set_style_text_color(obj, lv_color_hex(0x4EA1FF), 0);
  lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, 0);
}

static RenderInterface renderer_iface = {
    lvgl_renderer_init,   lvgl_renderer_cleanup,  lvgl_create_label,
    lvgl_create_button,   lvgl_create_text_input, lvgl_create_text_area,
    lvgl_register_link,   lvgl_create_container,  lvgl_set_text_color,
    lvgl_set_bg_color,    lvgl_set_bg_gradient,   lvgl_set_text_align,
    lvgl_clear_container, lvgl_get_height,        nullptr};

static RenderResult download_html_callback(const char * /*url*/,
                                           MemoryBuffer * /*buffer*/) {
  return RENDER_ERROR_NETWORK;
}

static void main_loop() {
  double now = emscripten_get_now();
  if (last_frame_time_ms == 0.0) {
    last_frame_time_ms = now;
  }
  double delta = now - last_frame_time_ms;
  last_frame_time_ms = now;
  lv_tick_inc(static_cast<uint32_t>(delta > 0 ? delta : 16));
  lv_timer_handler();
}

extern "C" {
EMSCRIPTEN_KEEPALIVE
void load_url(const char *url) {
  if (!url)
    return;

  std::string resolved = resolve_relative_url(url, current_url);
  if (resolved.empty()) {
    update_js_status("Unable to resolve relative path", "#FF6B6B");
    return;
  }

  strncpy(current_url, resolved.c_str(), sizeof(current_url) - 1);
  current_url[sizeof(current_url) - 1] = '\0';
  EM_ASM(
      {
        if (window.TactileBrowserWasm &&typeof window.TactileBrowserWasm
                    .fetchAndRender === 'function') {
          window.TactileBrowserWasm.fetchAndRender(UTF8ToString($0));
        }
      },
      current_url);
}

EMSCRIPTEN_KEEPALIVE
void render_html_from_js(const char *url, const char *html) {
  if (!content_area || !url || !html) {
    show_message("WASM pipeline not ready.", lv_color_hex(0xFF6B6B));
    update_js_status("Renderer unavailable", "#FF6B6B");
    return;
  }

  size_t html_len = strlen(html);
  if (html_len == 0) {
    show_message("No HTML received.", lv_color_hex(0xFF6B6B));
    update_js_status("Empty response", "#FF6B6B");
    return;
  }

  strncpy(current_url, url, sizeof(current_url) - 1);
  current_url[sizeof(current_url) - 1] = '\0';

  lv_obj_clean(content_area);
  RenderResult result = tactilebrowser_render_html_string(
      current_url, html, html_len, content_area, SCREEN_WIDTH - 32,
      SCREEN_HEIGHT - 80);

  if (result == RENDER_SUCCESS) {
    update_js_status("Loaded", "#4CAF50");
  } else {
    show_message("Failed to render content.", lv_color_hex(0xFF6B6B));
    update_js_status("Render failed", "#FF6B6B");
  }
}

EMSCRIPTEN_KEEPALIVE
void wasm_display_message(const char *text, uint32_t color_hex) {
  lv_color_t color = lv_color_hex(color_hex == 0 ? 0xFFD93D : color_hex);
  show_message(text ? text : "", color);
}

EMSCRIPTEN_KEEPALIVE
void start_app() { update_js_status("Ready", "#FFD93D"); }
}

int main() {
  if (!tactilebrowser_core_init()) {
    printf("Failed to init core\n");
    return 1;
  }

  lv_init();

  display = lv_sdl_window_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  if (!display) {
    printf("Failed to create SDL window\n");
    return 1;
  }

  mouse_indev = lv_sdl_mouse_create();
  keyboard_indev = lv_sdl_keyboard_create();
  wheel_indev = lv_sdl_mousewheel_create();

  tactilebrowser_set_renderer(&renderer_iface);
  tactilebrowser_set_html_downloader(download_html_callback);

  content_area = lv_obj_create(lv_screen_active());
  lv_obj_set_size(content_area, SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_obj_set_style_bg_color(content_area, lv_color_hex(0x0D1117), 0);
  lv_obj_set_style_border_width(content_area, 0, 0);
  lv_obj_set_style_radius(content_area, 0, 0);
  lv_obj_set_style_pad_all(content_area, 16, 0);
  lv_obj_set_scroll_dir(content_area, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content_area, LV_SCROLLBAR_MODE_ACTIVE);

  show_message("Use the address bar to load a page.", lv_color_hex(0xFFD93D));
  update_js_status("Booted", "#FFD93D");

  emscripten_set_main_loop(main_loop, 0, 1);
  return 0;
}