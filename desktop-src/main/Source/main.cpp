#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <SDL2/SDL.h>
#include <lvgl.h>
#include <tactilebrowser_core.h>
#include <lv_sdl_keyboard.h>
#include <lv_sdl_mouse.h>
#include <lv_sdl_mousewheel.h>
#include <lv_sdl_window.h>
#include <string>
#include <unordered_map>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_TABS 10
#define MAX_URL_LENGTH 512

void load_url(const char *url, int tab_index);

static std::unordered_map<lv_obj_t*, std::string> g_linkTargets;
static void navigate_to_link(const char* url);
static void lvgl_link_event_cb(lv_event_t *e);

static bool has_http_scheme(const char* url) {
    if (!url) return false;
    return strncmp(url, "http://", 7) == 0 || strncmp(url, "https://", 8) == 0;
}

static std::string extract_origin_from_url(const char* url) {
    if (!url || url[0] == '\0') return "";
    std::string value(url);
    size_t scheme_pos = value.find("://");
    if (scheme_pos == std::string::npos) return "";
    size_t host_start = scheme_pos + 3;
    size_t path_pos = value.find('/', host_start);
    if (path_pos == std::string::npos) {
        return value;
    }
    return value.substr(0, path_pos);
}

static std::string resolve_relative_url(const char* candidate, const char* base_url) {
    if (!candidate || candidate[0] == '\0') return "";
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

// LVGL Renderer implementation for tactilebrowser_core
class LVGLRenderer {
public:
    RenderInterface interface;
    
    LVGLRenderer() {
        // Initialize the interface function pointers
        interface.init = lvgl_init;
        interface.cleanup = lvgl_cleanup;
        interface.create_label = lvgl_create_label;
        interface.create_button = lvgl_create_button;
        interface.create_text_input = lvgl_create_text_input;
        interface.create_text_area = lvgl_create_text_area;
        interface.register_link_handler = lvgl_register_link;
        interface.create_container = lvgl_create_container;
        interface.set_text_color = lvgl_set_text_color;
        interface.set_bg_color = lvgl_set_bg_color;
        interface.set_bg_gradient = lvgl_set_bg_gradient;
        interface.set_text_align = lvgl_set_text_align;
        interface.clear_container = lvgl_clear_container;
        interface.get_height = lvgl_get_height;
        interface.platform_data = nullptr;
    }

private:
    static bool lvgl_init(Renderer* renderer) {
        // LVGL is already initialized in main
        return true;
    }

    static void lvgl_cleanup(Renderer* renderer) {
        // LVGL cleanup handled in main
    }

    static void* lvgl_create_label(Renderer* renderer, const char* text, int x, int y) {
        lv_obj_t* container = (lv_obj_t*)renderer->platform_data;
        lv_obj_t* label = lv_label_create(container);
        lv_label_set_text(label, text);
        lv_obj_set_pos(label, x, y);
        lv_obj_set_width(label, lv_pct(100));
        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(label, lv_color_hex(0xE0E0E0), 0);
        return label;
    }

    static void* lvgl_create_button(Renderer* renderer, const char* text, int x, int y) {
        lv_obj_t* container = (lv_obj_t*)renderer->platform_data;
        lv_obj_t* btn = lv_btn_create(container);
        lv_obj_set_pos(btn, x, y);
        lv_obj_t* label = lv_label_create(btn);
        lv_label_set_text(label, text);
        lv_obj_center(label);
        return btn;
    }

    static void* lvgl_create_text_input(Renderer* renderer,
                                        const char* value,
                                        const char* placeholder,
                                        int x,
                                        int y,
                                        int width,
                                        int height) {
        return lvgl_create_text_widget(renderer, value, placeholder, x, y, width, height, false);
    }

    static void* lvgl_create_text_area(Renderer* renderer,
                                       const char* value,
                                       int x,
                                       int y,
                                       int width,
                                       int height) {
        return lvgl_create_text_widget(renderer, value, nullptr, x, y, width, height, true);
    }

    static void* lvgl_create_container(Renderer* renderer, int x, int y, int width, int height) {
        lv_obj_t* parent = (lv_obj_t*)renderer->platform_data;
        lv_obj_t* container = lv_obj_create(parent);
        lv_obj_set_pos(container, x, y);
        lv_obj_set_size(container, width, height);
        lv_obj_set_style_bg_color(container, lv_color_hex(0x1E1E1E), 0);
        lv_obj_set_style_border_width(container, 0, 0);
        return container;
    }

    static void lvgl_set_text_color(Renderer* renderer, void* widget, uint32_t color) {
        lv_obj_t* obj = (lv_obj_t*)widget;
        lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
    }

    static void lvgl_set_bg_color(Renderer* renderer, void* widget, uint32_t color) {
        lv_obj_t* obj = (lv_obj_t*)widget;
        lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    }

    static void lvgl_set_bg_gradient(Renderer* renderer, void* widget, const LinearGradientFill* gradient) {
        (void)renderer;
        if (!widget || !gradient || gradient->stop_count == 0) return;

        lv_obj_t* obj = static_cast<lv_obj_t*>(widget);
        uint32_t start_color = gradient->stops[0].color;
        uint32_t end_color = gradient->stops[gradient->stop_count - 1].color;
        lv_obj_set_style_bg_color(obj, lv_color_hex(start_color), 0);
        lv_obj_set_style_bg_grad_color(obj, lv_color_hex(end_color), 0);
        lv_obj_set_style_bg_grad_dir(obj, gradient_dir_from_angle(gradient->angle_deg), 0);
        lv_obj_set_style_bg_main_stop(obj, 0, 0);
        lv_obj_set_style_bg_grad_stop(obj, 255, 0);
        lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    }

    static void lvgl_set_text_align(Renderer* renderer, void* widget, int align) {
        lv_obj_t* obj = (lv_obj_t*)widget;
        lv_obj_set_style_text_align(obj, (lv_text_align_t)align, 0);
    }

    static void lvgl_clear_container(Renderer* renderer, void* container) {
        lv_obj_t* obj = (lv_obj_t*)container;
        lv_obj_clean(obj);
    }

    static int lvgl_get_height(Renderer* renderer, void* widget) {
        lv_obj_t* obj = (lv_obj_t*)widget;
        return lv_obj_get_height(obj);
    }

    static void lvgl_register_link(Renderer* renderer, void* widget, const char* url) {
        if (!widget || !url || url[0] == '\0') return;

        lv_obj_t* obj = (lv_obj_t*)widget;
        g_linkTargets[obj] = url;
        lv_obj_add_flag(obj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(obj, lvgl_link_event_cb, LV_EVENT_ALL, NULL);
        lv_obj_set_style_text_color(obj, lv_color_hex(0x4EA1FF), 0);
        lv_obj_set_style_text_decor(obj, LV_TEXT_DECOR_UNDERLINE, 0);
    }

    static lv_obj_t* lvgl_create_text_widget(Renderer* renderer,
                                             const char* value,
                                             const char* placeholder,
                                             int x,
                                             int y,
                                             int width,
                                             int height,
                                             bool multiline) {
        lv_obj_t* container = (lv_obj_t*)renderer->platform_data;
        if (!container) return nullptr;

        lv_obj_t* textarea = lv_textarea_create(container);
        lv_obj_set_pos(textarea, x, y);
        lv_obj_set_size(textarea,
                        width > 0 ? width : (multiline ? 320 : 240),
                        height > 0 ? height : (multiline ? 120 : 40));
        lv_textarea_set_one_line(textarea, !multiline);
        lv_textarea_set_text(textarea, value ? value : "");
        if (placeholder && placeholder[0] != '\0') {
            lv_textarea_set_placeholder_text(textarea, placeholder);
        }
        lv_textarea_set_scrollbar_mode(textarea, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_bg_color(textarea, lv_color_hex(0x151515), 0);
        lv_obj_set_style_bg_opa(textarea, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(textarea, lv_color_hex(0x3A3A3A), 0);
        lv_obj_set_style_border_width(textarea, 1, 0);
        lv_obj_set_style_pad_all(textarea, 6, 0);
        return textarea;
    }

    static lv_grad_dir_t gradient_dir_from_angle(float angle_deg) {
        float normalized = angle_deg;
        while (normalized < 0.0f) normalized += 360.0f;
        while (normalized >= 360.0f) normalized -= 360.0f;

        if ((normalized >= 45.0f && normalized < 135.0f) ||
            (normalized >= 225.0f && normalized < 315.0f)) {
            return LV_GRAD_DIR_HOR;
        }
        return LV_GRAD_DIR_VER;
    }
};

typedef struct {
    char url[MAX_URL_LENGTH];
    lv_obj_t *content_area;
} Tab;

// Global variables
static Tab tabs[MAX_TABS];
static int tab_count = 1;
static int active_tab = 0;
static lv_obj_t *address_bar;
static lv_obj_t *tabview;
static lv_indev_t *mouse_indev, *kb_indev, *wheel_indev;
static lv_group_t *input_group;
static LVGLRenderer* lvgl_renderer = nullptr;
static Renderer renderer_wrapper;

// Forward declaration for CURL callback
static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp);

// CURL callback for tactilebrowser_core
static RenderResult download_html_callback(const char* url, MemoryBuffer* buffer) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return RENDER_ERROR_NETWORK;
    }

    memory_buffer_init(buffer);
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, buffer);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TactileBrowser/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        memory_buffer_free(buffer);
        return RENDER_ERROR_NETWORK;
    }

    return RENDER_SUCCESS;
}

// CURL write callback
static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    MemoryBuffer *mem = (MemoryBuffer *)userp;

    char *ptr = (char*)realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        return 0;
    }

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;
    return real_size;
}

// Load URL into specified tab using tactilebrowser_core
void load_url(const char *url, int tab_index) {
    if (!url || strlen(url) == 0 || tab_index >= MAX_TABS) return;

    std::string resolved = resolve_relative_url(url, tabs[tab_index].url);
    if (resolved.empty()) {
        lv_obj_clean(tabs[tab_index].content_area);
        lv_obj_t *error_label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(error_label, "Unable to resolve relative path without an active site.");
        lv_obj_center(error_label);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF6B6B), 0);
        return;
    }

    const char* final_url = resolved.c_str();

    // Validate URL format
    if (!has_http_scheme(final_url)) {
        lv_obj_clean(tabs[tab_index].content_area);
        lv_obj_t *error_label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(error_label, "Invalid URL format. Please use http(s) links.");
        lv_obj_center(error_label);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF6B6B), 0);
        return;
    }

    // Update tab URL
    strncpy(tabs[tab_index].url, final_url, MAX_URL_LENGTH - 1);
    tabs[tab_index].url[MAX_URL_LENGTH - 1] = '\0';

    // Show loading message
    lv_obj_clean(tabs[tab_index].content_area);
    lv_obj_t *loading_label = lv_label_create(tabs[tab_index].content_area);
    lv_label_set_text(loading_label, "Loading...");
    lv_obj_center(loading_label);
    lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFD93D), 0);

    // Set renderer platform data to current tab's content area
    renderer_wrapper.platform_data = tabs[tab_index].content_area;

    // Use tactilebrowser_core to render the URL
    RenderResult result = tactilebrowser_render_url(final_url, tabs[tab_index].content_area, SCREEN_WIDTH - 40, SCREEN_HEIGHT - 100);

    if (result != RENDER_SUCCESS) {
        lv_obj_clean(tabs[tab_index].content_area);
        lv_obj_t *error_label = lv_label_create(tabs[tab_index].content_area);
        const char* error_msg = "Failed to load page.";
        switch (result) {
            case RENDER_ERROR_NETWORK:
                error_msg = "Network error. Check your connection.";
                break;
            case RENDER_ERROR_PARSE:
                error_msg = "Failed to parse HTML content.";
                break;
            case RENDER_ERROR_MEMORY:
                error_msg = "Memory allocation error.";
                break;
            default:
                break;
        }
        lv_label_set_text(error_label, error_msg);
        lv_obj_center(error_label);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF6B6B), 0);
    }
}

static void navigate_to_link(const char* url) {
    if (!url || url[0] == '\0') return;
    load_url(url, active_tab);
}

static void lvgl_link_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t* target = (lv_obj_t*)lv_event_get_target(e);

    if (code == LV_EVENT_DELETE) {
        g_linkTargets.erase(target);
        return;
    }

    if (code == LV_EVENT_CLICKED) {
        auto it = g_linkTargets.find(target);
        if (it != g_linkTargets.end()) {
            navigate_to_link(it->second.c_str());
        }
    }
}

// Event handlers
static void address_bar_event_cb(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        const char *url = lv_textarea_get_text(address_bar);
        load_url(url, active_tab);
    }
}

static void refresh_event_cb(lv_event_t *e) {
    load_url(tabs[active_tab].url, active_tab);
}

static void new_tab_event_cb(lv_event_t *e) {
    if (tab_count < MAX_TABS) {
        strncpy(tabs[tab_count].url, "https://example.com", MAX_URL_LENGTH - 1);

        lv_obj_t *tab_content = lv_tabview_add_tab(tabview, "New Tab");
        tabs[tab_count].content_area = lv_obj_create(tab_content);
        lv_obj_set_size(tabs[tab_count].content_area, LV_PCT(100), LV_PCT(100));
        lv_obj_set_scrollbar_mode(tabs[tab_count].content_area, LV_SCROLLBAR_MODE_AUTO);
        lv_obj_set_style_bg_color(tabs[tab_count].content_area, lv_color_hex(0x1E1E1E), 0);
        lv_obj_set_style_border_width(tabs[tab_count].content_area, 0, 0);

        tab_count++;
        lv_tabview_set_act(tabview, tab_count - 1, false);
    }
}

static void tab_changed_event_cb(lv_event_t *e) {
    active_tab = lv_tabview_get_tab_act(tabview);
    if (active_tab < tab_count) {
        lv_textarea_set_text(address_bar, tabs[active_tab].url);
    }
}

// Handle Enter key press for address bar
static void trigger_address_bar_load(void) {
    const char *url = lv_textarea_get_text(address_bar);
    load_url(url, active_tab);
}

// Event processing functions for different input types
static void handle_mouse_event(SDL_Event *event) {
    // Handle mouse events manually since lv_sdl_mouse_handler might not be available
    static bool mouse_pressed = false;

    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                mouse_pressed = true;
                lv_indev_data_t data;
                data.point.x = event->button.x;
                data.point.y = event->button.y;
                data.state = LV_INDEV_STATE_PRESSED;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                mouse_pressed = false;
                lv_indev_data_t data;
                data.point.x = event->button.x;
                data.point.y = event->button.y;
                data.state = LV_INDEV_STATE_RELEASED;
            }
            break;
        case SDL_MOUSEMOTION:
            if (mouse_pressed) {
                lv_indev_data_t data;
                data.point.x = event->motion.x;
                data.point.y = event->motion.y;
                data.state = LV_INDEV_STATE_PRESSED;
            }
            break;
    }
}

static void handle_keyboard_event(SDL_Event *event) {
    // Handle keyboard events manually
    if (event->type == SDL_KEYDOWN) {
        SDL_Keycode key = event->key.keysym.sym;

        // Handle special keys
        switch (key) {
            case SDLK_RETURN:
                // Trigger address bar loading if address bar has focus
                if (lv_group_get_focused(input_group) == address_bar) {
                    trigger_address_bar_load();
                }
                break;
            case SDLK_TAB:
                // Switch focus
                lv_group_focus_next(input_group);
                break;
            case SDLK_BACKSPACE:
                // Handle backspace in address bar
                if (lv_group_get_focused(input_group) == address_bar) {
                    const char *current_text = lv_textarea_get_text(address_bar);
                    size_t len = strlen(current_text);
                    if (len > 0) {
                        char *new_text = (char*)malloc(len);
                        if (new_text) {
                            strncpy(new_text, current_text, len - 1);
                            new_text[len - 1] = '\0';
                            lv_textarea_set_text(address_bar, new_text);
                            free(new_text);
                        }
                    }
                }
                break;
        }
    }

    if (event->type == SDL_TEXTINPUT) {
        // Handle text input for address bar
        if (lv_group_get_focused(input_group) == address_bar) {
            const char *text = lv_textarea_get_text(address_bar);
            char new_text[MAX_URL_LENGTH];
            snprintf(new_text, sizeof(new_text), "%s%s", text, event->text.text);
            lv_textarea_set_text(address_bar, new_text);
        }
    }
}

// Initialize the browser UI
void init_browser_ui(void) {
    // Create input group first
    input_group = lv_group_create();

    // Main container
    lv_obj_t *main_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(main_cont, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(main_cont, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_border_width(main_cont, 0, 0);
    lv_obj_set_style_radius(main_cont, 0, 0);
    lv_obj_set_style_pad_all(main_cont, 0, 0);

    // Navigation bar
    lv_obj_t *nav_bar = lv_obj_create(main_cont);
    lv_obj_set_size(nav_bar, SCREEN_WIDTH, 50);
    lv_obj_align(nav_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(nav_bar, lv_color_hex(0x21262D), 0);
    lv_obj_set_style_border_width(nav_bar, 0, 0);
    lv_obj_set_style_radius(nav_bar, 0, 0);

    // Navigation buttons
    lv_obj_t *btn_refresh = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_refresh, 40, 30);
    lv_obj_align(btn_refresh, LV_ALIGN_LEFT_MID, 10, 0);
    lv_obj_t *refresh_label = lv_label_create(btn_refresh);
    lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH);
    lv_obj_center(refresh_label);
    lv_obj_add_event_cb(btn_refresh, refresh_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_new_tab = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_new_tab, 40, 30);
    lv_obj_align(btn_new_tab, LV_ALIGN_LEFT_MID, 60, 0);
    lv_obj_t *new_tab_label = lv_label_create(btn_new_tab);
    lv_label_set_text(new_tab_label, "+");
    lv_obj_center(new_tab_label);
    lv_obj_add_event_cb(btn_new_tab, new_tab_event_cb, LV_EVENT_CLICKED, NULL);

    // Address bar
    address_bar = lv_textarea_create(nav_bar);
    lv_textarea_set_one_line(address_bar, true);
    lv_obj_set_size(address_bar, SCREEN_WIDTH - 200, 30);
    lv_obj_align(address_bar, LV_ALIGN_RIGHT_MID, -10, 0);
    lv_textarea_set_text(address_bar, "https://example.com");
    lv_obj_set_style_bg_color(address_bar, lv_color_hex(0x0D1117), 0);
    lv_obj_set_style_text_color(address_bar, lv_color_hex(0xF0F6FC), 0);
    lv_obj_add_event_cb(address_bar, address_bar_event_cb, LV_EVENT_READY, NULL);

    // Tab view
    tabview = lv_tabview_create(main_cont);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - 50);
    lv_obj_align(tabview, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x0D1117), 0);
    lv_obj_add_event_cb(tabview, tab_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Create first tab
    lv_obj_t *tab1 = lv_tabview_add_tab(tabview, "Home");
    tabs[0].content_area = lv_obj_create(tab1);
    lv_obj_set_size(tabs[0].content_area, LV_PCT(100), LV_PCT(100));
    lv_obj_set_scrollbar_mode(tabs[0].content_area, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_set_style_bg_color(tabs[0].content_area, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(tabs[0].content_area, 0, 0);

    strncpy(tabs[0].url, "https://example.com", MAX_URL_LENGTH - 1);

    // Add input objects to group
    lv_group_add_obj(input_group, address_bar);
    lv_group_add_obj(input_group, btn_refresh);
    lv_group_add_obj(input_group, btn_new_tab);

    // Set keyboard input device to use the group
    if (kb_indev) {
        lv_indev_set_group(kb_indev, input_group);
    }
}

// Main function
int main(void) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL init failed: %s\n", SDL_GetError());
        return 1;
    }

    // Initialize LVGL
    lv_init();

    // Create display
    lv_display_t *display = lv_sdl_window_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display) {
        fprintf(stderr, "Failed to create display\n");
        SDL_Quit();
        return 1;
    }

    // Create input devices
    mouse_indev = lv_sdl_mouse_create();
    kb_indev = lv_sdl_keyboard_create();
    wheel_indev = lv_sdl_mousewheel_create();

    if (!mouse_indev || !kb_indev || !wheel_indev) {
        fprintf(stderr, "Failed to create input devices\n");
        SDL_Quit();
        return 1;
    }

    // Initialize CURL
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // Initialize tactilebrowser_core
    if (!tactilebrowser_core_init()) {
        fprintf(stderr, "Failed to initialize tactilebrowser_core\n");
        curl_global_cleanup();
        SDL_Quit();
        return 1;
    }

    // Set up LVGL renderer
    lvgl_renderer = new LVGLRenderer();
    renderer_wrapper.interface = &lvgl_renderer->interface;
    renderer_wrapper.platform_data = nullptr;

    tactilebrowser_set_renderer(&lvgl_renderer->interface);
    tactilebrowser_set_html_downloader(download_html_callback);

    // Initialize browser UI
    init_browser_ui();

    // Load initial page
    load_url(tabs[0].url, 0);

    // Main event loop
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
                break;
            }

            // Handle different event types with custom functions
            switch (event.type) {
                case SDL_MOUSEMOTION:
                case SDL_MOUSEBUTTONDOWN:
                case SDL_MOUSEBUTTONUP:
                    handle_mouse_event(&event);
                    break;
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                case SDL_TEXTINPUT:
                    handle_keyboard_event(&event);
                    break;
                case SDL_MOUSEWHEEL:
                    // Handle wheel events manually if needed
                    break;
            }
        }

        // Handle LVGL tasks
        lv_timer_handler();
        SDL_Delay(5); // ~200 FPS limit
    }

    // Cleanup
    delete lvgl_renderer;
    tactilebrowser_core_cleanup();
    lv_group_del(input_group);
    curl_global_cleanup();
    SDL_Quit();
    return 0;
}