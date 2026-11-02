#pragma once

#include "fdm_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the CSS parser module.
bool css_parser_init(void);

// Reset all cached stylesheet rules (must be called per document).
void css_parser_reset(void);

// Free all cached data.
void css_parser_cleanup(void);

// Parse and store rules from a stylesheet buffer.
void css_parser_add_stylesheet(const char *css, size_t length);

// Retrieve concatenated declarations for a selector (lowercase tag name).
const char *css_parser_get_declarations(const char *selector, size_t length);

// Parse a CSS color value using Lexbor (named colors, rgb(), hex, etc.).
bool css_parser_parse_color_value(const char *value, FdmColor *color_out);

#ifdef __cplusplus
}
#endif
