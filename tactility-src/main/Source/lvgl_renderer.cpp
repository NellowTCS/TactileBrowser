#include "lvgl_renderer.h"
#include <esp_http_client.h>
#include <string.h>
#include <stdlib.h>

// ESP32 HTTP downloader implementation
RenderResult esp32_download_html(const char* url, MemoryBuffer* buffer) {
    if (!url || !buffer) return RENDER_ERROR_UNKNOWN;

    // HTTP client configuration
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.timeout_ms = 8000;
    cfg.buffer_size = 1024;
    cfg.buffer_size_tx = 512;
    cfg.user_agent = "TactileBrowser/1.0";

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return RENDER_ERROR_NETWORK;

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        return RENDER_ERROR_NETWORK;
    }

    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);

    if (status_code != 200) {
        esp_http_client_cleanup(client);
        return RENDER_ERROR_NETWORK;
    }

    if (content_length <= 0 || content_length > 6144) { // MAX_HTML_SIZE
        content_length = 6144;
    }

    buffer->data = (char*)malloc(content_length + 1);
    if (!buffer->data) {
        esp_http_client_cleanup(client);
        return RENDER_ERROR_MEMORY;
    }

    int read_len = esp_http_client_read(client, buffer->data, content_length);
    esp_http_client_cleanup(client);

    if (read_len <= 0) {
        free(buffer->data);
        buffer->data = NULL;
        return RENDER_ERROR_NETWORK;
    }

    buffer->data[read_len] = 0;
    buffer->size = read_len;

    return RENDER_SUCCESS;
}

// LVGL renderer implementation
static bool lvgl_renderer_init(Renderer* renderer) {
    // LVGL-specific initialization if needed
    return true;
}

static void lvgl_renderer_cleanup(Renderer* renderer) {
    // LVGL-specific cleanup if needed
}

static void* lvgl_renderer_create_label(Renderer* renderer, const char* text, int x, int y) {
    lv_obj_t* label = lv_label_create((lv_obj_t*)renderer->platform_data);
    lv_label_set_text(label, text);
    lv_obj_set_pos(label, x, y);
    lv_obj_set_width(label, lv_pct(100));
    lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
    return label;
}

static void* lvgl_renderer_create_button(Renderer* renderer, const char* text, int x, int y) {
    lv_obj_t* btn = lv_btn_create((lv_obj_t*)renderer->platform_data);
    lv_obj_set_pos(btn, x, y);
    lv_obj_set_size(btn, 70, 35);

    lv_obj_t* btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, text);
    lv_obj_center(btn_label);

    return btn;
}

static void* lvgl_renderer_create_container(Renderer* renderer, int x, int y, int width, int height) {
    lv_obj_t* container = lv_obj_create((lv_obj_t*)renderer->platform_data);
    lv_obj_set_pos(container, x, y);
    lv_obj_set_size(container, width, height);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    return container;
}

static void lvgl_renderer_set_text_color(Renderer* renderer, void* widget, uint32_t color) {
    lv_obj_set_style_text_color((lv_obj_t*)widget, lv_color_hex(color), 0);
}

static void lvgl_renderer_set_bg_color(Renderer* renderer, void* widget, uint32_t color) {
    lv_obj_set_style_bg_color((lv_obj_t*)widget, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa((lv_obj_t*)widget, LV_OPA_COVER, 0);
}

static void lvgl_renderer_set_text_align(Renderer* renderer, void* widget, int align) {
    lv_text_align_t lv_align = LV_TEXT_ALIGN_LEFT;
    if (align == 1) lv_align = LV_TEXT_ALIGN_CENTER;
    else if (align == 2) lv_align = LV_TEXT_ALIGN_RIGHT;
    lv_obj_set_style_text_align((lv_obj_t*)widget, lv_align, 0);
}

static void lvgl_renderer_clear_container(Renderer* renderer, void* container) {
    lv_obj_clean((lv_obj_t*)container);
}

static int lvgl_renderer_get_height(Renderer* renderer, void* widget) {
    return lv_obj_get_height((lv_obj_t*)widget);
}

// Create LVGL renderer
LvglRenderer* lvgl_renderer_create(void) {
    LvglRenderer* renderer = (LvglRenderer*)malloc(sizeof(LvglRenderer));
    if (!renderer) return NULL;

    renderer->base.init = lvgl_renderer_init;
    renderer->base.cleanup = lvgl_renderer_cleanup;
    renderer->base.create_label = lvgl_renderer_create_label;
    renderer->base.create_button = lvgl_renderer_create_button;
    renderer->base.register_link_handler = NULL;
    renderer->base.create_container = lvgl_renderer_create_container;
    renderer->base.set_text_color = lvgl_renderer_set_text_color;
    renderer->base.set_bg_color = lvgl_renderer_set_bg_color;
    renderer->base.set_text_align = lvgl_renderer_set_text_align;
    renderer->base.clear_container = lvgl_renderer_clear_container;
    renderer->base.get_height = lvgl_renderer_get_height;
    renderer->base.platform_data = NULL;

    return renderer;
}

// Destroy LVGL renderer
void lvgl_renderer_destroy(LvglRenderer* renderer) {
    if (renderer) {
        free(renderer);
    }
}