#include <tt_app_manifest.h>
#include <tt_lvgl_toolbar.h>
#include "lvgl.h"

static lv_obj_t* content_area;

static void tab_event_cb(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_target(e);
    const char* tab_name = lv_label_get_text(lv_obj_get_child(target, 0));
    lv_label_set_text_fmt(content_area, "%s\nThis is the %s tab.", tab_name, tab_name);
}

static void onShow(AppHandle app, void* data, lv_obj_t* parent) {
    lv_obj_t* toolbar = tt_lvgl_toolbar_create_for_app(parent, app);
    lv_obj_align(toolbar, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* layout = lv_obj_create(parent);
    lv_obj_set_size(layout, lv_pct(100), lv_pct(100));
    lv_obj_clear_flag(layout, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_flex_flow(layout, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(layout, 6, 0);

    // Title bar
    lv_obj_t* title = lv_label_create(layout);
    lv_label_set_text(title, "Tactile Browser");
    lv_obj_set_style_text_font(title, LV_FONT_DEFAULT, 0);
    lv_obj_set_style_pad_top(title, 30, 0);

    // Tabs
    lv_obj_t* tabs = lv_obj_create(layout);
    lv_obj_set_height(tabs, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(tabs, LV_FLEX_FLOW_ROW);
    lv_obj_set_scroll_dir(tabs, LV_DIR_HOR);

    lv_obj_t* tab1 = lv_btn_create(tabs);
    lv_obj_set_style_radius(tab1, 10, 0);
    lv_obj_set_style_bg_color(tab1, lv_color_hex(0x4a90e2), 0);
    lv_obj_set_style_text_color(tab1, lv_color_white(), 0);
    lv_obj_add_event_cb(tab1, tab_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t* tab1_label = lv_label_create(tab1);
    lv_label_set_text(tab1_label, "Home");

    // Nav row
    lv_obj_t* nav = lv_obj_create(layout);
    lv_obj_set_width(nav, lv_pct(100));
    lv_obj_set_flex_flow(nav, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_gap(nav, 4, 0);
    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);

    const char* nav_btns[] = { "←", "→", "⟳", "+" };
    for (int i = 0; i < 4; i++) {
        lv_obj_t* btn = lv_btn_create(nav);
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, nav_btns[i]);
    }

    lv_obj_t* address = lv_textarea_create(nav);
    lv_textarea_set_placeholder_text(address, "https://tactilebrowser.local");
    lv_obj_set_flex_grow(address, 1);

    lv_obj_t* menu_btn = lv_btn_create(nav);
    lv_label_set_text(lv_label_create(menu_btn), "☰");

    // Bookmarks
    lv_obj_t* bookmarks = lv_obj_create(layout);
    lv_obj_set_flex_flow(bookmarks, LV_FLEX_FLOW_ROW_WRAP);
    const char* bm[] = { "GitHub", "YouTube", "MDN" };
    for (int i = 0; i < 3; i++) {
        lv_obj_t* b = lv_btn_create(bookmarks);
        lv_label_set_text(lv_label_create(b), bm[i]);
    }

    // Content area
    content_area = lv_label_create(layout);
    lv_label_set_text(content_area, "Welcome to Tactile Browser\nThis is a browser mockup.");
    lv_obj_set_style_text_align(content_area, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_bg_color(content_area, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_pad_all(content_area, 10, 0);
    lv_obj_set_style_radius(content_area, 12, 0);
    lv_obj_set_width(content_area, lv_pct(100));
}

ExternalAppManifest manifest = {
    .name = "Tactile Browser",
    .onShow = onShow
};

int main(int argc, char* argv[]) {
    tt_app_register(&manifest);
    return 0;
}
