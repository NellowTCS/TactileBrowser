#pragma once

#include "common_types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// CSS Box Model
typedef struct {
    int top;
    int right;
    int bottom;
    int left;
} BoxSpacing;

// Layout box model with all CSS properties
typedef struct {
    BoxSpacing margin;
    BoxSpacing padding;
    BoxSpacing border;
    
    int width;          // Content width
    int height;         // Content height
    bool width_auto;    // Auto width calculation
    bool height_auto;   // Auto height calculation
    
    // Positioning
    int x;
    int y;
    
    // Display properties
    bool is_block;      // Block vs inline
    bool is_inline_block;
    
    // Text properties
    uint32_t color;
    uint32_t bg_color;
    bool has_explicit_color;
    bool has_explicit_bg_color;
    BackgroundFill background;
    int font_size;
    int line_height;
    int text_align;     // 0=left, 1=center, 2=right
    
    // Overflow handling
    bool scroll_y;
    bool scroll_x;
} LayoutBox;

// Layout node representing an element in the layout tree
typedef struct LayoutNode {
    ElementType type;
    LayoutBox box;
    
    char* text_content;
    char* href;         // For links
    char* href_resolved;
    char* href_path;
    char* form_value;   // For inputs/textarea
    char* placeholder;  // For inputs
    
    void* widget;       // Platform widget
    
    struct LayoutNode* parent;
    struct LayoutNode* first_child;
    struct LayoutNode* next_sibling;
} LayoutNode;

// Layout context for building layout tree
typedef struct {
    LayoutNode* root;
    LayoutNode* current_container;
    
    RenderContext* render_context;
    
    // Current position tracking
    int current_x;
    int current_y;
    int max_line_height;
    int container_width;
    
    // Line breaking for inline elements
    bool needs_line_break;
} LayoutContext;

// Initialize layout engine
bool layout_engine_init(void);

// Cleanup layout engine
void layout_engine_cleanup(void);

// Create a new layout node
LayoutNode* layout_node_create(ElementType type);

// Destroy layout node and its children
void layout_node_destroy(LayoutNode* node);

// Add child node
void layout_node_add_child(LayoutNode* parent, LayoutNode* child);

// Initialize layout context
LayoutContext* layout_context_create(RenderContext* render_ctx);

// Destroy layout context
void layout_context_destroy(LayoutContext* ctx);

// Calculate box dimensions based on content
void layout_calculate_dimensions(LayoutNode* node, int available_width);

// Position node and its children
void layout_position_node(LayoutNode* node, int parent_x, int parent_y);

// Render layout tree to screen
void layout_render_tree(LayoutNode* root, RenderContext* render_ctx);

// Apply CSS properties to layout box
void layout_apply_css_property(LayoutBox* box, const char* property, const char* value);

// Parse CSS spacing (margin, padding, border)
BoxSpacing layout_parse_spacing(const char* value);

// Calculate total width including padding, border, margin
int layout_get_total_width(const LayoutBox* box);

// Calculate total height including padding, border, margin
int layout_get_total_height(const LayoutBox* box);

// Text wrapping utilities
typedef struct {
    char** lines;
    int line_count;
    int* line_widths;
} TextLines;

TextLines* layout_wrap_text(const char* text, int max_width, int font_size);
void layout_free_text_lines(TextLines* lines);

#ifdef __cplusplus
}
#endif
