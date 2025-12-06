#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize CSS parser module (currently a no-op but kept for parity and future use)
bool css_parser_init(void);

// Reset all cached stylesheet rules (must be called per document)
void css_parser_reset(void);

// Free all cached data
void css_parser_cleanup(void);

// Parse and store rules from a stylesheet buffer
void css_parser_add_stylesheet(const char* css, size_t length);

// Retrieve concatenated declarations for a selector (lowercase tag name)
const char* css_parser_get_declarations(const char* selector, size_t length);

#ifdef __cplusplus
}
#endif
