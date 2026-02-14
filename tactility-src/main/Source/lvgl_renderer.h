#pragma once

#include <lvgl.h>
#include <tactilebrowser_core.h>

#ifdef __cplusplus
extern "C" {
#endif

// LVGL-specific renderer implementation
typedef struct {
  RenderInterface base;
  // LVGL-specific data can be added here
} LvglRenderer;

// Create LVGL renderer
LvglRenderer *lvgl_renderer_create(void);

// Destroy LVGL renderer
void lvgl_renderer_destroy(LvglRenderer *renderer);

// ESP32 HTTP downloader
RenderResult esp32_download_html(const char *url, MemoryBuffer *buffer);

#ifdef __cplusplus
}
#endif