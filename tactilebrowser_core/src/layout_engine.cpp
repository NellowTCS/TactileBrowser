#include "layout_engine.h"
#include "tactilebrowser_core.h"
#include "css_parser.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

// Default box model values
static const LayoutBox DEFAULT_BOX = {
    .margin = {0, 0, 0, 0},
    .padding = {8, 8, 8, 8},
    .border = {0, 0, 0, 0},
    .width = 0,
    .height = 0,
    .width_auto = true,
    .height_auto = true,
    .x = 0,
    .y = 0,
    .is_block = true,
    .is_inline_block = false,
    .color = 0xFFFFFF,
    .bg_color = 0x000000,
    .has_explicit_color = false,
    .has_explicit_bg_color = false,
    .background = {
        .type = BACKGROUND_FILL_NONE
    },
    .font_size = 14,
    .line_height = 20,
    .text_align = 0,
    .scroll_y = false,
    .scroll_x = false
};

static char* trim_whitespace_inplace(char* str) {
    if (!str) return NULL;
    while (*str && isspace((unsigned char)*str)) {
        str++;
    }
    char* end = str + strlen(str);
    while (end > str && isspace((unsigned char)*(end - 1))) {
        *(--end) = '\0';
    }
    return str;
}

static bool layout_parse_css_color(const char* value, uint32_t* color) {
    if (!value || !color) return false;
    return css_parser_parse_color_value(value, color);
}

static bool parse_angle_keyword(const char* token, float* angle_deg) {
    if (!token || !angle_deg) return false;
    char* copy = safe_strdup(token);
    if (!copy) return false;
    char* trimmed = trim_whitespace_inplace(copy);
    for (char* c = trimmed; *c; ++c) {
        *c = (char)tolower((unsigned char)*c);
    }

    bool handled = false;
    if (strstr(trimmed, "deg")) {
        double value = atof(trimmed);
        *angle_deg = (float)value;
        handled = true;
    } else if (strncmp(trimmed, "to ", 3) == 0) {
        bool to_top = strstr(trimmed, "top") != NULL;
        bool to_bottom = strstr(trimmed, "bottom") != NULL;
        bool to_left = strstr(trimmed, "left") != NULL;
        bool to_right = strstr(trimmed, "right") != NULL;
        if (to_top && to_right) {
            *angle_deg = 45.0f;
            handled = true;
        } else if (to_top && to_left) {
            *angle_deg = 315.0f;
            handled = true;
        } else if (to_bottom && to_right) {
            *angle_deg = 135.0f;
            handled = true;
        } else if (to_bottom && to_left) {
            *angle_deg = 225.0f;
            handled = true;
        } else if (to_top) {
            *angle_deg = 0.0f;
            handled = true;
        } else if (to_right) {
            *angle_deg = 90.0f;
            handled = true;
        } else if (to_bottom) {
            *angle_deg = 180.0f;
            handled = true;
        } else if (to_left) {
            *angle_deg = 270.0f;
            handled = true;
        }
    }

    free(copy);
    return handled;
}

static bool parse_gradient_color_token(const char* token, uint32_t* color) {
    if (!token || !color) return false;
    char* copy = safe_strdup(token);
    if (!copy) return false;
    char* trimmed = trim_whitespace_inplace(copy);
    bool parsed = css_parser_parse_color_value(trimmed, color);
    if (!parsed) {
        char* last_space = strrchr(trimmed, ' ');
        if (last_space) {
            *last_space = '\0';
            parsed = css_parser_parse_color_value(trimmed, color);
        }
    }
    free(copy);
    return parsed;
}

static bool layout_parse_linear_gradient(const char* value, LinearGradientFill* gradient) {
    if (!value || !gradient) return false;
    const char* keyword = strstr(value, "linear-gradient");
    if (!keyword) return false;
    const char* open = strchr(keyword, '(');
    const char* close = strrchr(keyword, ')');
    if (!open || !close || close <= open) return false;

    char* inner = safe_strndup(open + 1, (size_t)(close - open - 1));
    if (!inner) return false;

    float angle = 180.0f;
    uint32_t colors[TACTILEBROWSER_MAX_GRADIENT_STOPS] = {0};
    size_t stop_count = 0;

    char* saveptr = NULL;
    char* token = strtok_r(inner, ",", &saveptr);
    while (token && stop_count < TACTILEBROWSER_MAX_GRADIENT_STOPS) {
        char* trimmed = trim_whitespace_inplace(token);
        if (*trimmed == '\0') {
            token = strtok_r(NULL, ",", &saveptr);
            continue;
        }

        if (stop_count == 0) {
            float parsed_angle = 0.0f;
            if (parse_angle_keyword(trimmed, &parsed_angle)) {
                angle = parsed_angle;
                token = strtok_r(NULL, ",", &saveptr);
                continue;
            }
        }

        uint32_t color = 0;
        if (parse_gradient_color_token(trimmed, &color)) {
            colors[stop_count++] = color;
        }

        token = strtok_r(NULL, ",", &saveptr);
    }

    free(inner);

    if (stop_count < 2) {
        return false;
    }

    gradient->angle_deg = angle;
    gradient->stop_count = stop_count;
    for (size_t i = 0; i < stop_count; ++i) {
        gradient->stops[i].color = colors[i];
        gradient->stops[i].position = (stop_count == 1) ? 0.0f : (float)i / (float)(stop_count - 1);
    }

    return true;
}

static void layout_apply_background_fill(RenderInterface* iface,
                                         Renderer* renderer,
                                         LayoutBox* box,
                                         void* widget) {
    if (!iface || !renderer || !box || !widget) return;

    if (box->background.type == BACKGROUND_FILL_LINEAR_GRADIENT &&
        box->background.data.linear.stop_count > 0) {
        if (iface->set_bg_gradient) {
            iface->set_bg_gradient(renderer, widget, &box->background.data.linear);
        } else if (iface->set_bg_color) {
            iface->set_bg_color(renderer, widget, box->background.data.linear.stops[0].color);
        }
    } else if (box->has_explicit_bg_color && iface->set_bg_color) {
        iface->set_bg_color(renderer, widget, box->bg_color);
    }
}

// Initialize layout engine
bool layout_engine_init(void) {
    return true;
}

// Cleanup layout engine
void layout_engine_cleanup(void) {
    // Nothing to clean up yet
}

// Create a new layout node
LayoutNode* layout_node_create(ElementType type) {
    LayoutNode* node = (LayoutNode*)calloc(1, sizeof(LayoutNode));
    if (!node) return NULL;
    
    node->type = type;
    node->box = DEFAULT_BOX;
    
    // Set display properties based on element type
    switch (type) {
        case ELEMENT_HEADING1:
            node->box.font_size = 32;
            node->box.line_height = 40;
            node->box.margin.top = 20;
            node->box.margin.bottom = 16;
            node->box.is_block = true;
            break;
            
        case ELEMENT_HEADING2:
            node->box.font_size = 24;
            node->box.line_height = 32;
            node->box.margin.top = 16;
            node->box.margin.bottom = 12;
            node->box.is_block = true;
            break;
            
        case ELEMENT_HEADING3:
            node->box.font_size = 20;
            node->box.line_height = 28;
            node->box.margin.top = 12;
            node->box.margin.bottom = 10;
            node->box.is_block = true;
            break;
            
        case ELEMENT_HEADING4:
        case ELEMENT_HEADING5:
        case ELEMENT_HEADING6:
            node->box.font_size = 16;
            node->box.line_height = 24;
            node->box.margin.top = 10;
            node->box.margin.bottom = 8;
            node->box.is_block = true;
            break;
            
        case ELEMENT_PARAGRAPH:
            node->box.margin.top = 8;
            node->box.margin.bottom = 8;
            node->box.is_block = true;
            break;
            
        case ELEMENT_DIV:
        case ELEMENT_CONTAINER:
            node->box.is_block = true;
            node->box.padding = (BoxSpacing){0, 0, 0, 0};
            break;
            
        case ELEMENT_UNORDERED_LIST:
        case ELEMENT_ORDERED_LIST:
            node->box.margin.top = 8;
            node->box.margin.bottom = 8;
            node->box.padding.left = 24;
            node->box.is_block = true;
            break;
            
        case ELEMENT_LIST_ITEM:
            node->box.margin.bottom = 4;
            node->box.is_block = true;
            break;
            
        case ELEMENT_SPAN:
        case ELEMENT_STRONG:
        case ELEMENT_EM:
        case ELEMENT_BOLD:
        case ELEMENT_ITALIC:
        case ELEMENT_UNDERLINE:
        case ELEMENT_LINK:
            node->box.is_block = false;
            node->box.padding = (BoxSpacing){0, 0, 0, 0};
            node->box.margin = (BoxSpacing){0, 0, 0, 0};
            break;
            
        case ELEMENT_BUTTON:
            node->box.padding = (BoxSpacing){8, 16, 8, 16};
            node->box.margin = (BoxSpacing){4, 4, 4, 4};
            node->box.is_inline_block = true;
            break;
        case ELEMENT_INPUT_TEXT:
            node->box.padding = (BoxSpacing){6, 10, 6, 10};
            node->box.margin = (BoxSpacing){6, 6, 6, 6};
            node->box.is_inline_block = true;
            node->box.width = 240;
            node->box.width_auto = false;
            node->box.height = 32;
            node->box.height_auto = false;
            break;
        case ELEMENT_TEXTAREA:
            node->box.padding = (BoxSpacing){8, 8, 8, 8};
            node->box.margin = (BoxSpacing){8, 0, 8, 0};
            node->box.is_block = true;
            node->box.width_auto = true;
            node->box.height = 120;
            node->box.height_auto = false;
            break;   
        case ELEMENT_BREAK:
            node->box.is_block = true;
            node->box.height = 10;
            node->box.height_auto = false;
            break;
            
        case ELEMENT_HORIZONTAL_RULE:
            node->box.is_block = true;
            node->box.height = 2;
            node->box.height_auto = false;
            node->box.margin.top = 12;
            node->box.margin.bottom = 12;
            break;
            
        default:
            node->box.is_block = true;
            break;
    }
    
    return node;
}

// Destroy layout node and its children
void layout_node_destroy(LayoutNode* node) {
    if (!node) return;
    
    // Destroy children recursively
    LayoutNode* child = node->first_child;
    while (child) {
        LayoutNode* next = child->next_sibling;
        layout_node_destroy(child);
        child = next;
    }
    
    free(node->text_content);
    free(node->href);
    free(node->href_resolved);
    free(node->href_path);
    free(node->form_value);
    free(node->placeholder);
    free(node);
}

// Add child node
void layout_node_add_child(LayoutNode* parent, LayoutNode* child) {
    if (!parent || !child) return;
    
    child->parent = parent;
    
    if (!parent->first_child) {
        parent->first_child = child;
    } else {
        LayoutNode* last = parent->first_child;
        while (last->next_sibling) {
            last = last->next_sibling;
        }
        last->next_sibling = child;
    }
}

// Initialize layout context
LayoutContext* layout_context_create(RenderContext* render_ctx) {
    LayoutContext* ctx = (LayoutContext*)calloc(1, sizeof(LayoutContext));
    if (!ctx) return NULL;
    
    ctx->render_context = render_ctx;
    ctx->container_width = render_ctx->max_width;
    ctx->current_x = 0;
    ctx->current_y = 0;
    ctx->max_line_height = 0;
    
    return ctx;
}

// Destroy layout context
void layout_context_destroy(LayoutContext* ctx) {
    if (!ctx) return;
    
    if (ctx->root) {
        layout_node_destroy(ctx->root);
    }
    
    free(ctx);
}

// Parse CSS spacing value (e.g., "10px", "10px 20px", "10px 20px 30px 40px")
BoxSpacing layout_parse_spacing(const char* value) {
    BoxSpacing spacing = {0, 0, 0, 0};
    if (!value) return spacing;
    
    int values[4] = {0, 0, 0, 0};
    int count = 0;
    
    const char* p = value;
    while (*p && count < 4) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;
        
        values[count++] = atoi(p);
        
        while (*p && !isspace((unsigned char)*p)) p++;
    }
    
    // Apply CSS spacing rules
    if (count == 1) {
        // All sides
        spacing.top = spacing.right = spacing.bottom = spacing.left = values[0];
    } else if (count == 2) {
        // Vertical | Horizontal
        spacing.top = spacing.bottom = values[0];
        spacing.left = spacing.right = values[1];
    } else if (count == 3) {
        // Top | Horizontal | Bottom
        spacing.top = values[0];
        spacing.left = spacing.right = values[1];
        spacing.bottom = values[2];
    } else if (count == 4) {
        // Top | Right | Bottom | Left
        spacing.top = values[0];
        spacing.right = values[1];
        spacing.bottom = values[2];
        spacing.left = values[3];
    }
    
    return spacing;
}

// Apply CSS property to layout box
void layout_apply_css_property(LayoutBox* box, const char* property, const char* value) {
    if (!box || !property || !value) return;
    
    if (strcmp(property, "margin") == 0) {
        box->margin = layout_parse_spacing(value);
    } else if (strcmp(property, "margin-top") == 0) {
        box->margin.top = atoi(value);
    } else if (strcmp(property, "margin-right") == 0) {
        box->margin.right = atoi(value);
    } else if (strcmp(property, "margin-bottom") == 0) {
        box->margin.bottom = atoi(value);
    } else if (strcmp(property, "margin-left") == 0) {
        box->margin.left = atoi(value);
    } else if (strcmp(property, "padding") == 0) {
        box->padding = layout_parse_spacing(value);
    } else if (strcmp(property, "padding-top") == 0) {
        box->padding.top = atoi(value);
    } else if (strcmp(property, "padding-right") == 0) {
        box->padding.right = atoi(value);
    } else if (strcmp(property, "padding-bottom") == 0) {
        box->padding.bottom = atoi(value);
    } else if (strcmp(property, "padding-left") == 0) {
        box->padding.left = atoi(value);
    } else if (strcmp(property, "width") == 0) {
        if (strcmp(value, "auto") == 0) {
            box->width_auto = true;
        } else {
            box->width = atoi(value);
            box->width_auto = false;
        }
    } else if (strcmp(property, "height") == 0) {
        if (strcmp(value, "auto") == 0) {
            box->height_auto = true;
        } else {
            box->height = atoi(value);
            box->height_auto = false;
        }
    } else if (strcmp(property, "font-size") == 0) {
        box->font_size = atoi(value);
        if (box->line_height < box->font_size + 4) {
            box->line_height = box->font_size + 6;
        }
    } else if (strcmp(property, "line-height") == 0) {
        box->line_height = atoi(value);
    } else if (strcmp(property, "display") == 0) {
        if (strcmp(value, "block") == 0) {
            box->is_block = true;
            box->is_inline_block = false;
        } else if (strcmp(value, "inline") == 0) {
            box->is_block = false;
            box->is_inline_block = false;
        } else if (strcmp(value, "inline-block") == 0) {
            box->is_block = false;
            box->is_inline_block = true;
        }
    } else if (strcmp(property, "text-align") == 0) {
        if (strcmp(value, "center") == 0) {
            box->text_align = 1;
        } else if (strcmp(value, "right") == 0) {
            box->text_align = 2;
        } else {
            box->text_align = 0;
        }
    } else if (strcmp(property, "color") == 0) {
        uint32_t parsed = 0;
        if (layout_parse_css_color(value, &parsed)) {
            box->color = parsed;
            box->has_explicit_color = true;
        }
    } else if (strcmp(property, "background") == 0 || strcmp(property, "background-image") == 0) {
        LinearGradientFill gradient = {0};
        if (layout_parse_linear_gradient(value, &gradient)) {
            box->background.type = BACKGROUND_FILL_LINEAR_GRADIENT;
            box->background.data.linear = gradient;
            box->bg_color = gradient.stops[0].color;
            box->has_explicit_bg_color = true;
        } else {
            uint32_t parsed = 0;
            if (layout_parse_css_color(value, &parsed)) {
                box->bg_color = parsed;
                box->has_explicit_bg_color = true;
                box->background.type = BACKGROUND_FILL_SOLID;
            }
        }
    } else if (strcmp(property, "background-color") == 0) {
        uint32_t parsed = 0;
        if (layout_parse_css_color(value, &parsed)) {
            box->bg_color = parsed;
            box->has_explicit_bg_color = true;
            box->background.type = BACKGROUND_FILL_SOLID;
        }
    }
}

// Calculate total width including padding, border, margin
int layout_get_total_width(const LayoutBox* box) {
    return box->margin.left + box->border.left + box->padding.left +
           box->width +
           box->padding.right + box->border.right + box->margin.right;
}

// Calculate total height including padding, border, margin
int layout_get_total_height(const LayoutBox* box) {
    return box->margin.top + box->border.top + box->padding.top +
           box->height +
           box->padding.bottom + box->border.bottom + box->margin.bottom;
}

// Estimate text width (simple approximation)
static int estimate_text_width(const char* text, int font_size) {
    if (!text) return 0;
    // Rough approximation: character width ≈ font_size * 0.6
    return (int)(strlen(text) * font_size * 0.6);
}

// Wrap text to fit within max_width
TextLines* layout_wrap_text(const char* text, int max_width, int font_size) {
    if (!text) return NULL;
    
    TextLines* result = (TextLines*)calloc(1, sizeof(TextLines));
    if (!result) return NULL;
    
    int text_len = strlen(text);
    if (text_len == 0) return result;
    
    // Allocate initial arrays
    int capacity = 4;
    result->lines = (char**)calloc(capacity, sizeof(char*));
    result->line_widths = (int*)calloc(capacity, sizeof(int));
    
    const char* start = text;
    const char* end = text + text_len;
    
    while (start < end) {
        // Skip leading whitespace
        while (start < end && isspace((unsigned char)*start)) start++;
        if (start >= end) break;
        
        // Find line break point
        const char* line_end = start;
        const char* last_space = NULL;
        int current_width = 0;
        
        while (line_end < end) {
            if (*line_end == '\n') {
                break;
            }
            
            // Check if adding this character exceeds max width
            int char_width = (int)(font_size * 0.6);
            if (current_width + char_width > max_width && last_space) {
                line_end = last_space;
                break;
            }
            
            if (isspace((unsigned char)*line_end)) {
                last_space = line_end;
            }
            
            current_width += char_width;
            line_end++;
        }
        
        // Ensure we have capacity
        if (result->line_count >= capacity) {
            capacity *= 2;
            result->lines = (char**)realloc(result->lines, capacity * sizeof(char*));
            result->line_widths = (int*)realloc(result->line_widths, capacity * sizeof(int));
        }
        
        // Copy line
        int line_len = line_end - start;
        result->lines[result->line_count] = (char*)malloc(line_len + 1);
        memcpy(result->lines[result->line_count], start, line_len);
        result->lines[result->line_count][line_len] = '\0';
        result->line_widths[result->line_count] = estimate_text_width(result->lines[result->line_count], font_size);
        result->line_count++;
        
        start = line_end;
        if (start < end && *start == '\n') start++;
    }
    
    return result;
}

// Free text lines
void layout_free_text_lines(TextLines* lines) {
    if (!lines) return;
    
    for (int i = 0; i < lines->line_count; i++) {
        free(lines->lines[i]);
    }
    free(lines->lines);
    free(lines->line_widths);
    free(lines);
}

// Calculate dimensions for a node
void layout_calculate_dimensions(LayoutNode* node, int available_width) {
    if (!node) return;
    
    // Calculate content width
    if (node->box.width_auto) {
        int inner_width = available_width - 
                         node->box.margin.left - node->box.margin.right -
                         node->box.padding.left - node->box.padding.right -
                         node->box.border.left - node->box.border.right;
        
        if (node->box.is_block) {
            // Block elements take full available width
            node->box.width = inner_width > 0 ? inner_width : available_width;
        } else {
            // Inline elements fit to content
            if (node->text_content) {
                node->box.width = estimate_text_width(node->text_content, node->box.font_size);
            } else {
                node->box.width = 0;
            }
        }
    }
    
    // Calculate content height
    if (node->box.height_auto) {
        if (node->text_content) {
            // Wrap text and calculate height
            int content_width = node->box.width;
            TextLines* lines = layout_wrap_text(node->text_content, content_width, node->box.font_size);
            if (lines) {
                node->box.height = lines->line_count * node->box.line_height;
                layout_free_text_lines(lines);
            } else {
                node->box.height = node->box.line_height;
            }
        } else if (node->first_child) {
            // Calculate height based on children
            int child_height = 0;
            LayoutNode* child = node->first_child;
            while (child) {
                layout_calculate_dimensions(child, node->box.width);
                child_height += layout_get_total_height(&child->box);
                child = child->next_sibling;
            }
            node->box.height = child_height;
        } else {
            node->box.height = node->box.line_height;
        }
    }
}

// Position node and children
void layout_position_node(LayoutNode* node, int parent_x, int parent_y) {
    if (!node) return;
    
    // Calculate position including margins
    node->box.x = parent_x + node->box.margin.left;
    node->box.y = parent_y + node->box.margin.top;
    
    // Position children
    if (node->first_child) {
        int child_x = node->box.x + node->box.padding.left + node->box.border.left;
        int child_y = node->box.y + node->box.padding.top + node->box.border.top;
        
        LayoutNode* child = node->first_child;
        while (child) {
            layout_position_node(child, child_x, child_y);
            
            // Move down for next block-level child
            if (child->box.is_block) {
                child_y += layout_get_total_height(&child->box);
            } else {
                // Inline elements flow horizontally
                child_x += layout_get_total_width(&child->box);
            }
            
            child = child->next_sibling;
        }
    }
}

// Render layout tree to widgets
void layout_render_tree(LayoutNode* root, RenderContext* render_ctx) {
    if (!root || !render_ctx || !render_ctx->renderer) return;
    
    // Recursively render nodes
    LayoutNode* node = root;
    if (!node) return;
    
    // Create widget based on node type
    RenderInterface* iface = render_ctx->renderer->interface;
    
    const char* form_value = node->form_value ? node->form_value : "";
    const char* placeholder = node->placeholder ? node->placeholder : NULL;

    if (node->type == ELEMENT_INPUT_TEXT && iface->create_text_input) {
        node->widget = iface->create_text_input(render_ctx->renderer,
                                               form_value,
                                               placeholder,
                                               node->box.x,
                                               node->box.y,
                                               node->box.width,
                                               node->box.height);
        layout_apply_background_fill(iface, render_ctx->renderer, &node->box, node->widget);
    }
    else if (node->type == ELEMENT_TEXTAREA && iface->create_text_area) {
        node->widget = iface->create_text_area(render_ctx->renderer,
                                              form_value,
                                              node->box.x,
                                              node->box.y,
                                              node->box.width,
                                              node->box.height);
        layout_apply_background_fill(iface, render_ctx->renderer, &node->box, node->widget);
    }
    else if (node->text_content && strlen(node->text_content) > 0) {
        if (node->type == ELEMENT_BUTTON && iface->create_button) {
            node->widget = iface->create_button(render_ctx->renderer, 
                                               node->text_content,
                                               node->box.x, 
                                               node->box.y);
        } else if (iface->create_label) {
            node->widget = iface->create_label(render_ctx->renderer,
                                              node->text_content,
                                              node->box.x,
                                              node->box.y);
        }
        
        // Apply colors
        if (node->widget && iface->set_text_color) {
            iface->set_text_color(render_ctx->renderer, node->widget, node->box.color);
        }
        layout_apply_background_fill(iface, render_ctx->renderer, &node->box, node->widget);
        
        // Register link handler
        if (node->type == ELEMENT_LINK && iface->register_link_handler) {
            const char* link_target = node->href_path ? node->href_path
                                       : (node->href_resolved ? node->href_resolved : node->href);
            if (link_target && link_target[0] != '\0') {
                iface->register_link_handler(render_ctx->renderer, node->widget, link_target);
            }
        }
    } else if (node->type == ELEMENT_DIV || node->type == ELEMENT_CONTAINER) {
        if (iface->create_container) {
            node->widget = iface->create_container(render_ctx->renderer,
                                                   node->box.x,
                                                   node->box.y,
                                                   node->box.width,
                                                   node->box.height);
        }
        layout_apply_background_fill(iface, render_ctx->renderer, &node->box, node->widget);
    }
    
    // Render children
    LayoutNode* child = node->first_child;
    while (child) {
        layout_render_tree(child, render_ctx);
        child = child->next_sibling;
    }
}
