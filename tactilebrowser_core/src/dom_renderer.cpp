#include "dom_renderer.h"
#include "tactilebrowser_core.h"
#include "css_parser.h"
#include "html_parser.h"
#include "url_utils.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <lexbor/dom/interfaces/node.h>

// Default renderer (placeholder)
static RenderInterface* default_renderer = NULL;

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

static void apply_css_selector(RenderContext* context, void* widget, const char* selector, size_t length) {
    if (!context || !widget || !selector || length == 0) return;
    const char* css_block = css_parser_get_declarations(selector, length);
    if (css_block) {
        dom_renderer.apply_styles(widget, context, css_block);
    }
}

static void apply_class_selectors(lxb_dom_element_t* element, RenderContext* context, void* widget) {
    if (!element) return;
    size_t class_len = 0;
    const char* class_attr = html_parser.get_element_attr(element, "class", &class_len);
    if (!class_attr || class_len == 0) return;

    char* classes = safe_strndup(class_attr, class_len);
    if (!classes) return;

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
                apply_css_selector(context, widget, selector, selector_len);
                free(selector);
            }
        }
        token = strtok_r(NULL, " \t\r\n", &saveptr);
    }

    free(classes);
}

static void apply_id_selector(lxb_dom_element_t* element, RenderContext* context, void* widget) {
    if (!element) return;
    size_t id_len = 0;
    const char* id_attr = html_parser.get_element_attr(element, "id", &id_len);
    if (!id_attr || id_len == 0) return;

    char* selector = (char*)malloc(id_len + 2);
    if (!selector) return;

    selector[0] = '#';
    memcpy(selector + 1, id_attr, id_len);
    selector[id_len + 1] = '\0';

    apply_css_selector(context, widget, selector, id_len + 1);
    free(selector);
}

// DOM renderer interface implementation
static RenderResult dom_renderer_render_document(lxb_html_document_t* document, RenderContext* context) {
    if (!document || !context || !context->renderer) return RENDER_ERROR_UNKNOWN;

    css_parser_reset();
    lxb_dom_element_t* root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (root) {
        collect_stylesheets(lxb_dom_interface_node(root), context->document_url);
    }

    lxb_dom_element_t* body = html_parser.find_body_element(document);
    if (!body) return RENDER_ERROR_PARSE;

    // Clear container
    if (context->renderer->interface->clear_container) {
        context->renderer->interface->clear_container(context->renderer, context->root_container);
    }

    // Reset Y position
    context->current_y = 10;
    
    // Render body content
    dom_renderer.render_node(lxb_dom_interface_node(body), context);

    return RENDER_SUCCESS;
}

static void dom_renderer_render_node(lxb_dom_node_t* node, RenderContext* context) {
    if (!node || !context) return;

    lxb_dom_node_type_t node_type = node->type;

    if (node_type == LXB_DOM_NODE_TYPE_TEXT) {
        size_t len = 0;
        char* txt = copy_node_text(node, &len);

        if (txt && len > 0) {
            char* start = txt;
            char* end = txt + len - 1;

            while (start <= end && isspace((unsigned char)*start)) start++;
            while (end >= start && isspace((unsigned char)*end)) end--;

            if (start <= end) {
                size_t trimmed_len = end - start + 1;
                if (start != txt) {
                    memmove(txt, start, trimmed_len);
                }
                txt[trimmed_len] = '\0';

                void* label = dom_renderer.create_element_widget(ELEMENT_PARAGRAPH, context, txt);
                if (label && context->renderer->interface->get_height) {
                    int height = context->renderer->interface->get_height(context->renderer, label);
                    context->current_y += height + 3;
                }
            }
        }

        free(txt);
        return;
    }
    else if (node_type == LXB_DOM_NODE_TYPE_ELEMENT) {
        lxb_dom_element_t* el = (lxb_dom_element_t*)node;

        ElementType elem_type = get_element_type_from_element(el);

        bool consumes_children = false;
        size_t text_len = 0;
        char* text_content = NULL;

        switch (elem_type) {
            case ELEMENT_HEADING1:
            case ELEMENT_HEADING2:
            case ELEMENT_HEADING3:
            case ELEMENT_HEADING4:
            case ELEMENT_HEADING5:
            case ELEMENT_HEADING6:
            case ELEMENT_PARAGRAPH:
            case ELEMENT_LINK:
            case ELEMENT_SPAN:
            case ELEMENT_BUTTON:
            case ELEMENT_STRONG:
            case ELEMENT_EM:
            case ELEMENT_BOLD:
            case ELEMENT_ITALIC:
            case ELEMENT_UNDERLINE:
            case ELEMENT_LIST_ITEM:
                consumes_children = true;
                break;
            default:
                break;
        }

        if (consumes_children) {
            text_content = html_parser.get_element_text(el, &text_len);
            if (text_content && text_len > 0) {
                char* start = text_content;
                char* end = text_content + text_len - 1;

                while (start <= end && isspace((unsigned char)*start)) start++;
                while (end >= start && isspace((unsigned char)*end)) end--;

                if (start > end) {
                    text_content[0] = '\0';
                } else {
                    size_t trimmed_len = end - start + 1;
                    if (start != text_content) {
                        memmove(text_content, start, trimmed_len);
                    }
                    text_content[trimmed_len] = '\0';
                }
            }
        }

        void* widget = dom_renderer.create_element_widget(elem_type, context, text_content ? text_content : "");

        if (widget) {
            apply_class_selectors(el, context, widget);
            apply_id_selector(el, context, widget);

            // Apply inline styles
            size_t style_len;
            const char* style_attr = html_parser.get_element_attr(el, "style", &style_len);

            if (style_attr && style_len > 0) {
                char* style = safe_strndup(style_attr, style_len);
                if (style) {
                    css_parser_parse_inline_style(style, context, widget);
                    free(style);
                }
            }

            if (elem_type == ELEMENT_LINK && context->renderer->interface->register_link_handler) {
                size_t href_len = 0;
                const char* href_attr = html_parser.get_element_attr(el, "href", &href_len);
                if (href_attr && href_len > 0) {
                    char* href = safe_strndup(href_attr, href_len);
                    if (href) {
                        context->renderer->interface->register_link_handler(context->renderer, widget, href);
                        free(href);
                    }
                }
            }

            if (context->renderer->interface->get_height && elem_type != ELEMENT_BREAK && elem_type != ELEMENT_HORIZONTAL_RULE) {
                int height = context->renderer->interface->get_height(context->renderer, widget);
                context->current_y += height + 3;
            }

            bool render_children = !consumes_children || !text_content || text_content[0] == '\0';
            if (render_children) {
                lxb_dom_node_t* child = html_parser.get_first_child(node);
                while (child) {
                    dom_renderer.render_node(child, context);
                    child = html_parser.get_next_sibling(child);
                }
            }
        }

        free(text_content);
    }
}

static void* dom_renderer_create_element_widget(ElementType type, RenderContext* context, const char* text) {
    if (!context || !context->renderer) return NULL;

    void* widget = NULL;

    switch (type) {
        case ELEMENT_HEADING1:
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
                if (context->renderer->interface->set_text_color) {
                    context->renderer->interface->set_text_color(context->renderer, widget, 0x000080);
                }
                context->current_y += 5; // Extra space after H1
            }
            break;

        case ELEMENT_HEADING2:
        case ELEMENT_HEADING3:
        case ELEMENT_HEADING4:
        case ELEMENT_HEADING5:
        case ELEMENT_HEADING6:
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
                if (context->renderer->interface->set_text_color) {
                    context->renderer->interface->set_text_color(context->renderer, widget, 0x000080);
                }
                context->current_y += 3; // Extra space after headings
            }
            break;

        case ELEMENT_PARAGRAPH:
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
                context->current_y += 3; // Space after paragraph
            }
            break;

        case ELEMENT_LINK:
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
                if (context->renderer->interface->set_text_color) {
                    context->renderer->interface->set_text_color(context->renderer, widget, 0x0000EE);
                }
            }
            break;

        case ELEMENT_SPAN:
        case ELEMENT_STRONG:
        case ELEMENT_EM:
        case ELEMENT_BOLD:
        case ELEMENT_ITALIC:
        case ELEMENT_UNDERLINE:
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
            }
            break;

        case ELEMENT_BUTTON:
            if (context->renderer->interface->create_button) {
                widget = context->renderer->interface->create_button(context->renderer, text, 0, context->current_y);
                context->current_y += 3; // Space after button
            }
            break;

        case ELEMENT_LIST_ITEM:
            if (context->renderer->interface->create_label) {
                // Add bullet point
                char bullet_text[512] = "• ";
                if (text) {
                    strncat(bullet_text, text, sizeof(bullet_text) - 3);
                }
                widget = context->renderer->interface->create_label(context->renderer, bullet_text, 10, context->current_y);
            }
            break;

        case ELEMENT_UNORDERED_LIST:
        case ELEMENT_ORDERED_LIST:
            if (context->renderer->interface->create_container) {
                widget = context->renderer->interface->create_container(context->renderer, 0, context->current_y,
                                                           context->max_width, 50);
                context->current_y += 3; // Space after list
            }
            break;

        case ELEMENT_DIV:
            if (context->renderer->interface->create_container) {
                widget = context->renderer->interface->create_container(context->renderer, 0, context->current_y,
                                                           context->max_width, 50);
            }
            break;

        case ELEMENT_BREAK:
            context->current_y += 10; // Line break spacing
            break;

        case ELEMENT_HORIZONTAL_RULE:
            // Create a visual separator
            if (context->renderer->interface->create_container) {
                widget = context->renderer->interface->create_container(context->renderer, 0, context->current_y,
                                                           context->max_width, 2);
                context->current_y += 10; // Space around HR
            }
            break;

        case ELEMENT_IMAGE:
            // Placeholder for image
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, "[Image]", 0, context->current_y);
                context->current_y += 3;
            }
            break;

        default:
            // Default to label
            if (context->renderer->interface->create_label) {
                widget = context->renderer->interface->create_label(context->renderer, text, 0, context->current_y);
            }
            break;
    }

    return widget;
}

static void dom_renderer_apply_styles(void* widget, RenderContext* context, const char* style) {
    css_parser_parse_inline_style(style, context, widget);
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
    return true;
}

// Cleanup DOM renderer
void dom_renderer_cleanup(void) {
    // Cleanup if needed
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

    // Render document
    RenderResult render_result = dom_renderer.render_document(document, context);

    // Cleanup
    lxb_html_document_destroy(document);
    free(buffer.data);

    return render_result;
}