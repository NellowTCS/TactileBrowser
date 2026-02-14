#pragma once

#include "common_types.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize CSS parser module (currently a no-op but kept for parity and
// future use)
bool css_parser_init(void);

// Reset all cached stylesheet rules (must be called per document)
void css_parser_reset(void);

// Free all cached data
void css_parser_cleanup(void);

// Parse and store rules from a stylesheet buffer
void css_parser_add_stylesheet(const char *css, size_t length);

// Retrieve concatenated declarations for a selector (lowercase tag name)
const char *css_parser_get_declarations(const char *selector, size_t length);

// Parse inline style attribute and apply to widget
void css_parser_parse_inline_style(const char *style, RenderContext *context,
                                   void *widget);

// Parse a CSS color value using Lexbor (supports named colors, rgb(), etc.)
bool css_parser_parse_color_value(const char *value, uint32_t *color_out);

#ifdef __cplusplus
}
#endif
