#include "lvgl_renderer.h"
#include <lvgl.h>
#include <lvgl/widgets/toolbar.h>
#include <tt_app.h>

// Global app handle
static AppHandle global_app;

// The persistent surface (owns the core session). Re-pointed at the content
// container on every show.
static FdmSurface *surface = NULL;

// Widgets recreated on every show
static lv_obj_t *addr_bar = NULL;
static lv_obj_t *content_cont = NULL;

static void toolbar_nav_cb(lv_event_t *event) {
  (void)event;
  tt_app_stop();
}

static void load_url(const char *url) {
  if (!surface || !url || url[0] == '\0')
    return;

  lv_obj_t *container = (lv_obj_t *)surface->platform_data;
  if (!container)
    return;

  int width = lv_obj_get_width(container);
  int height = lv_obj_get_height(container);
  if (width <= 0)
    width = 320;
  if (height <= 0)
    height = 240;

  fdm_render_url(surface, url, width, height);
}

// Called by the core when a link is activated
static void on_link(void *user_data, const char *url) {
  (void)user_data;
  if (!url || url[0] == '\0')
    return;
  if (addr_bar)
    lv_textarea_set_text(addr_bar, url);
  load_url(url);
}

// Forwards container events (tap / drag / scroll) to the core
static void content_event_cb(lv_event_t *event) {
  lvgl_surface_handle_event(surface, event);
}

static void init_timer_cb(lv_timer_t *timer) {
  lv_timer_delete(timer);
  load_url("http://example.com");
}

static void fetch_btn_event_cb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_CLICKED)
    return;
  if (!addr_bar)
    return;
  load_url(lv_textarea_get_text(addr_bar));
}

static void addr_bar_event_cb(lv_event_t *event) {
  if (lv_event_get_code(event) != LV_EVENT_READY)
    return;
  if (!addr_bar)
    return;
  load_url(lv_textarea_get_text(addr_bar));
}

static void onShow(AppHandle app, void *data, lv_obj_t *parent) {
  (void)data;
  global_app = app;

  // One-time core setup
  if (!surface) {
    fdm_init();
    fdm_set_html_downloader(esp32_download_html);
    fdm_set_link_handler(on_link, NULL);
  }

  lv_obj_t *toolbar = lvgl_toolbar_create(parent, "Tactile Browser");
  lvgl_toolbar_set_nav_action(toolbar, LV_SYMBOL_CLOSE, toolbar_nav_cb, NULL);
  const int top_offset = lv_obj_get_height(toolbar);

  // Address bar
  addr_bar = lv_textarea_create(parent);
  lv_obj_set_width(addr_bar, lv_pct(70));
  lv_obj_set_height(addr_bar, 35);
  lv_textarea_set_one_line(addr_bar, true);
  lv_textarea_set_text(addr_bar, "http://example.com");
  lv_textarea_set_placeholder_text(addr_bar, "Enter URL...");
  lv_obj_align(addr_bar, LV_ALIGN_TOP_LEFT, 10, top_offset + 5);

  // Go button
  lv_obj_t *fetch_btn = lv_btn_create(parent);
  lv_obj_set_size(fetch_btn, 70, 35);
  lv_obj_align(fetch_btn, LV_ALIGN_TOP_RIGHT, -10, top_offset + 5);
  lv_obj_set_style_bg_color(fetch_btn, lv_color_hex(0x2196F3), 0);

  lv_obj_t *btn_label = lv_label_create(fetch_btn);
  lv_label_set_text(btn_label, "Go");
  lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
  lv_obj_center(btn_label);

  // Content container. Scrolling is owned by the core, not LVGL.
  content_cont = lv_obj_create(parent);
  lv_obj_set_size(content_cont, lv_pct(100), lv_pct(85));
  lv_obj_align(content_cont, LV_ALIGN_TOP_LEFT, 0, top_offset + 45);
  lv_obj_remove_flag(content_cont, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(content_cont, 0, 0);
  lv_obj_set_style_bg_color(content_cont, lv_color_make(0xFF, 0xFF, 0xFF), 0);
  lv_obj_set_style_border_width(content_cont, 0, 0);

  if (surface) {
    surface->platform_data = content_cont;
  } else {
    surface = lvgl_surface_create(content_cont);
    if (!surface) {
      tt_app_stop();
      return;
    }
  }

  lv_obj_add_event_cb(content_cont, content_event_cb, LV_EVENT_PRESSED, NULL);
  lv_obj_add_event_cb(content_cont, content_event_cb, LV_EVENT_PRESSING, NULL);
  lv_obj_add_event_cb(content_cont, content_event_cb, LV_EVENT_RELEASED, NULL);

  lv_obj_add_event_cb(fetch_btn, fetch_btn_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(addr_bar, addr_bar_event_cb, LV_EVENT_READY, NULL);

  // Load the initial page after the layout has been measured.
  lv_timer_create(init_timer_cb, 200, NULL);
}

AppRegistration manifest = {
    NULL,
    NULL,
    NULL,
    NULL,
    onShow,
    NULL,
    NULL,
};

extern "C" void app_main(void) { tt_app_register(manifest); }
