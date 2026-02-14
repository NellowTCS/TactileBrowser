#include "lvgl_renderer.h"
#include <lvgl.h>
#include <tactilebrowser_core.h>
#include <tt_app.h>
#include <tt_lvgl_toolbar.h>

// Memory optimization: Static buffers instead of malloc
#define MAX_HTML_SIZE 6144
#define MAX_TEXT_BUFFER 512
#define MAX_STYLE_BUFFER 256
#define MAX_URL_LENGTH 256

// Global app handle
static AppHandle global_app;

// Static buffers for memory efficiency
static char html_buffer[MAX_HTML_SIZE];
static char text_buffer[MAX_TEXT_BUFFER];
static char style_buffer[MAX_STYLE_BUFFER];
static char url_buffer[MAX_URL_LENGTH];

// LVGL renderer instance
static LvglRenderer *lvgl_renderer = NULL;

typedef struct {
  lv_obj_t *addr_bar;
  lv_obj_t *content_cont;
} refs_t;

// Button event callback
static void fetch_btn_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_CLICKED)
    return;

  lv_obj_t *btn = (lv_obj_t *)lv_event_get_target(e);

  // Get associated objects stored in user data
  refs_t *refs = (refs_t *)lv_obj_get_user_data(btn);

  if (!refs || !refs->addr_bar || !refs->content_cont)
    return;

  const char *url = lv_textarea_get_text(refs->addr_bar);
  if (!url || strlen(url) == 0)
    return;

  // Use core library to render
  tactilebrowser_render_url(url, refs->content_cont, lv_pct(100), lv_pct(85));
}

// Address bar event callback
static void addr_bar_event_cb(lv_event_t *e) {
  if (lv_event_get_code(e) != LV_EVENT_READY)
    return;

  lv_obj_t *addr_bar = (lv_obj_t *)lv_event_get_target(e);

  refs_t *refs = (refs_t *)lv_obj_get_user_data(addr_bar);

  if (!refs || !refs->content_cont)
    return;

  const char *url = lv_textarea_get_text(addr_bar);
  if (!url || strlen(url) == 0)
    return;

  // Use core library to render
  tactilebrowser_render_url(url, refs->content_cont, lv_pct(100), lv_pct(85));
}

static void onShow(AppHandle app, void *data, lv_obj_t *parent) {
  global_app = app;

  // Initialize core library if not already done
  if (!lvgl_renderer) {
    tactilebrowser_core_init();
    lvgl_renderer = lvgl_renderer_create();
    tactilebrowser_set_renderer((RenderInterface *)lvgl_renderer);
    tactilebrowser_set_html_downloader(esp32_download_html);
  }

  // Create toolbar
  tt_lvgl_toolbar_create_for_app(parent, app);

  // Address bar
  lv_obj_t *addr_bar = lv_textarea_create(parent);
  lv_obj_set_width(addr_bar, lv_pct(75));
  lv_obj_set_height(addr_bar, 35);
  lv_textarea_set_one_line(addr_bar, true);
  lv_textarea_set_text(addr_bar, "http://example.com");
  lv_textarea_set_placeholder_text(addr_bar, "Enter URL...");
  lv_obj_align(addr_bar, LV_ALIGN_TOP_LEFT, 10, 30);

  // Content container
  lv_obj_t *content_cont = lv_obj_create(parent);
  lv_obj_set_size(content_cont, lv_pct(100), lv_pct(85));
  lv_obj_align(content_cont, LV_ALIGN_TOP_LEFT, 0, 75);
  lv_obj_set_scroll_dir(content_cont, LV_DIR_VER);
  lv_obj_set_style_pad_all(content_cont, 10, 0);
  lv_obj_set_style_bg_color(content_cont, lv_color_make(0xFF, 0xFF, 0xFF), 0);
  lv_obj_set_style_border_width(content_cont, 1, 0);
  lv_obj_set_style_border_color(content_cont, lv_color_hex(0xCCCCCC), 0);

  // Create Go button
  lv_obj_t *fetch_btn = lv_btn_create(parent);
  lv_obj_set_size(fetch_btn, 70, 35);
  lv_obj_align(fetch_btn, LV_ALIGN_TOP_RIGHT, -10, 30);
  lv_obj_set_style_bg_color(fetch_btn, lv_color_hex(0x2196F3), 0);

  lv_obj_t *btn_label = lv_label_create(fetch_btn);
  lv_label_set_text(btn_label, "Go");
  lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
  lv_obj_center(btn_label);

  // Link address bar and content container
  refs_t *refs = (refs_t *)malloc(sizeof(refs_t));
  refs->addr_bar = addr_bar;
  refs->content_cont = content_cont;

  // Store the struct in both button and address bar
  lv_obj_set_user_data(fetch_btn, refs);
  lv_obj_set_user_data(addr_bar, refs);

  // Attach event callbacks
  lv_obj_add_event_cb(fetch_btn, fetch_btn_event_cb, LV_EVENT_CLICKED, NULL);
  lv_obj_add_event_cb(addr_bar, addr_bar_event_cb, LV_EVENT_READY, NULL);

  // Initial page load
  tactilebrowser_render_url("http://example.com", content_cont, lv_pct(100),
                            lv_pct(85));
}

AppRegistration manifest = {
    .onShow = onShow,
};

extern "C" void app_main(void) { tt_app_register(manifest); }
