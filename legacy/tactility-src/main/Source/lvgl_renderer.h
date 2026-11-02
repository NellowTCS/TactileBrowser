#pragma once

#include <fdm.h>
#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Create an FdmSurface that paints into the given LVGL container.
// The container is cleared on every begin_frame, so it must not hold any
// other children. Returns NULL on allocation failure.
FdmSurface *lvgl_surface_create(lv_obj_t *container);

// Destroy the surface created by lvgl_surface_create.
void lvgl_surface_destroy(FdmSurface *surface);

// Forward an LVGL event from the surface container to the core:
// a tap becomes pointer down/up (links, inputs), a vertical drag becomes a
// scroll, and a horizontal drag selects text. Attach this callback to
// LV_EVENT_PRESSED, LV_EVENT_PRESSING and LV_EVENT_RELEASED.
void lvgl_surface_handle_event(FdmSurface *surface, lv_event_t *event);

// ESP32 HTTP downloader used by fdm_render_url.
FdmResult esp32_download_html(const char *url, FdmBuffer *buffer);

#ifdef __cplusplus
}
#endif
