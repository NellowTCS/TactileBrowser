#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <SDL2/SDL.h>
#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>
#include <lvgl.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320
#define MAX_TABS 10

typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

typedef struct {
    char *url;
    lv_obj_t *content_area; // Container for rendered HTML content
} Tab;

static Tab tabs[MAX_TABS];
static int tab_count = 1;
static int active_tab = 0;
static lv_obj_t *address_bar;
static lv_style_t style_bg, style_surface, style_accent, style_btn, style_label;

// CURL write callback
static size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
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

// Download HTML
char *download_html(const char *url) {
    fprintf(stderr, "Attempting to download: %s\n", url); // Debug URL
    CURL *curl = curl_easy_init();
    MemoryBuffer chunk = {0};
    if (!curl) {
        fprintf(stderr, "curl_easy_init failed\n");
        return NULL;
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TactileBrowser/0.1");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform failed for %s: %s\n", url, curl_easy_strerror(res));
        free(chunk.data);
        chunk.data = NULL;
    }
    curl_easy_cleanup(curl);
    return chunk.data;
}

// String duplication functions
char* my_strdup(const char* s) {
    size_t len = strlen(s) + 1;
    char* result = malloc(len);
    if (result) memcpy(result, s, len);
    return result;
}

char* my_strndup(const char* s, size_t n) {
    size_t len = strlen(s);
    if (n < len) len = n;
    char* result = malloc(len + 1);
    if (result) {
        memcpy(result, s, len);
        result[len] = '\0';
    }
    return result;
}

// Extract HTML title
char* extract_title(lxb_html_document_t *document) {
    lxb_status_t status;
    lxb_dom_element_t *root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (root == NULL) return my_strdup("No Title");
    lxb_dom_collection_t *collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 16);
    if (collection == NULL) return my_strdup("No Title");
    status = lxb_dom_elements_by_tag_name(root, collection, (const lxb_char_t *)"title", 5);
    if (status != LXB_STATUS_OK || lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return my_strdup("No Title");
    }
    lxb_dom_element_t *title_element = lxb_dom_collection_element(collection, 0);
    size_t text_len = 0;
    lxb_char_t *text = lxb_dom_node_text_content(lxb_dom_interface_node(title_element), &text_len);
    char *result = (text && text_len > 0) ? my_strndup((const char*)text, text_len) : my_strdup("No Title");
    if (text) lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
    lxb_dom_collection_destroy(collection, true);
    return result ? result : my_strdup("No Title");
}

// Render HTML body elements
void render_body_elements(lxb_html_document_t *document, lv_obj_t *content_area) {
    lv_obj_clean(content_area); // Clear previous content
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
    while (node) {
        if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            const lxb_tag_id_t tag_id = lxb_dom_element_tag_id((lxb_dom_element_t *)node);
            if (tag_id == LXB_TAG_P || tag_id == LXB_TAG_H1 || tag_id == LXB_TAG_A) {
                size_t text_len = 0;
                lxb_char_t *text = lxb_dom_node_text_content(node, &text_len);
                if (text && text_len > 0) {
                    char *str = my_strndup((const char *)text, text_len);
                    lv_obj_t *label = lv_label_create(content_area);
                    lv_label_set_text(label, str);
                    lv_obj_set_style_text_color(label, lv_color_hex(0xf0f0f0), 0);
                    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, y_offset);
                    y_offset += 30;
                    free(str);
                    lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
                }
            }
        }
        node = node->next;
    }
    lxb_dom_collection_destroy(body_collection, true);
}

// Load and render a URL in the active tab
void load_url(const char *url, int tab_index) {
    fprintf(stderr, "Loading URL: %s\n", url); // Debug URL before processing
    if (!url || strlen(url) == 0 || strncmp(url, "http", 4) != 0) {
        fprintf(stderr, "Invalid URL: %s\n", url ? url : "NULL");
        return;
    }
    free(tabs[tab_index].url);
    tabs[tab_index].url = my_strdup(url);
    char *html = download_html(url);
    if (!html) {
        lv_obj_t *label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(label, "Failed to load page");
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_hex(0xf0f0f0), 0);
        fprintf(stderr, "Download failed for URL: %s\n", url);
        return;
    }
    lxb_html_document_t *document = lxb_html_document_create();
    if (!document) {
        free(html);
        fprintf(stderr, "Failed to create HTML document\n");
        return;
    }
    if (lxb_html_document_parse(document, (const lxb_char_t *)html, strlen(html)) != LXB_STATUS_OK) {
        lv_obj_t *label = lv_label_create(tabs[tab_index].content_area);
        lv_label_set_text(label, "Failed to parse HTML");
        lv_obj_center(label);
        lv_obj_set_style_text_color(label, lv_color_hex(0xf0f0f0), 0);
        lxb_html_document_destroy(document);
        free(html);
        return;
    }
    char *title = extract_title(document);
    render_body_elements(document, tabs[tab_index].content_area);
    lxb_html_document_destroy(document);
    free(html);
    free(title);
}

// Event handler for address bar
static void address_bar_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_VALUE_CHANGED) {
        const char *url = lv_textarea_get_text(address_bar);
        fprintf(stderr, "Address bar changed to: %s\n", url); // Debug
        load_url(url, active_tab);
    }
}

// Event handler for new tab button
static void new_tab_event_cb(lv_event_t *e) {
    if (tab_count < MAX_TABS) {
        lv_obj_t *tabview = lv_event_get_user_data(e);
        tabs[tab_count].url = my_strdup("https://example.com");
        tabs[tab_count].content_area = lv_obj_create(lv_tabview_get_content(tabview));
        lv_obj_set_size(tabs[tab_count].content_area, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 170);
        lv_obj_align(tabs[tab_count].content_area, LV_ALIGN_TOP_LEFT, 10, 10);
        lv_obj_set_style_bg_color(tabs[tab_count].content_area, lv_color_hex(0x2a2a2a), 0);
        lv_tabview_add_tab(tabview, lv_textarea_get_text(address_bar));
        tab_count++;
        fprintf(stderr, "New tab created, count: %d\n", tab_count); // Debug
        load_url(tabs[tab_count - 1].url, tab_count - 1);
    }
}

// Event handler for tab switch
static void tab_switch_event_cb(lv_event_t *e) {
    lv_obj_t *tabview = lv_event_get_target(e);
    active_tab = lv_tabview_get_tab_act(tabview);
    lv_textarea_set_text(address_bar, tabs[active_tab].url);
    fprintf(stderr, "Switched to tab: %d\n", active_tab); // Debug
}

// Initialize styles
void init_styles(void) {
    lv_style_init(&style_bg);
    lv_style_set_bg_color(&style_bg, lv_color_hex(0x1c1b1a));

    lv_style_init(&style_surface);
    lv_style_set_bg_color(&style_surface, lv_color_hex(0x2a2a2a));
    lv_style_set_border_width(&style_surface, 0);
    lv_style_set_radius(&style_surface, 12);

    lv_style_init(&style_accent);
    lv_style_set_bg_color(&style_accent, lv_color_hex(0x4a90e2));
    lv_style_set_text_color(&style_accent, lv_color_hex(0xffffff));

    lv_style_init(&style_btn);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x2a2a2a));
    lv_style_set_text_color(&style_btn, lv_color_hex(0xf0f0f0));
    lv_style_set_radius(&style_btn, 12);
    lv_style_set_border_width(&style_btn, 0);

    lv_style_init(&style_label);
    lv_style_set_text_color(&style_label, lv_color_hex(0xf0f0f0));
}

// Main function
int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 1;
    }

    lv_init();
    lv_display_t *display = lv_sdl_window_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display) {
        fprintf(stderr, "Could not create SDL display\n");
        SDL_Quit();
        return 1;
    }

    lv_indev_t *mouse_indev = lv_sdl_mouse_create();
    if (!mouse_indev) {
        fprintf(stderr, "Could not create SDL mouse input\n");
        SDL_Quit();
        return 1;
    }

    lv_indev_t *kb_indev = lv_sdl_keyboard_create();
    if (!kb_indev) {
        fprintf(stderr, "Could not create SDL keyboard input\n");
        SDL_Quit();
        return 1;
    }

    // Create input group for event handling
    lv_group_t *group = lv_group_create();
    lv_indev_set_group(mouse_indev, group);
    lv_indev_set_group(kb_indev, group);

    curl_global_init(CURL_GLOBAL_DEFAULT);
    init_styles();

    // Create main container with flex layout
    lv_obj_t *main_container = lv_obj_create(lv_screen_active());
    lv_obj_set_size(main_container, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_layout(main_container, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(main_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_bg_color(main_container, lv_color_hex(0x1c1b1a), 0);

    // Title bar
    lv_obj_t *title_bar = lv_obj_create(main_container);
    lv_obj_set_size(title_bar, SCREEN_WIDTH, 30);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0x1c1b1a), 0);
    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "Tactile Browser");
    lv_obj_center(title_label);
    lv_obj_set_style_text_color(title_label, lv_color_hex(0xf0f0f0), 0);

    // Tabview with bottom tab bar (adjusted via size)
    lv_obj_t *tabview = lv_tabview_create(main_container);
    lv_obj_set_size(tabview, SCREEN_WIDTH, SCREEN_HEIGHT - 130); // Adjusted for title, nav, and bookmarks
    lv_obj_set_style_bg_color(tabview, lv_color_hex(0x2a2a2a), 0);
    lv_obj_add_event_cb(tabview, tab_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    tabs[0].content_area = lv_obj_create(lv_tabview_get_content(tabview));
    lv_obj_set_size(tabs[0].content_area, SCREEN_WIDTH - 20, SCREEN_HEIGHT - 170);
    lv_obj_align(tabs[0].content_area, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_style_bg_color(tabs[0].content_area, lv_color_hex(0x2a2a2a), 0);
    lv_tabview_add_tab(tabview, "Home");

    // Navigation bar
    lv_obj_t *nav_bar = lv_obj_create(main_container);
    lv_obj_set_size(nav_bar, SCREEN_WIDTH, 40);
    lv_obj_set_style_bg_color(nav_bar, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_layout(nav_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(nav_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(nav_bar, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Navigation buttons
    lv_obj_t *btn_back = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_back, 30, 30);
    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "←");
    lv_obj_center(label_back);
    lv_obj_set_style_bg_color(btn_back, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(label_back, lv_color_hex(0xf0f0f0), 0);

    lv_obj_t *btn_forward = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_forward, 30, 30);
    lv_obj_t *label_forward = lv_label_create(btn_forward);
    lv_label_set_text(label_forward, "→");
    lv_obj_center(label_forward);
    lv_obj_set_style_bg_color(btn_forward, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(label_forward, lv_color_hex(0xf0f0f0), 0);

    lv_obj_t *btn_refresh = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_refresh, 30, 30);
    lv_obj_t *label_refresh = lv_label_create(btn_refresh);
    lv_label_set_text(label_refresh, "⟳");
    lv_obj_center(label_refresh);
    lv_obj_set_style_bg_color(btn_refresh, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(label_refresh, lv_color_hex(0xf0f0f0), 0);

    lv_obj_t *btn_new_tab = lv_btn_create(nav_bar);
    lv_obj_set_size(btn_new_tab, 30, 30);
    lv_obj_t *label_new_tab = lv_label_create(btn_new_tab);
    lv_label_set_text(label_new_tab, "+");
    lv_obj_center(label_new_tab);
    lv_obj_set_style_bg_color(btn_new_tab, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(label_new_tab, lv_color_hex(0xf0f0f0), 0);
    lv_obj_add_event_cb(btn_new_tab, new_tab_event_cb, LV_EVENT_CLICKED, tabview);
    lv_group_add_obj(group, btn_new_tab); // Add to input group

    // Address bar
    address_bar = lv_textarea_create(nav_bar);
    lv_textarea_set_one_line(address_bar, true);
    lv_textarea_set_text(address_bar, "https://example.com");
    lv_obj_set_size(address_bar, SCREEN_WIDTH - 170, 30);
    lv_obj_set_style_bg_color(address_bar, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(address_bar, lv_color_hex(0xf0f0f0), 0);
    lv_obj_add_event_cb(address_bar, address_bar_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_group_add_obj(group, address_bar); // Add to input group

    // Dropdown menu
    lv_obj_t *menu_btn = lv_btn_create(nav_bar);
    lv_obj_set_size(menu_btn, 30, 30);
    lv_obj_t *label_menu = lv_label_create(menu_btn);
    lv_label_set_text(label_menu, "☰");
    lv_obj_center(label_menu);
    lv_obj_set_style_bg_color(menu_btn, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(label_menu, lv_color_hex(0xf0f0f0), 0);

    lv_obj_t *dropdown = lv_dropdown_create(main_container);
    lv_dropdown_set_options(dropdown, "Settings\nAbout");
    lv_obj_align_to(dropdown, menu_btn, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 5);
    lv_obj_set_style_bg_color(dropdown, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_style_text_color(dropdown, lv_color_hex(0xf0f0f0), 0);
    lv_obj_add_flag(dropdown, LV_OBJ_FLAG_HIDDEN);

    // Bookmarks
    lv_obj_t *bookmarks = lv_obj_create(main_container);
    lv_obj_set_size(bookmarks, SCREEN_WIDTH, 30);
    lv_obj_set_style_bg_color(bookmarks, lv_color_hex(0x2a2a2a), 0);
    lv_obj_set_layout(bookmarks, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bookmarks, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bookmarks, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    const char *bookmark_names[] = {"GitHub", "YouTube", "MDN"};
    for (int i = 0; i < 3; i++) {
        lv_obj_t *bookmark = lv_btn_create(bookmarks);
        lv_obj_set_size(bookmark, 60, 25);
        lv_obj_t *label = lv_label_create(bookmark);
        lv_label_set_text(label, bookmark_names[i]);
        lv_obj_center(label);
        lv_obj_set_style_bg_color(bookmark, lv_color_hex(0x2a2a2a), 0);
        lv_obj_set_style_text_color(label, lv_color_hex(0xf0f0f0), 0);
    }

    // Load initial page
    tabs[0].url = my_strdup("https://example.com");
    fprintf(stderr, "Initial URL set to: %s\n", tabs[0].url); // Debug initial URL
    load_url(tabs[0].url, 0);

    // Event loop
    SDL_Event event;
    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            lv_indev_t *indev = lv_indev_get_next(NULL);
            if (indev) {
                fprintf(stderr, "Input device active\n"); // Debug input
            }
        }
        lv_timer_handler();
        SDL_Delay(5);
    }

    // Cleanup
    for (int i = 0; i < tab_count; i++) {
        free(tabs[i].url);
    }
    curl_global_cleanup();
    SDL_Quit();
    return 0;
}