#include "lvgl_renderer.h"
#include <esp_http_client.h>
#include <lvgl/fonts.h>
#include <stdlib.h>
#include <string.h>

// Immediate-mode canvas renderer using LVGL 9.4 canvas buffer & draw layer API.
// No widgets (labels, buttons, etc.) are created per DOM element; instead,
// all rectangles and text are drawn directly into an immediate-mode draw buffer.

typedef struct {
  lv_obj_t *canvas;
  lv_draw_buf_t *draw_buf;
  lv_layer_t layer;
  int width;
  int height;
} LvglCanvasSurface;

typedef struct {
  bool pressed;
  bool scrolling;
  int32_t start_x;
  int32_t start_y;
  int32_t last_x;
  int32_t last_y;
} LvglPointerState;

// Tactility OS handles font management via its lvgl-module.
// We map font sizes to the OS's font abstractions to avoid missing symbols.
static const lv_font_t *lvgl_font_for_size(int font_size) {
  if (font_size <= 12) {
    return lvgl_get_text_font(FONT_SIZE_SMALL);
  } else if (font_size <= 18) {
    return lvgl_get_text_font(FONT_SIZE_DEFAULT);
  } else {
    return lvgl_get_text_font(FONT_SIZE_LARGE);
  }
}

static void lvgl_begin_frame(FdmSurface *surface, int width, int height) {
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (!cs)
    return;

  if (width <= 0)
    width = 320;
  if (height <= 0)
    height = 240;

  if (!cs->draw_buf || cs->width != width || cs->height != height) {
    if (cs->draw_buf) {
      lv_draw_buf_destroy(cs->draw_buf);
    }
    cs->width = width;
    cs->height = height;
    cs->draw_buf = lv_draw_buf_create((uint32_t)width, (uint32_t)height,
                                      LV_COLOR_FORMAT_ARGB8888, LV_STRIDE_AUTO);
    if (cs->draw_buf) {
      lv_canvas_set_draw_buf(cs->canvas, cs->draw_buf);
    }
    lv_obj_set_size(cs->canvas, width, height);
  }

  if (cs->draw_buf) {
    lv_canvas_init_layer(cs->canvas, &cs->layer);
    // Clear canvas background to white
    lv_draw_rect_dsc_t bg_dsc;
    lv_draw_rect_dsc_init(&bg_dsc);
    bg_dsc.bg_color = lv_color_white();
    bg_dsc.bg_opa = LV_OPA_COVER;
    lv_area_t bg_area = {0, 0, (int32_t)(width - 1), (int32_t)(height - 1)};
    lv_draw_rect(&cs->layer, &bg_dsc, &bg_area);
  }
}

static void lvgl_end_frame(FdmSurface *surface) {
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (!cs || !cs->draw_buf)
    return;
  lv_canvas_finish_layer(cs->canvas, &cs->layer);
  lv_obj_invalidate(cs->canvas);
}

static void lvgl_fill_rect(FdmSurface *surface, int x, int y, int w, int h,
                           FdmColor color) {
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (!cs || !cs->draw_buf || w <= 0 || h <= 0)
    return;

  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = lv_color_hex(color);
  dsc.bg_opa = LV_OPA_COVER;
  lv_area_t area = {
      (int32_t)x,
      (int32_t)y,
      (int32_t)(x + w - 1),
      (int32_t)(y + h - 1),
  };
  lv_draw_rect(&cs->layer, &dsc, &area);
}

static lv_grad_dir_t lvgl_gradient_dir(float angle_deg) {
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

static void lvgl_fill_rect_gradient(FdmSurface *surface, int x, int y, int w,
                                    int h,
                                    const FdmLinearGradient *gradient) {
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (!cs || !cs->draw_buf || !gradient || gradient->stop_count == 0 || w <= 0 ||
      h <= 0)
    return;

  FdmColor start = gradient->stops[0].color;
  FdmColor end = gradient->stops[gradient->stop_count - 1].color;

  lv_draw_rect_dsc_t dsc;
  lv_draw_rect_dsc_init(&dsc);
  dsc.bg_color = lv_color_hex(start);
  dsc.bg_opa = LV_OPA_COVER;

  dsc.bg_grad.stops[0].color = lv_color_hex(start);
  dsc.bg_grad.stops[0].opa = LV_OPA_COVER;
  dsc.bg_grad.stops[0].frac = 0;
  dsc.bg_grad.stops[1].color = lv_color_hex(end);
  dsc.bg_grad.stops[1].opa = LV_OPA_COVER;
  dsc.bg_grad.stops[1].frac = 255;
  dsc.bg_grad.stops_count = 2;
  dsc.bg_grad.dir = lvgl_gradient_dir(gradient->angle_deg);

  lv_area_t area = {
      (int32_t)x,
      (int32_t)y,
      (int32_t)(x + w - 1),
      (int32_t)(y + h - 1),
  };
  lv_draw_rect(&cs->layer, &dsc, &area);
}

static void lvgl_draw_text(FdmSurface *surface, const char *text, int x, int y,
                           int max_width, int font_size, FdmColor color,
                           int align, bool underline) {
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (!cs || !cs->draw_buf || !text || text[0] == '\0')
    return;

  lv_draw_label_dsc_t dsc;
  lv_draw_label_dsc_init(&dsc);
  dsc.text = text;
  dsc.font = lvgl_font_for_size(font_size);
  dsc.color = lv_color_hex(color);

  if (align == FDM_ALIGN_CENTER)
    dsc.align = LV_TEXT_ALIGN_CENTER;
  else if (align == FDM_ALIGN_RIGHT)
    dsc.align = LV_TEXT_ALIGN_RIGHT;
  else
    dsc.align = LV_TEXT_ALIGN_LEFT;

  if (underline)
    dsc.decor = LV_TEXT_DECOR_UNDERLINE;

  int w = max_width > 0 ? max_width : 2000;
  int h = font_size + 8;
  lv_area_t area = {
      (int32_t)x,
      (int32_t)y,
      (int32_t)(x + w - 1),
      (int32_t)(y + h - 1),
  };
  lv_draw_label(&cs->layer, &dsc, &area);
}

static int lvgl_measure_text(FdmSurface *surface, const char *text,
                             int font_size) {
  (void)surface;
  if (!text || text[0] == '\0')
    return 0;
  const lv_font_t *font = lvgl_font_for_size(font_size);
  lv_point_t size = {0, 0};
  lv_text_get_size(&size, text, font, 0, 0, LV_COORD_MAX, LV_TEXT_FLAG_NONE);
  return (int)size.x;
}

static const FdmSurfaceOps lvgl_surface_ops = {
    lvgl_begin_frame,
    lvgl_end_frame,
    lvgl_fill_rect,
    lvgl_fill_rect_gradient,
    lvgl_draw_text,
    lvgl_measure_text,
};

FdmSurface *lvgl_surface_create(lv_obj_t *parent) {
  if (!parent)
    return NULL;

  FdmSurface *surface = (FdmSurface *)malloc(sizeof(FdmSurface));
  LvglCanvasSurface *cs =
      (LvglCanvasSurface *)malloc(sizeof(LvglCanvasSurface));
  LvglPointerState *state = (LvglPointerState *)malloc(sizeof(LvglPointerState));

  if (!surface || !cs || !state) {
    free(surface);
    free(cs);
    free(state);
    return NULL;
  }

  memset(cs, 0, sizeof(*cs));
  memset(state, 0, sizeof(*state));

  cs->canvas = lv_canvas_create(parent);
  lv_obj_add_flag(cs->canvas, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_remove_flag(cs->canvas, LV_OBJ_FLAG_SCROLLABLE);

  surface->ops = &lvgl_surface_ops;
  surface->platform_data = cs;
  surface->user_data = state;
  return surface;
}

void lvgl_surface_destroy(FdmSurface *surface) {
  if (!surface)
    return;
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (cs) {
    if (cs->draw_buf) {
      lv_draw_buf_destroy(cs->draw_buf);
    }
    free(cs);
  }
  free(surface->user_data);
  free(surface);
}

static void lvgl_surface_container_origin(const lv_obj_t *container,
                                          int32_t *origin_x,
                                          int32_t *origin_y) {
  int32_t x = 0;
  int32_t y = 0;
  for (const lv_obj_t *obj = container; obj != NULL;
       obj = lv_obj_get_parent(obj)) {
    x += lv_obj_get_x(obj);
    y += lv_obj_get_y(obj);
  }
  *origin_x = x;
  *origin_y = y;
}

void lvgl_surface_handle_event(FdmSurface *surface, lv_event_t *event) {
  if (!surface)
    return;
  LvglCanvasSurface *cs = (LvglCanvasSurface *)surface->platform_data;
  if (!cs || !cs->canvas)
    return;

  lv_indev_t *indev = lv_indev_active();
  if (!indev)
    return;
  lv_point_t point;
  lv_indev_get_point(indev, &point);

  int32_t origin_x = 0;
  int32_t origin_y = 0;
  lvgl_surface_container_origin(cs->canvas, &origin_x, &origin_y);
  const int32_t local_x = point.x - origin_x;
  const int32_t local_y = point.y - origin_y;

  LvglPointerState *state = (LvglPointerState *)surface->user_data;
  if (!state)
    return;

  lv_event_code_t code = lv_event_get_code(event);
  switch (code) {
  case LV_EVENT_PRESSED:
    state->pressed = true;
    state->scrolling = false;
    state->start_x = local_x;
    state->start_y = local_y;
    state->last_x = local_x;
    state->last_y = local_y;
    fdm_handle_pointer(surface, local_x, local_y, FDM_POINTER_DOWN);
    break;
  case LV_EVENT_PRESSING: {
    if (!state->pressed)
      break;
    const int32_t dy = local_y - state->last_y;
    state->last_x = local_x;
    state->last_y = local_y;

    if (!state->scrolling) {
      const int32_t total_dx = local_x - state->start_x;
      const int32_t total_dy = local_y - state->start_y;
      const int32_t threshold = 12;
      const bool vertical_drag =
          (total_dy > threshold && total_dy > abs(total_dx)) ||
          (total_dy < -threshold && -total_dy > abs(total_dx));
      if (vertical_drag)
        state->scrolling = true;
    }

    if (state->scrolling) {
      fdm_scroll_by(surface, -dy);
    } else {
      fdm_handle_pointer(surface, local_x, local_y, FDM_POINTER_MOVE);
    }
    break;
  }
  case LV_EVENT_RELEASED:
    if (!state->pressed)
      break;
    if (!state->scrolling) {
      fdm_handle_pointer(surface, local_x, local_y, FDM_POINTER_UP);
    }
    state->pressed = false;
    state->scrolling = false;
    break;
  default:
    break;
  }
}

// ESP32 HTTP downloader
FdmResult esp32_download_html(const char *url, FdmBuffer *buffer) {
  if (!url || !buffer)
    return FDM_ERR_UNKNOWN;

  esp_http_client_config_t cfg = {};
  cfg.url = url;
  cfg.timeout_ms = 8000;
  cfg.buffer_size = 1024;
  cfg.buffer_size_tx = 512;
  cfg.user_agent = "TactileBrowser/1.0";

  esp_http_client_handle_t client = esp_http_client_init(&cfg);
  if (!client)
    return FDM_ERR_NETWORK;

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    esp_http_client_cleanup(client);
    return FDM_ERR_NETWORK;
  }

  int content_length = esp_http_client_fetch_headers(client);
  int status_code = esp_http_client_get_status_code(client);

  if (status_code != 200) {
    esp_http_client_cleanup(client);
    return FDM_ERR_NETWORK;
  }

  const int max_html_size = 6144;
  if (content_length <= 0 || content_length > max_html_size) {
    content_length = max_html_size;
  }

  buffer->data = (char *)malloc((size_t)content_length + 1);
  if (!buffer->data) {
    esp_http_client_cleanup(client);
    return FDM_ERR_MEMORY;
  }

  int read_len = esp_http_client_read(client, buffer->data, content_length);
  esp_http_client_cleanup(client);

  if (read_len <= 0) {
    free(buffer->data);
    buffer->data = NULL;
    buffer->size = 0;
    return FDM_ERR_NETWORK;
  }

  buffer->data[read_len] = 0;
  buffer->size = (size_t)read_len;

  return FDM_OK;
}
