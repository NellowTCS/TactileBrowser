#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <SDL2/SDL.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>
#include <lvgl.h>

#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define MAX_TABS 10
#define MAX_URL_LENGTH 512

typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

typedef struct {
    char url[MAX_URL_LENGTH];
    lv_obj_t *content_area;
    lv_obj_t *scroll_container;
} Tab;

// Global variables
static Tab tabs[MAX_TABS];
static int tab_count = 1;
static int active_tab = 0;
static lv_obj_t *address_bar;
static lv_obj_t *tabview;
static lv_indev_t *mouse_indev, *kb_indev, *wheel_indev;
static lv_group_t *input_group;

// CURL callback - renamed to avoid conflict
static size_t http_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    MemoryBuffer *mem = (MemoryBuffer *)userp;
    
    char *ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) {
        fprintf(stderr, "Memory reallocation failed\n");
        return 0;
    }
    
    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;
    return real_size;
}

// Download HTML content
char *download_html(const char *url) {
    CURL *curl = curl_easy_init();
    MemoryBuffer chunk = {0};
    
    if (!curl) {
        fprintf(stderr, "curl_easy_init failed\n");
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, http_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TactileBrowser/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform failed: %s\n", curl_easy_strerror(res));
        free(chunk.data);
        chunk.data = NULL;
    }
    
    curl_easy_cleanup(curl);
    return chunk.data;
}

// Safe string duplication
char* safe_strdup(const char* s) {
    if (!s) return NULL;
    size_t len = strlen(s) + 1;
    char* result = malloc(len);
    if (result) memcpy(result, s, len);
    return result;
}

char* safe_strndup(const char* s, size_t n) {
    if (!s) return NULL;
    size_t len = strlen(s);
    if (n < len) len = n;
    char* result = malloc(len + 1);
    if (result) {
        memcpy(result, s, len);
        result[len] = '\0';
    }
    return result;
}

// Extract title from HTML document
char* extract_title(lxb_html_document_t *document) {
    lxb_dom_element_t *root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (!root) return safe_strdup("Untitled");

    lxb_dom_collection_t *collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 16);
    if (!collection) return safe_strdup("Untitled");

    lxb_status_t status = lxb_dom_elements_by_tag_name(root, collection, 
                                                       (const lxb_char_t *)"title", 5);
    
    if (status != LXB_STATUS_OK || lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return safe_strdup("Untitled");
    }

    lxb_dom_element_t *title_element = lxb_dom_collection_element(collection, 0);
    size_t text_len = 0;
    lxb_char_t *text = lxb_dom_node_text_content(lxb_dom_interface_node(title_element), &text_len);
    
    char *result = (text && text_len > 0) ? 
                   safe_strndup((const char*)text, text_len) : 
                   safe_strdup("Untitled");
    
    if (text) lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
    lxb_dom_collection_destroy(collection, true);
    
    return result ? result : safe_strdup("Untitled");
}

// Render HTML elements to LVGL objects
void render_html_content(lxb_html_document_t *document, lv_obj_t *container) {
    lv_obj_clean(container);
    
    lxb_dom_element_t *root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (!root) return;

    lxb_dom_collection_t *body_collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 4);
    if (!body_collection) return;

    if (lxb_dom_elements_by_tag_name(root, body_collection, (const lxb_char_t *)"body", 4) != LXB_STATUS_OK ||
        lxb_dom_collection_length(body_collection) == 0) {
        lxb_dom_collection_destroy(body_collection, true);
        return;
    }

    lxb_dom_element_t *body = lxb_dom_collection_element(body_collection, 0);
    if (!body) {
        lxb_dom_collection_destroy(body_collection, true);
        return;
    }

    lxb_dom_node_t *node = lxb_dom_interface_node(body)->first_child;
    lv_coord_t y_offset = 10;

    while (node && y_offset < 2000) { // Prevent infinite scrolling
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            const lxb_tag_id_t tag_id = lxb_dom_element_tag_id((lxb_dom_element_t *)node);
            
            if (tag_id == LXB_TAG_P || tag_id == LXB_TAG_H1 || tag_id == LXB_TAG_H2 || 
                tag_id == LXB_TAG_H3 || tag_id == LXB_TAG_A || tag_id == LXB_TAG_DIV) {
                
                size_t text_len = 0;
                lxb_char_t *text = lxb_dom_node_text_content(node, &text_len);
                
                if (text && text_len > 0) {
                    char *str = safe_strndup((const char *)text, text_len);
                    if (str && strlen(str) > 0) {
                        lv_obj_t *label = lv_label_create(container);
                        lv_label_set_text(label, str);
                        lv_obj_set_width(label, SCREEN_WIDTH - 40);
                        lv_label_set_long_mode(label, LV_LABEL_LONG_WRAP);
                        
                        // Style based on tag type - use available font
                        if (tag_id == LXB_TAG_H1 || tag_id == LXB_TAG_H2 || tag_id == LXB_TAG_H3) {
                            lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0); // Use available font
                            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
                            y_offset += 10;
                        } else if (tag_id == LXB_TAG_A) {
                            lv_obj_set_style_text_color(label, lv_color_hex(0x4A90E2), 0);
                        } else {
                            lv_obj_set_style_text_color(label, lv_color_hex(0xE0E0E0), 0);
                        }
                        
                        lv_obj_align(label, LV_ALIGN_TOP_LEFT, 20, y_offset);
                        y_offset += lv_obj_get_height(label) + 10;
                    }
                    free(str);
                    lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
                }
            }
        }
        node = node->next;
    }
    
    lxb_dom_collection_destroy(body_collection, true);
}

// Load URL into specified tab
void load_url(const char *url, int tab_index) {
    if (!url || strlen(url) == 0 || tab_index >= MAX_TABS) return;
    
    // Validate URL format
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        lv_obj_t *error_label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(error_label, "Invalid URL format. Please use http:// or https://");
        lv_obj_center(error_label);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF6B6B), 0);
        return;
    }

    // FIX: Create a temporary buffer to avoid overlap
    char temp_url[MAX_URL_LENGTH];
    strncpy(temp_url, url, MAX_URL_LENGTH - 1);
    temp_url[MAX_URL_LENGTH - 1] = '\0';
    
    // Update tab URL using the temporary buffer
    strncpy(tabs[tab_index].url, temp_url, MAX_URL_LENGTH - 1);
    tabs[tab_index].url[MAX_URL_LENGTH - 1] = '\0';

    // Show loading message
    lv_obj_clean(tabs[tab_index].content_area);
    lv_obj_t *loading_label = lv_label_create(tabs[tab_index].content_area);
    lv_label_set_text(loading_label, "Loading...");
    lv_obj_center(loading_label);
    lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFD93D), 0);

    // Download HTML
    char *html = download_html(temp_url); // Use temp_url instead of url
    if (!html) {
        lv_obj_clean(tabs[tab_index].content_area);
        lv_obj_t *error_label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(error_label, "Failed to load page. Check your connection.");
        lv_obj_center(error_label);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF6B6B), 0);
        return;
    }

    // Parse HTML
    lxb_html_document_t *document = lxb_html_document_create();
    if (!document) {
        free(html);
        return;
    }

    if (lxb_html_document_parse(document, (const lxb_char_t *)html, strlen(html)) != LXB_STATUS_OK) {
        lv_obj_clean(tabs[tab_index].content_area);
        lv_obj_t *error_label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(error_label, "Failed to parse HTML content");
        lv_obj_center(error_label);
        lv_obj_set_style_text_color(error_label, lv_color_hex(0xFF6B6B), 0);
        lxb_html_document_destroy(document);
        free(html);
        return;
    }

    // Extract title and update tab (simplified for space)
    char *title = extract_title(document);
    
    // Render content
    render_html_content(document, tabs[tab_index].content_area);

    // Cleanup
    free(title);
    lxb_html_document_destroy(document);
    free(html);
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
                // In LVGL, check if address bar is focused by checking the current focused object
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
                        char *new_text = malloc(len);
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
    for (int i = 0; i < tab_count; i++) {
        // Tabs are automatically cleaned up by LVGL
    }
    
    lv_group_del(input_group);
    curl_global_cleanup();
    SDL_Quit();
    return 0;
}