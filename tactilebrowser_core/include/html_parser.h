#pragma once

#include "common_types.h"
#include <lexbor/html/html.h>
#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>

#ifdef __cplusplus
extern "C" {
#endif

// HTML parser interface
typedef struct {
    // Download HTML content from URL
    RenderResult (*download_html)(const char* url, MemoryBuffer* buffer);
    // Parse HTML document
    lxb_html_document_t* (*parse_html)(const char* html, size_t length);
    // Extract title from document
    char* (*extract_title)(lxb_html_document_t* document);
    // Extract text content from document
    char* (*extract_text_content)(lxb_html_document_t* document);
    // Find body element
    lxb_dom_element_t* (*find_body_element)(lxb_html_document_t* document);
    // Get element tag name
    const char* (*get_element_tag)(lxb_dom_element_t* element, size_t* length);
    // Get element text content
    char* (*get_element_text)(lxb_dom_element_t* element, size_t* length);
    // Get element attribute
    const char* (*get_element_attr)(lxb_dom_element_t* element, const char* attr_name, size_t* length);
    // Get next sibling
    lxb_dom_node_t* (*get_next_sibling)(lxb_dom_node_t* node);
    // Get first child
    lxb_dom_node_t* (*get_first_child)(lxb_dom_node_t* node);
} HtmlParserInterface;

// Global HTML parser instance
extern HtmlParserInterface html_parser;

// Initialize HTML parser
bool html_parser_init(void);

// Cleanup HTML parser
void html_parser_cleanup(void);

// Determine element type from Lexbor element (pure native Lexbor approach)
ElementType get_element_type_from_element(lxb_dom_element_t* element);

// Color table for named colors
extern const ColorEntry color_table[];
extern const size_t color_table_size;

#ifdef __cplusplus
}
#endif