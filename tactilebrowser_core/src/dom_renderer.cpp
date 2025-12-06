#include "dom_renderer.h"
#include "tactilebrowser_core.h"
#include "css_parser.h"
#include "html_parser.h"
#include "url_utils.h"
#include "layout_engine.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <lexbor/dom/interfaces/node.h>

// Helper: Copy node text
static char* copy_node_text(lxb_dom_node_t* node, size_t* length) {
    if (!node) {
        if (length) *length = 0;
        return NULL;
    }

    size_t text_len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(node, &text_len);
    if (length) *length = text_len;

    char* copy = NULL;
    if (text && text_len > 0) {
        copy = safe_strndup((const char*)text, text_len);
    }

    if (text) {
        lxb_dom_document_t* owner = node->owner_document;
        if (owner) {
            lxb_dom_document_destroy_text(owner, text);
        } else {
            free(text);
        }
    }

    return copy;
}

// Helper: Check if rel attribute contains "stylesheet"
static bool rel_contains_stylesheet(const char* rel, size_t length) {
    if (!rel || length == 0) return false;
    char* lowered = safe_strndup(rel, length);
    if (!lowered) return false;
    for (size_t i = 0; i < length; ++i) {
        lowered[i] = (char)tolower((unsigned char)lowered[i]);
    }
    bool is_stylesheet = strstr(lowered, "stylesheet") != NULL;
    free(lowered);
    return is_stylesheet;
}

// Helper: Import external stylesheet
static void import_external_stylesheet(const char* href, size_t href_len, const char* base_url) {
    if (!href || href_len == 0 || !html_parser.download_html) return;
    char* href_copy = safe_strndup(href, href_len);
    if (!href_copy) return;

    char* absolute_url = tactilebrowser_resolve_url(base_url, href_copy);
    free(href_copy);
    if (!absolute_url) return;

    MemoryBuffer css_buffer;
    memory_buffer_init(&css_buffer);
    RenderResult result = html_parser.download_html(absolute_url, &css_buffer);
    if (result == RENDER_SUCCESS && css_buffer.data && css_buffer.size > 0) {
        css_parser_add_stylesheet(css_buffer.data, css_buffer.size);
    }
    memory_buffer_free(&css_buffer);
    free(absolute_url);
}

// Collect all stylesheets from document
static void collect_stylesheets(lxb_dom_node_t* node, const char* base_url) {
    if (!node) return;

    if (node->type == LXB_DOM_NODE_TYPE_ELEMENT) {
        lxb_dom_element_t* element = (lxb_dom_element_t*)node;
        size_t tag_len = 0;
        const char* tag = html_parser.get_element_tag(element, &tag_len);
        
        if (tag && tag_len == 5 && strncmp(tag, "style", 5) == 0) {
            size_t css_len = 0;
            char* css_text = html_parser.get_element_text(element, &css_len);
            if (css_text && css_len > 0) {
                css_parser_add_stylesheet(css_text, css_len);
            }
            free(css_text);
        }
        else if (tag && tag_len == 4 && strncmp(tag, "link", 4) == 0) {
            size_t rel_len = 0;
            const char* rel_attr = html_parser.get_element_attr(element, "rel", &rel_len);
            if (rel_contains_stylesheet(rel_attr, rel_len)) {
                size_t href_len = 0;
                const char* href_attr = html_parser.get_element_attr(element, "href", &href_len);
                if (href_attr && href_len > 0) {
                    import_external_stylesheet(href_attr, href_len, base_url);
                }
            }
        }
    }

    lxb_dom_node_t* child = html_parser.get_first_child(node);
    while (child) {
        collect_stylesheets(child, base_url);
        child = html_parser.get_next_sibling(child);
    }
}

static LayoutNode* build_layout_tree_from_dom(lxb_dom_node_t* dom_node, RenderContext* context) {
    if (!dom_node) return NULL;
    
    lxb_dom_node_type_t node_type = dom_node->type;
    
    // Handle text nodes
    if (node_type == LXB_DOM_NODE_TYPE_TEXT) {
        size_t len = 0;
        char* txt = copy_node_text(dom_node, &len);
        
        if (txt && len > 0) {
            // Trim whitespace
            char* start = txt;
            char* end = txt + len - 1;
            while (start <= end && isspace((unsigned char)*start)) start++;
            while (end >= start && isspace((unsigned char)*end)) end--;
            
            if (start <= end) {
                LayoutNode* text_node = layout_node_create(ELEMENT_SPAN);
                if (text_node) {
                    size_t trimmed_len = end - start + 1;
                    text_node->text_content = (char*)malloc(trimmed_len + 1);
                    memcpy(text_node->text_content, start, trimmed_len);
                    text_node->text_content[trimmed_len] = '\0';
                }
                free(txt);
                return text_node;
            }
        }
        free(txt);
        return NULL;
    }
    
    // Handle element nodes only
    if (node_type != LXB_DOM_NODE_TYPE_ELEMENT) {
        return NULL;
    }
    
    lxb_dom_element_t* element = (lxb_dom_element_t*)dom_node;
    ElementType elem_type = get_element_type_from_element(element);
    
    // Skip scripts and styles
    size_t tag_len = 0;
    const char* tag = html_parser.get_element_tag(element, &tag_len);
    if (tag && ((tag_len == 6 && strncmp(tag, "script", 6) == 0) ||
                (tag_len == 5 && strncmp(tag, "style", 5) == 0))) {
        return NULL;
    }
    
    // Create layout node
    LayoutNode* layout_node = layout_node_create(elem_type);
    if (!layout_node) return NULL;
    
    // Determine if we should extract text from this element
    bool should_extract_text = false;
    switch (elem_type) {
        case ELEMENT_HEADING1:
        case ELEMENT_HEADING2:
        case ELEMENT_HEADING3:
        case ELEMENT_HEADING4:
        case ELEMENT_HEADING5:
        case ELEMENT_HEADING6:
        case ELEMENT_PARAGRAPH:
        case ELEMENT_LINK:
        case ELEMENT_BUTTON:
        case ELEMENT_SPAN:
        case ELEMENT_STRONG:
        case ELEMENT_EM:
        case ELEMENT_BOLD:
        case ELEMENT_ITALIC:
        case ELEMENT_UNDERLINE:
        case ELEMENT_LIST_ITEM:
            should_extract_text = true;
            break;
        default:
            break;
    }
    
    if (should_extract_text) {
        size_t text_len = 0;
        char* text = html_parser.get_element_text(element, &text_len);
        if (text && text_len > 0) {
            // Trim whitespace
            char* start = text;
            char* end = text + text_len - 1;
            while (start <= end && isspace((unsigned char)*start)) start++;
            while (end >= start && isspace((unsigned char)*end)) end--;
            
            if (start <= end) {
                size_t trimmed_len = end - start + 1;
                layout_node->text_content = (char*)malloc(trimmed_len + 1);
                memcpy(layout_node->text_content, start, trimmed_len);
                layout_node->text_content[trimmed_len] = '\0';
            }
        }
        free(text);
    }
    
    // Extract href for links
    if (elem_type == ELEMENT_LINK) {
        size_t href_len = 0;
        const char* href_attr = html_parser.get_element_attr(element, "href", &href_len);
        if (href_attr && href_len > 0) {
            layout_node->href = safe_strndup(href_attr, href_len);
            layout_node->box.color = 0x0000EE; // Blue for links
        }
    }
    
    // Apply CSS classes
    size_t class_len = 0;
    const char* class_attr = html_parser.get_element_attr(element, "class", &class_len);
    if (class_attr && class_len > 0) {
        char* classes = safe_strndup(class_attr, class_len);
        if (classes) {
            char* saveptr = NULL;
            char* token = strtok_r(classes, " \t\r\n", &saveptr);
            while (token) {
                size_t token_len = strlen(token);
                if (token_len > 0) {
                    size_t selector_len = token_len + 1;
                    char* selector = (char*)malloc(selector_len + 1);
                    if (selector) {
                        selector[0] = '.';
                        memcpy(selector + 1, token, token_len + 1);
                        
                        const char* css_block = css_parser_get_declarations(selector, selector_len);
                        if (css_block) {
                            // Parse CSS declarations and apply to layout box
                            char* css_copy = safe_strdup(css_block);
                            if (css_copy) {
                                char* prop_saveptr = NULL;
                                char* property = strtok_r(css_copy, ";", &prop_saveptr);
                                while (property) {
                                    char* colon = strchr(property, ':');
                                    if (colon) {
                                        *colon = '\0';
                                        char* prop_name = property;
                                        char* prop_value = colon + 1;
                                        
                                        // Trim whitespace
                                        while (*prop_name && isspace((unsigned char)*prop_name)) prop_name++;
                                        while (*prop_value && isspace((unsigned char)*prop_value)) prop_value++;
                                        
                                        char* prop_end = prop_name + strlen(prop_name) - 1;
                                        while (prop_end > prop_name && isspace((unsigned char)*prop_end)) *prop_end-- = '\0';
                                        
                                        char* val_end = prop_value + strlen(prop_value) - 1;
                                        while (val_end > prop_value && isspace((unsigned char)*val_end)) *val_end-- = '\0';
                                        
                                        layout_apply_css_property(&layout_node->box, prop_name, prop_value);
                                    }
                                    property = strtok_r(NULL, ";", &prop_saveptr);
                                }
                                free(css_copy);
                            }
                        }
                        free(selector);
                    }
                }
                token = strtok_r(NULL, " \t\r\n", &saveptr);
            }
            free(classes);
        }
    }
    
    // Apply inline styles
    size_t style_len = 0;
    const char* style_attr = html_parser.get_element_attr(element, "style", &style_len);
    if (style_attr && style_len > 0) {
        char* style_copy = safe_strndup(style_attr, style_len);
        if (style_copy) {
            char* saveptr = NULL;
            char* property = strtok_r(style_copy, ";", &saveptr);
            while (property) {
                char* colon = strchr(property, ':');
                if (colon) {
                    *colon = '\0';
                    char* prop_name = property;
                    char* prop_value = colon + 1;
                    
                    // Trim whitespace
                    while (*prop_name && isspace((unsigned char)*prop_name)) prop_name++;
                    while (*prop_value && isspace((unsigned char)*prop_value)) prop_value++;
                    
                    char* prop_end = prop_name + strlen(prop_name) - 1;
                    while (prop_end > prop_name && isspace((unsigned char)*prop_end)) *prop_end-- = '\0';
                    
                    char* val_end = prop_value + strlen(prop_value) - 1;
                    while (val_end > prop_value && isspace((unsigned char)*val_end)) *val_end-- = '\0';
                    
                    layout_apply_css_property(&layout_node->box, prop_name, prop_value);
                }
                property = strtok_r(NULL, ";", &saveptr);
            }
            free(style_copy);
        }
    }
    
    // Process children (if we didn't extract text or text is empty)
    if (!should_extract_text || !layout_node->text_content || layout_node->text_content[0] == '\0') {
        lxb_dom_node_t* child = html_parser.get_first_child(dom_node);
        while (child) {
            LayoutNode* child_layout = build_layout_tree_from_dom(child, context);
            if (child_layout) {
                layout_node_add_child(layout_node, child_layout);
            }
            child = html_parser.get_next_sibling(child);
        }
    }
    
    return layout_node;
}

static RenderResult dom_renderer_render_document(lxb_html_document_t* document, RenderContext* context) {
    if (!document || !context || !context->renderer) return RENDER_ERROR_UNKNOWN;

    // Collect stylesheets
    css_parser_reset();
    lxb_dom_element_t* root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (root) {
        collect_stylesheets(lxb_dom_interface_node(root), context->document_url);
    }

    // Get body
    lxb_dom_element_t* body = html_parser.find_body_element(document);
    if (!body) return RENDER_ERROR_PARSE;

    // Clear container
    if (context->renderer->interface->clear_container) {
        context->renderer->interface->clear_container(context->renderer, context->root_container);
    }

    // Build layout tree from DOM
    LayoutNode* layout_root = build_layout_tree_from_dom(lxb_dom_interface_node(body), context);
    if (layout_root) {
        // Calculate dimensions
        layout_calculate_dimensions(layout_root, context->max_width);
        
        // Position nodes
        layout_position_node(layout_root, 0, 10);
        
        // Render to screen
        layout_render_tree(layout_root, context);
        
        // Update context Y position
        context->current_y = layout_root->box.y + layout_get_total_height(&layout_root->box);
        
        // Cleanup
        layout_node_destroy(layout_root);
    }

    return RENDER_SUCCESS;
}

// Stub implementations for backwards compatibility (unused now)
static void dom_renderer_render_node(lxb_dom_node_t* node, RenderContext* context) {
    (void)node;
    (void)context;
}

static void* dom_renderer_create_element_widget(ElementType type, RenderContext* context, const char* text) {
    (void)type;
    (void)context;
    (void)text;
    return NULL;
}

static void dom_renderer_apply_styles(void* widget, RenderContext* context, const char* style) {
    (void)widget;
    (void)context;
    (void)style;
}

// Global DOM renderer instance
DomRendererInterface dom_renderer = {
    .render_document = dom_renderer_render_document,
    .render_node = dom_renderer_render_node,
    .create_element_widget = dom_renderer_create_element_widget,
    .apply_styles = dom_renderer_apply_styles
};

// Initialize DOM renderer
bool dom_renderer_init(void) {
    return layout_engine_init();
}

// Cleanup DOM renderer
void dom_renderer_cleanup(void) {
    layout_engine_cleanup();
}

// Main rendering function
RenderResult render_html_to_container(const char* url, RenderContext* context) {
    if (!url || !context) return RENDER_ERROR_UNKNOWN;

    // Download HTML
    MemoryBuffer buffer = {0};
    RenderResult download_result = html_parser.download_html(url, &buffer);
    if (download_result != RENDER_SUCCESS) {
        return download_result;
    }

    if (!buffer.data || buffer.size == 0) {
        return RENDER_ERROR_NETWORK;
    }

    // Parse HTML
    lxb_html_document_t* document = html_parser.parse_html(buffer.data, buffer.size);
    if (!document) {
        free(buffer.data);
        return RENDER_ERROR_PARSE;
    }

    // Render document using THE POWER ENGINE
    RenderResult render_result = dom_renderer.render_document(document, context);

    // Cleanup
    lxb_html_document_destroy(document);
    free(buffer.data);

    return render_result;
}
