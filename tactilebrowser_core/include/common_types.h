#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Common data structures
#ifndef MEMORY_BUFFER_DEFINED
#define MEMORY_BUFFER_DEFINED
// Memory buffer for HTTP data
typedef struct {
    char *data;
    size_t size;
} MemoryBuffer;
#endif

// Color entry for CSS parsing
typedef struct {
    const char* name;
    uint32_t color;
} ColorEntry;

// Forward declarations for platform-specific rendering
typedef struct _Renderer Renderer;
typedef struct _RenderContext RenderContext;

// Abstract rendering interface
typedef struct {
    // Initialize renderer
    bool (*init)(Renderer* renderer);
    // Cleanup renderer
    void (*cleanup)(Renderer* renderer);
    // Create text label
    void* (*create_label)(Renderer* renderer, const char* text, int x, int y);
    // Create button
    void* (*create_button)(Renderer* renderer, const char* text, int x, int y);
    // Register hyperlink handler for interactive widgets
    void (*register_link_handler)(Renderer* renderer, void* widget, const char* url);
    // Create container
    void* (*create_container)(Renderer* renderer, int x, int y, int width, int height);
    // Set text color
    void (*set_text_color)(Renderer* renderer, void* widget, uint32_t color);
    // Set background color
    void (*set_bg_color)(Renderer* renderer, void* widget, uint32_t color);
    // Set text alignment
    void (*set_text_align)(Renderer* renderer, void* widget, int align);
    // Clear container
    void (*clear_container)(Renderer* renderer, void* container);
    // Get widget height
    int (*get_height)(Renderer* renderer, void* widget);
    // Platform-specific data
    void* platform_data;
} RenderInterface;

// Actual Renderer struct definition
struct _Renderer {
    RenderInterface* interface;
    void* platform_data;
};

// Render context for HTML rendering
struct _RenderContext {
    Renderer* renderer;
    void* root_container;
    int current_y;
    int max_width;
    int max_height;
    const char* document_url;
};

// HTML element types
typedef enum {
    ELEMENT_PARAGRAPH,
    ELEMENT_HEADING1,
    ELEMENT_HEADING2,
    ELEMENT_HEADING3,
    ELEMENT_LINK,
    ELEMENT_BUTTON,
    ELEMENT_DIV,
    ELEMENT_SPAN,
    ELEMENT_UNKNOWN
} ElementType;

// CSS property types
typedef enum {
    CSS_COLOR,
    CSS_BACKGROUND_COLOR,
    CSS_TEXT_ALIGN,
    CSS_PADDING,
    CSS_MARGIN,
    CSS_UNKNOWN
} CssProperty;

// Rendering result
typedef enum {
    RENDER_SUCCESS,
    RENDER_ERROR_NETWORK,
    RENDER_ERROR_PARSE,
    RENDER_ERROR_MEMORY,
    RENDER_ERROR_UNKNOWN
} RenderResult;

#ifdef __cplusplus
}
#endif