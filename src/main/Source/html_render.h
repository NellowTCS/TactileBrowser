#ifndef HTML_RENDER_H
#define HTML_RENDER_H

#include <lvgl.h>
#include <tt_app_manifest.h>

void render_html_to_lvgl(AppHandle app, lv_obj_t* parent, const char* html);

#endif
