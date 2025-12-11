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

#define TACTILEBROWSER_MAX_GRADIENT_STOPS 4

typedef struct {
    uint32_t color;
    float position; // 0.0 - 1.0 range
} GradientStop;

typedef struct {
    float angle_deg;
    size_t stop_count;
    GradientStop stops[TACTILEBROWSER_MAX_GRADIENT_STOPS];
} LinearGradientFill;

typedef enum {
    BACKGROUND_FILL_NONE = 0,
    BACKGROUND_FILL_SOLID,
    BACKGROUND_FILL_LINEAR_GRADIENT
} BackgroundFillType;

typedef struct {
    BackgroundFillType type;
    union {
        LinearGradientFill linear;
    } data;
} BackgroundFill;

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
    // Create single-line text input
    void* (*create_text_input)(Renderer* renderer, const char* value, const char* placeholder, int x, int y, int width, int height);
    // Create multi-line text area
    void* (*create_text_area)(Renderer* renderer, const char* value, int x, int y, int width, int height);
    // Register hyperlink handler for interactive widgets
    void (*register_link_handler)(Renderer* renderer, void* widget, const char* url);
    // Create container
    void* (*create_container)(Renderer* renderer, int x, int y, int width, int height);
    // Set text color
    void (*set_text_color)(Renderer* renderer, void* widget, uint32_t color);
    // Set background color
    void (*set_bg_color)(Renderer* renderer, void* widget, uint32_t color);
    // Set background gradient fill (optional)
    void (*set_bg_gradient)(Renderer* renderer, void* widget, const LinearGradientFill* gradient);
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
    ELEMENT_HEADING4,
    ELEMENT_HEADING5,
    ELEMENT_HEADING6,
    ELEMENT_LINK,
    ELEMENT_BUTTON,
    ELEMENT_INPUT_TEXT,
    ELEMENT_TEXTAREA,
    ELEMENT_DIV,
    ELEMENT_SPAN,
    ELEMENT_STRONG,
    ELEMENT_EM,
    ELEMENT_BOLD,
    ELEMENT_ITALIC,
    ELEMENT_UNDERLINE,
    ELEMENT_LIST_ITEM,
    ELEMENT_ORDERED_LIST,
    ELEMENT_UNORDERED_LIST,
    ELEMENT_IMAGE,
    ELEMENT_CONTAINER,
    ELEMENT_BREAK,
    ELEMENT_HORIZONTAL_RULE,
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