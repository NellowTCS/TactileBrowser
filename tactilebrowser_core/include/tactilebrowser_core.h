#pragma once

#include "common_types.h"
#include "dom_renderer.h"
#include "html_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

// Main TactileBrowser Core API

// Initialize the core library
bool tactilebrowser_core_init(void);

// Cleanup the core library
void tactilebrowser_core_cleanup(void);

// Set platform-specific HTML downloader
void tactilebrowser_set_html_downloader(
    RenderResult (*downloader)(const char *url, MemoryBuffer *buffer));

// Set platform-specific renderer
void tactilebrowser_set_renderer(RenderInterface *renderer);

// Render URL to container
RenderResult tactilebrowser_render_url(const char *url, void *container,
                                       int max_width, int max_height);

// Render already-downloaded HTML string to a container
RenderResult tactilebrowser_render_html_string(const char *url,
                                               const char *html, size_t length,
                                               void *container, int max_width,
                                               int max_height);

// Utility functions
void memory_buffer_init(MemoryBuffer *buffer);
void memory_buffer_free(MemoryBuffer *buffer);
char *safe_strdup(const char *str);
char *safe_strndup(const char *str, size_t n);

#ifdef __cplusplus
}
#endif