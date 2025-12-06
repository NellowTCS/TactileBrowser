#include "css_parser.h"
#include "tactilebrowser_core.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <lexbor/css/css.h>
#include <lexbor/css/stylesheet.h>
#include <lexbor/css/declaration.h>
#include <lexbor/css/rule.h>
#include <lexbor/css/value.h>
#include <lexbor/css/property.h>

// Static parser instance for stylesheet parsing
static lxb_css_parser_t* css_parser = NULL;

// Helper function to convert Lexbor color to uint32_t
static uint32_t lexbor_color_to_uint32(const lxb_css_value_color_t* color) {
    if (!color) return 0;

    switch (color->type) {
        case LXB_CSS_VALUE_HEX: {
            const lxb_css_value_color_hex_rgba_t* rgba = &color->u.hex.rgba;
            return ((uint32_t)rgba->r << 16) | ((uint32_t)rgba->g << 8) | (uint32_t)rgba->b;
        }
        case LXB_CSS_VALUE_RGB: {
            const lxb_css_value_color_rgba_t* rgba = &color->u.rgb;
            // Convert percentage/number values to 0-255 range
            uint8_t r = (rgba->r.type == LXB_CSS_VALUE__PERCENTAGE) ?
                       (uint8_t)((rgba->r.u.percentage.num / 100.0) * 255.0) : (uint8_t)rgba->r.u.number.num;
            uint8_t g = (rgba->g.type == LXB_CSS_VALUE__PERCENTAGE) ?
                       (uint8_t)((rgba->g.u.percentage.num / 100.0) * 255.0) : (uint8_t)rgba->g.u.number.num;
            uint8_t b = (rgba->b.type == LXB_CSS_VALUE__PERCENTAGE) ?
                       (uint8_t)((rgba->b.u.percentage.num / 100.0) * 255.0) : (uint8_t)rgba->b.u.number.num;
            return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
        }
        case LXB_CSS_VALUE_CURRENTCOLOR:
            // Return a default color for currentColor (could be made configurable)
            return 0x000000;
        case LXB_CSS_VALUE_INHERIT:
        case LXB_CSS_VALUE_INITIAL:
        case LXB_CSS_VALUE_UNSET:
        default:
            return 0x000000;
    }
}

bool css_parser_init(void) {
    if (css_parser) return true;

    css_parser = lxb_css_parser_create();
    if (!css_parser) return false;

    lxb_status_t status = lxb_css_parser_init(css_parser, NULL);
    if (status != LXB_STATUS_OK) {
        lxb_css_parser_destroy(css_parser, true);
        css_parser = NULL;
        return false;
    }

    return true;
}

void css_parser_reset(void) {
    // Reset parser state for new document
    // Currently a no-op, but kept for API compatibility
}

void css_parser_cleanup(void) {
    if (css_parser) {
        lxb_css_parser_destroy(css_parser, true);
        css_parser = NULL;
    }
}

void css_parser_add_stylesheet(const char* css, size_t length) {
    if (!css_parser || !css || length == 0) return;

    // Parse the stylesheet using Lexbor's built-in CSS parser
    lxb_css_stylesheet_t* stylesheet = lxb_css_stylesheet_parse(css_parser,
                                                               (const lxb_char_t*)css,
                                                               length);
    if (!stylesheet) return;

    // TODO: store and index rules by selectors for stylesheet lookup
    lxb_css_stylesheet_destroy(stylesheet, false);
}

const char* css_parser_get_declarations(const char* selector, size_t length) {
    // TODO: look up cached stylesheet rules for the given selector
    return NULL;
}

void css_parser_parse_inline_style(const char* style, RenderContext* context, void* widget) {
    if (!style || !context || !widget || !css_parser) return;

    // Parse the inline style declarations using Lexbor
    lxb_css_rule_declaration_list_t* decl_list = lxb_css_declaration_list_parse(
        css_parser, NULL, (const lxb_char_t*)style, strlen(style));

    if (!decl_list) return;

    // Iterate through all declarations
    lxb_css_rule_t* rule = decl_list->first;
    while (rule) {
        if (rule->type == LXB_CSS_RULE_DECLARATION) {
            lxb_css_rule_declaration_t* decl = (lxb_css_rule_declaration_t*)rule;

            // Handle different property types
            switch (decl->type) {
                case LXB_CSS_PROPERTY_COLOR: {
                    lxb_css_property_color_t* color_prop = decl->u.color;
                    if (color_prop && context->renderer->interface->set_text_color) {
                        uint32_t color = lexbor_color_to_uint32(color_prop);
                        context->renderer->interface->set_text_color(context->renderer, widget, color);
                    }
                    break;
                }
                case LXB_CSS_PROPERTY_BACKGROUND_COLOR: {
                    lxb_css_property_background_color_t* bg_color_prop = decl->u.background_color;
                    if (bg_color_prop && context->renderer->interface->set_bg_color) {
                        uint32_t color = lexbor_color_to_uint32(bg_color_prop);
                        context->renderer->interface->set_bg_color(context->renderer, widget, color);
                    }
                    break;
                }
                case LXB_CSS_PROPERTY_TEXT_ALIGN: {
                    lxb_css_property_text_align_t* text_align_prop = decl->u.text_align;
                    if (text_align_prop && context->renderer->interface->set_text_align) {
                        int align_value = 0; // left/default
                        switch (text_align_prop->type) {
                            case LXB_CSS_TEXT_ALIGN_CENTER:
                                align_value = 1; // center
                                break;
                            case LXB_CSS_TEXT_ALIGN_RIGHT:
                            case LXB_CSS_TEXT_ALIGN_END:
                                align_value = 2; // right
                                break;
                            case LXB_CSS_TEXT_ALIGN_LEFT:
                            case LXB_CSS_TEXT_ALIGN_START:
                            case LXB_CSS_TEXT_ALIGN_JUSTIFY:
                            default:
                                align_value = 0; // left
                                break;
                        }
                        context->renderer->interface->set_text_align(context->renderer, widget, align_value);
                    }
                    break;
                }
                // Add more property handlers as needed
                default:
                    // Ignore unsupported properties for now
                    break;
            }
        }

        rule = rule->next;
    }

    // Clean up the declaration list
    lxb_css_rule_declaration_list_destroy(decl_list, true);
}