#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <SDL2/SDL.h>

#include "lexbor/html/html.h"
#include "lexbor/dom/interfaces/document.h"
#include "lexbor/dom/interfaces/element.h"

// LVGL includes
#include "lvgl.h"

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 320

typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;

static size_t my_curl_write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t real_size = size * nmemb;
    MemoryBuffer *mem = (MemoryBuffer *)userp;

    char *ptr = realloc(mem->data, mem->size + real_size + 1);
    if (!ptr) return 0;

    mem->data = ptr;
    memcpy(&(mem->data[mem->size]), contents, real_size);
    mem->size += real_size;
    mem->data[mem->size] = 0;
    return real_size;
}

char *download_html(const char *url) {
    CURL *curl = curl_easy_init();
    MemoryBuffer chunk = {0};

    if (!curl) return NULL;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_curl_write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "TactileBrowser/0.1");

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        free(chunk.data);
        chunk.data = NULL;
    }

    curl_easy_cleanup(curl);
    return chunk.data;
}

char* extract_title(lxb_html_document_t *document) {
    lxb_status_t status;
    
    // Get the document element (root element)
    lxb_dom_element_t *root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (root == NULL) {
        return strdup("No Title");
    }
    
    // Create a collection to hold the results
    lxb_dom_collection_t *collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 16);
    if (collection == NULL) {
        return strdup("No Title");
    }
    
    // Find all title elements
    status = lxb_dom_elements_by_tag_name(root, collection, 
                                         (const lxb_char_t *)"title", 5);
    
    if (status != LXB_STATUS_OK || lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return strdup("No Title");
    }
    
    // Get the first title element
    lxb_dom_element_t *title_element = lxb_dom_collection_element(collection, 0);
    if (title_element == NULL) {
        lxb_dom_collection_destroy(collection, true);
        return strdup("No Title");
    }
    
    // Get the text content of the title element
    size_t text_len = 0;
    lxb_char_t *text = lxb_dom_node_text_content(lxb_dom_interface_node(title_element), &text_len);
    
    char *result;
    if (text != NULL && text_len > 0) {
        result = strndup((const char*)text, text_len);
        // Free the text buffer allocated by lexbor
        lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
    } else {
        result = strdup("No Title");
    }
    
    // Clean up
    lxb_dom_collection_destroy(collection, true);
    
    return result ? result : strdup("No Title");
}

void lv_example_label(const char *title) {
    lv_obj_t *label = lv_label_create(lv_scr_act());
    lv_label_set_text_fmt(label, "Parsed <title>:\n%s", title);
    lv_obj_center(label);
}

int main(void) {
    // Init LVGL
    lv_init();

    // Create display - using correct LVGL v9+ API
    lv_display_t *display = lv_sdl_window_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    if (!display) {
        fprintf(stderr, "Could not create SDL display\n");
        return 1;
    }

    // Create input device for mouse
    lv_indev_t *mouse_indev = lv_sdl_mouse_create();
    if (!mouse_indev) {
        fprintf(stderr, "Could not create SDL mouse input\n");
        return 1;
    }

    // Create input device for keyboard
    lv_indev_t *kb_indev = lv_sdl_keyboard_create();
    if (!kb_indev) {
        fprintf(stderr, "Could not create SDL keyboard input\n");
        return 1;
    }

    // Fetch and parse HTML
    const char *url = "https://example.com";
    char *html = download_html(url);
    if (!html) {
        fprintf(stderr, "Failed to download HTML\n");
        return 1;
    }

    // Parse the HTML document
    lxb_html_document_t *document = lxb_html_document_create();
    if (document == NULL) {
        fprintf(stderr, "Failed to create HTML document\n");
        free(html);
        return 1;
    }

    lxb_status_t status = lxb_html_document_parse(document, (const lxb_char_t *)html, strlen(html));
    if (status != LXB_STATUS_OK) {
        fprintf(stderr, "Failed to parse HTML document\n");
        lxb_html_document_destroy(document);
        free(html);
        return 1;
    }

    // Extract title and display it
    char *title = extract_title(document);
    lv_example_label(title);

    printf("Title: %s\n", title);

    // Clean up
    free(html);
    free(title);
    lxb_html_document_destroy(document);

    // Main loop
    while (1) {
        lv_tick_inc(5);
        lv_timer_handler();
        SDL_Delay(5);
    }

    return 0;
}