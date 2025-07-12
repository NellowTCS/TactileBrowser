#include <tt_app_manifest.h>
#include <tt_lvgl_toolbar.h>
#include <lvgl.h>
#include <esp_http_client.h>
#include <lexbor/html/parser.h>
#include <lexbor/html/interface.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/text.h>
#include <lexbor/dom/interfaces/node.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

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

// CSS color table for fast lookup
typedef struct {
    const char* name;
    uint32_t color;
} color_entry_t;

static const color_entry_t color_table[] = {
    {"red", 0xFF0000}, {"green", 0x00FF00}, {"blue", 0x0000FF},
    {"black", 0x000000}, {"white", 0xFFFFFF}, {"gray", 0x808080},
    {"yellow", 0xFFFF00}, {"orange", 0xFFA500}, {"purple", 0x800080}
};

// Forward declarations
static void fetch_and_render(AppHandle app, const char* url, lv_obj_t* parent);
static void render_node_tree(lxb_dom_node_t* node, lv_obj_t* parent, int* y_offset);

// Helper function to get appropriate font based on size
static const lv_font_t* get_font_for_size(int size) {
    // Use only available built-in fonts - only lv_font_montserrat_14 is guaranteed
    if (size >= 16) {
        return &lv_font_montserrat_14; // Use 14 as largest available
    } else if (size >= 14) {
        return &lv_font_montserrat_14;
    } else {
        return &lv_font_montserrat_14; // Use 14 as smallest available too
    }
}

// Optimized CSS parser with color name support
static void apply_inline_style(lv_obj_t* obj, const char* style) {
    if (!style || strlen(style) >= MAX_STYLE_BUFFER) return;
    
    strncpy(style_buffer, style, MAX_STYLE_BUFFER - 1);
    style_buffer[MAX_STYLE_BUFFER - 1] = 0;
    
    char* p = style_buffer;
    char* key_start, *val_start;
    
    while (*p) {
        // Skip whitespace - cast to unsigned char to fix warning
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        
        // Find key
        key_start = p;
        while (*p && *p != ':' && !isspace((unsigned char)*p)) p++;
        if (*p != ':') continue;
        *p++ = 0;
        
        // Skip whitespace after colon
        while (*p && isspace((unsigned char)*p)) p++;
        val_start = p;
        
        // Find value end
        while (*p && *p != ';') p++;
        if (*p == ';') *p++ = 0;
        
        // Remove trailing whitespace from value
        char* val_end = val_start + strlen(val_start) - 1;
        while (val_end > val_start && isspace((unsigned char)*val_end)) *val_end-- = 0;
        
        // Apply styles
        if (strcmp(key_start, "color") == 0) {
            uint32_t color = 0;
            if (val_start[0] == '#') {
                // Hex color - use %lx for uint32_t
                if (sscanf(val_start + 1, "%06lx", &color) == 1) {
                    lv_obj_set_style_text_color(obj, lv_color_hex(color), 0);
                }
            } else {
                // Named color
                for (int i = 0; i < sizeof(color_table)/sizeof(color_table[0]); i++) {
                    if (strcmp(val_start, color_table[i].name) == 0) {
                        lv_obj_set_style_text_color(obj, lv_color_hex(color_table[i].color), 0);
                        break;
                    }
                }
            }
        }
        else if (strcmp(key_start, "background-color") == 0) {
            uint32_t color = 0;
            if (val_start[0] == '#' && sscanf(val_start + 1, "%06lx", &color) == 1) {
                lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
                lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
            }
        }
        else if (strcmp(key_start, "font-size") == 0) {
            int size = atoi(val_start);
            lv_obj_set_style_text_font(obj, get_font_for_size(size), 0);
        }
        else if (strcmp(key_start, "font-weight") == 0) {
            if (strcmp(val_start, "bold") == 0) {
                lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, 0);
            }
        }
        else if (strcmp(key_start, "text-align") == 0) {
            if (strcmp(val_start, "center") == 0) {
                lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
            } else if (strcmp(val_start, "right") == 0) {
                lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_RIGHT, 0);
            }
        }
        else if (strcmp(key_start, "padding") == 0) {
            int pad = atoi(val_start);
            if (pad > 0 && pad <= 20) {
                lv_obj_set_style_pad_all(obj, pad, 0);
            }
        }
        else if (strcmp(key_start, "margin") == 0) {
            int margin = atoi(val_start);
            if (margin > 0 && margin <= 20) {
                lv_obj_set_style_margin_all(obj, margin, 0);
            }
        }
    }
}

// Enhanced element rendering with better HTML support
static lv_obj_t* create_element_widget(lxb_dom_element_t* el, lv_obj_t* parent, const char* tag_name, size_t tag_len) {
    lv_obj_t* widget = NULL;
    
    // Handle different HTML elements
    if (tag_len == 2 && memcmp(tag_name, "h1", 2) == 0) {
        widget = lv_label_create(parent);
        lv_obj_set_style_text_font(widget, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(widget, lv_color_hex(0x000080), 0);
    }
    else if (tag_len == 2 && (memcmp(tag_name, "h2", 2) == 0 || memcmp(tag_name, "h3", 2) == 0)) {
        widget = lv_label_create(parent);
        lv_obj_set_style_text_font(widget, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(widget, lv_color_hex(0x000080), 0);
    }
    else if (tag_len == 1 && memcmp(tag_name, "p", 1) == 0) {
        widget = lv_label_create(parent);
        lv_obj_set_style_margin_bottom(widget, 10, 0);
    }
    else if (tag_len == 1 && memcmp(tag_name, "a", 1) == 0) {
        widget = lv_label_create(parent);
        lv_obj_set_style_text_color(widget, lv_color_hex(0x0000EE), 0);
        lv_obj_set_style_text_decor(widget, LV_TEXT_DECOR_UNDERLINE, 0);
    }
    else if (tag_len == 6 && memcmp(tag_name, "button", 6) == 0) {
        widget = lv_btn_create(parent);
    }
    else if (tag_len == 3 && memcmp(tag_name, "div", 3) == 0) {
        widget = lv_obj_create(parent);
        lv_obj_set_style_border_width(widget, 0, 0);
        lv_obj_set_style_bg_opa(widget, LV_OPA_TRANSP, 0);
    }
    else if (tag_len == 4 && memcmp(tag_name, "span", 4) == 0) {
        widget = lv_label_create(parent);
    }
    else {
        // Default container
        widget = lv_obj_create(parent);
        lv_obj_set_style_border_width(widget, 0, 0);
        lv_obj_set_style_bg_opa(widget, LV_OPA_TRANSP, 0);
    }
    
    if (widget) {
        lv_obj_set_width(widget, lv_pct(100));
        lv_obj_set_height(widget, LV_SIZE_CONTENT);
    }
    
    return widget;
}

// Optimized DOM tree rendering
static void render_node_tree(lxb_dom_node_t* node, lv_obj_t* parent, int* y_offset) {
    if (!node || !parent) return;
    
    lxb_dom_node_t* child = lxb_dom_node_first_child(node);
    
    while (child) {
        lxb_dom_node_type_t node_type = child->type;
        
        if (node_type == LXB_DOM_NODE_TYPE_TEXT) {
            size_t len;
            const lxb_char_t* txt = lxb_dom_node_text_content(child, &len);
            
            if (txt && len > 0 && len < MAX_TEXT_BUFFER) {
                // Use static buffer
                memcpy(text_buffer, txt, len);
                text_buffer[len] = 0;
                
                // Trim whitespace more efficiently - cast to unsigned char
                char* start = text_buffer;
                char* end = text_buffer + len - 1;
                
                while (start <= end && isspace((unsigned char)*start)) start++;
                while (end >= start && isspace((unsigned char)*end)) *end-- = 0;
                
                if (start <= end && *start) {
                    lv_obj_t* lbl = lv_label_create(parent);
                    lv_label_set_text(lbl, start);
                    lv_obj_set_width(lbl, lv_pct(100));
                    lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
                    lv_obj_set_pos(lbl, 0, *y_offset);
                    *y_offset += 25;
                }
            }
        }
        else if (node_type == LXB_DOM_NODE_TYPE_ELEMENT) {
            lxb_dom_element_t* el = lxb_dom_interface_element(child);
            
            // Get element tag name
            size_t tag_len;
            const lxb_char_t* tag_name = lxb_dom_element_local_name(el, &tag_len);
            
            // Create appropriate widget
            lv_obj_t* widget = create_element_widget(el, parent, (const char*)tag_name, tag_len);
            
            if (widget) {
                lv_obj_set_pos(widget, 0, *y_offset);
                
                // Apply inline styles
                size_t attr_len;
                const lxb_char_t* style_attr = lxb_dom_element_get_attribute(el, 
                    (const lxb_char_t*)"style", 5, &attr_len);
                
                if (style_attr && attr_len < MAX_STYLE_BUFFER) {
                    memcpy(style_buffer, style_attr, attr_len);
                    style_buffer[attr_len] = 0;
                    apply_inline_style(widget, style_buffer);
                }
                
                // Handle button text
                if (tag_len == 6 && memcmp(tag_name, "button", 6) == 0) {
                    size_t text_len;
                    const lxb_char_t* btn_text = lxb_dom_node_text_content(child, &text_len);
                    if (btn_text && text_len > 0 && text_len < MAX_TEXT_BUFFER) {
                        memcpy(text_buffer, btn_text, text_len);
                        text_buffer[text_len] = 0;
                        
                        lv_obj_t* btn_label = lv_label_create(widget);
                        lv_label_set_text(btn_label, text_buffer);
                        lv_obj_center(btn_label);
                    }
                }
                
                int child_y = 0;
                render_node_tree(child, widget, &child_y);
                
                *y_offset += lv_obj_get_height(widget) + 5;
            }
        }
        
        child = lxb_dom_node_next(child);
    }
}

// Optimized HTTP fetch with better error handling
static void fetch_and_render(AppHandle app, const char* url, lv_obj_t* parent) {
    if (!url || !parent) return;
    
    // Clear previous content
    lv_obj_clean(parent);
    
    // Show loading indicator
    lv_obj_t* loading_lbl = lv_label_create(parent);
    lv_label_set_text(loading_lbl, "Loading...");
    lv_obj_set_style_text_color(loading_lbl, lv_color_hex(0x808080), 0);
    
    // HTTP client configuration
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 8000,
        .buffer_size = 1024,
        .buffer_size_tx = 512,
        .user_agent = "TactileBrowser/1.0"
    };
    
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        lv_label_set_text(loading_lbl, "HTTP client init failed");
        return;
    }
    
    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        lv_label_set_text(loading_lbl, "Connection failed");
        esp_http_client_cleanup(client);
        return;
    }
    
    int content_length = esp_http_client_fetch_headers(client);
    int status_code = esp_http_client_get_status_code(client);
    
    if (status_code != 200) {
        snprintf(text_buffer, MAX_TEXT_BUFFER, "HTTP Error: %d", status_code);
        lv_label_set_text(loading_lbl, text_buffer);
        esp_http_client_cleanup(client);
        return;
    }
    
    if (content_length <= 0) {
        lv_label_set_text(loading_lbl, "No content received");
        esp_http_client_cleanup(client);
        return;
    }
    
    // Limit content size
    if (content_length > MAX_HTML_SIZE) content_length = MAX_HTML_SIZE;
    
    int read_len = esp_http_client_read(client, html_buffer, content_length);
    esp_http_client_cleanup(client);
    
    if (read_len <= 0) {
        lv_label_set_text(loading_lbl, "Failed to read response");
        return;
    }
    
    html_buffer[read_len] = 0;
    
    // Remove loading indicator
    lv_obj_del(loading_lbl);
    
    // Parse HTML
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) {
        lv_obj_t* err_lbl = lv_label_create(parent);
        lv_label_set_text(err_lbl, "HTML document creation failed");
        return;
    }
    
    lxb_status_t status = lxb_html_document_parse(document, (const lxb_char_t*)html_buffer, read_len);
    if (status != LXB_STATUS_OK) {
        lv_obj_t* err_lbl = lv_label_create(parent);
        lv_label_set_text(err_lbl, "HTML parsing failed");
        lxb_html_document_destroy(document);
        return;
    }
    
    lxb_dom_document_t* dom_document = lxb_dom_interface_document(document);
    if (!dom_document) {
        lv_obj_t* err_lbl = lv_label_create(parent);
        lv_label_set_text(err_lbl, "DOM document interface failed");
        lxb_html_document_destroy(document);
        return;
    }
    
    lxb_dom_element_t* root = lxb_dom_document_element(dom_document);
    if (!root) {
        lv_obj_t* err_lbl = lv_label_create(parent);
        lv_label_set_text(err_lbl, "No root element found");
        lxb_html_document_destroy(document);
        return;
    }
    
    // Find body element
    lxb_dom_node_t* body = NULL;
    lxb_dom_node_t* child = lxb_dom_node_first_child(lxb_dom_interface_node(root));
    while (child) {
        if (child->type == LXB_DOM_NODE_TYPE_ELEMENT) {
            lxb_dom_element_t* elem = lxb_dom_interface_element(child);
            size_t tag_len;
            const lxb_char_t* tag_name = lxb_dom_element_local_name(elem, &tag_len);
            if (tag_name && tag_len == 4 && memcmp(tag_name, "body", 4) == 0) {
                body = child;
                break;
            }
        }
        child = lxb_dom_node_next(child);
    }
    
    if (body) {
        int y_offset = 0;
        render_node_tree(body, parent, &y_offset);
    } else {
        // No body found, try to render the root
        int y_offset = 0;
        render_node_tree(lxb_dom_interface_node(root), parent, &y_offset);
    }
    
    lxb_html_document_destroy(document);
}

// Button event callback
static void fetch_btn_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) != LV_EVENT_CLICKED) return;
    
    lv_obj_t* btn = lv_event_get_target(e);
    lv_obj_t* parent = lv_obj_get_parent(btn);
    
    // Find components
    lv_obj_t* addr_bar = NULL;
    lv_obj_t* content_cont = NULL;
    
    uint32_t child_cnt = lv_obj_get_child_cnt(parent);
    for (uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t* child = lv_obj_get_child(parent, i);
        if (lv_obj_check_type(child, &lv_textarea_class)) {
            addr_bar = child;
        } else if (child != btn && !lv_obj_check_type(child, &lv_label_class)) {
            content_cont = child;
        }
    }
    
    if (!addr_bar || !content_cont) return;
    
    // Get URL and validate
    const char* url = lv_textarea_get_text(addr_bar);
    if (!url || strlen(url) == 0) return;
    
    // Copy URL to buffer for safety
    strncpy(url_buffer, url, MAX_URL_LENGTH - 1);
    url_buffer[MAX_URL_LENGTH - 1] = 0;
    
    // Fetch and render
    fetch_and_render(global_app, url_buffer, content_cont);
}

// Enhanced address bar with history - Fixed event handling
static void addr_bar_event_cb(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_READY) {
        // User pressed Enter - simulate a button click by calling the function directly
        lv_obj_t* addr_bar = lv_event_get_target(e);
        lv_obj_t* parent = lv_obj_get_parent(addr_bar);
        
        // Find the content container and execute fetch directly
        lv_obj_t* content_cont = NULL;
        uint32_t child_cnt = lv_obj_get_child_cnt(parent);
        for (uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t* child = lv_obj_get_child(parent, i);
            if (child != addr_bar && !lv_obj_check_type(child, &lv_button_class) && 
                !lv_obj_check_type(child, &lv_label_class)) {
                content_cont = child;
                break;
            }
        }
        
        if (content_cont) {
            const char* url = lv_textarea_get_text(addr_bar);
            if (url && strlen(url) > 0) {
                strncpy(url_buffer, url, MAX_URL_LENGTH - 1);
                url_buffer[MAX_URL_LENGTH - 1] = 0;
                fetch_and_render(global_app, url_buffer, content_cont);
            }
        }
    }
}

static void onShow(AppHandle app, void* data, lv_obj_t* parent) {
    global_app = app;
    
    // Create toolbar
    tt_lvgl_toolbar_create_for_app(parent, app);
    
    // Address bar with better styling
    lv_obj_t* addr_bar = lv_textarea_create(parent);
    lv_obj_set_width(addr_bar, lv_pct(75));
    lv_obj_set_height(addr_bar, 35);
    lv_textarea_set_one_line(addr_bar, true);
    lv_textarea_set_text(addr_bar, "http://example.com");
    lv_textarea_set_placeholder_text(addr_bar, "Enter URL...");
    lv_obj_align(addr_bar, LV_ALIGN_TOP_LEFT, 10, 30);
    lv_obj_add_event_cb(addr_bar, addr_bar_event_cb, LV_EVENT_READY, NULL);
    
    // Go button with better styling
    lv_obj_t* fetch_btn = lv_btn_create(parent);
    lv_obj_set_size(fetch_btn, 70, 35);
    lv_obj_align(fetch_btn, LV_ALIGN_TOP_RIGHT, -10, 30);
    lv_obj_set_style_bg_color(fetch_btn, lv_color_hex(0x2196F3), 0);
    
    lv_obj_t* btn_label = lv_label_create(fetch_btn);
    lv_label_set_text(btn_label, "Go");
    lv_obj_set_style_text_color(btn_label, lv_color_white(), 0);
    lv_obj_center(btn_label);
    
    lv_obj_add_event_cb(fetch_btn, fetch_btn_event_cb, LV_EVENT_CLICKED, NULL);
    
    // Content container with better scrolling
    lv_obj_t* content_cont = lv_obj_create(parent);
    lv_obj_set_size(content_cont, lv_pct(100), lv_pct(85));
    lv_obj_align(content_cont, LV_ALIGN_TOP_LEFT, 0, 75);
    lv_obj_set_scroll_dir(content_cont, LV_DIR_VER);
    lv_obj_set_style_pad_all(content_cont, 10, 0);
    lv_obj_set_style_bg_color(content_cont, lv_color_white(), 0);
    lv_obj_set_style_border_width(content_cont, 1, 0);
    lv_obj_set_style_border_color(content_cont, lv_color_hex(0xCCCCCC), 0);
    
    // Initial page load
    fetch_and_render(app, "http://example.com", content_cont);
}

ExternalAppManifest manifest = {
    .name = "Tactile Browser",
    .onShow = onShow
};

int main(int argc, char* argv[]) {
    tt_app_register(&manifest);
    return 0;
}