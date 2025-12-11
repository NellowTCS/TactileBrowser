#include "html_parser.h"
#include "tactilebrowser_core.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <lexbor/dom/interfaces/node.h>
#include <lexbor/tag/tag.h>

// Default HTML downloader (placeholder)
static RenderResult default_download_html(const char* url, MemoryBuffer* buffer) {
    // This should be overridden by platform-specific implementation
    return RENDER_ERROR_NETWORK;
}

// HTML parser interface implementation
static RenderResult html_parser_download_html(const char* url, MemoryBuffer* buffer) {
    return default_download_html(url, buffer);
}

static lxb_html_document_t* html_parser_parse_html(const char* html, size_t length) {
    lxb_html_document_t* document = lxb_html_document_create();
    if (!document) return NULL;

    lxb_status_t status = lxb_html_document_parse(document, (const lxb_char_t*)html, length);
    if (status != LXB_STATUS_OK) {
        lxb_html_document_destroy(document);
        return NULL;
    }

    return document;
}

static char* html_parser_extract_title(lxb_html_document_t* document) {
    if (!document) return safe_strdup("Untitled");

    lxb_dom_element_t* root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (!root) return safe_strdup("Untitled");

    lxb_dom_collection_t* collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 16);
    if (!collection) return safe_strdup("Untitled");

    lxb_status_t status = lxb_dom_elements_by_tag_name(root, collection,
                                                       (const lxb_char_t*)"title", 5);

    if (status != LXB_STATUS_OK || lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return safe_strdup("Untitled");
    }

    lxb_dom_element_t* title_element = lxb_dom_collection_element(collection, 0);
    size_t text_len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(lxb_dom_interface_node(title_element), &text_len);

    char* result = safe_strdup("Untitled");
    if (text && text_len > 0) {
        result = safe_strndup((const char*)text, text_len);
    }

    if (text) lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
    lxb_dom_collection_destroy(collection, true);

    return result;
}

static char* html_parser_extract_text_content(lxb_html_document_t* document) {
    if (!document) return NULL;

    lxb_dom_element_t* body = html_parser.find_body_element(document);
    if (!body) return NULL;

    size_t text_len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(lxb_dom_interface_node(body), &text_len);

    char* result = NULL;
    if (text && text_len > 0) {
        result = safe_strndup((const char*)text, text_len);
    }

    if (text) lxb_dom_document_destroy_text(lxb_dom_interface_document(document), text);
    return result;
}

static lxb_dom_element_t* html_parser_find_body_element(lxb_html_document_t* document) {
    if (!document) return NULL;

    lxb_dom_element_t* root = lxb_dom_document_element(lxb_dom_interface_document(document));
    if (!root) return NULL;

    lxb_dom_collection_t* collection = lxb_dom_collection_make(lxb_dom_interface_document(document), 4);
    if (!collection) return NULL;

    if (lxb_dom_elements_by_tag_name(root, collection, (const lxb_char_t*)"body", 4) != LXB_STATUS_OK ||
        lxb_dom_collection_length(collection) == 0) {
        lxb_dom_collection_destroy(collection, true);
        return NULL;
    }

    lxb_dom_element_t* body = lxb_dom_collection_element(collection, 0);
    lxb_dom_collection_destroy(collection, true);
    return body;
}

static const char* html_parser_get_element_tag(lxb_dom_element_t* element, size_t* length) {
    return (const char*)lxb_dom_element_local_name(element, length);
}

static char* html_parser_get_element_text(lxb_dom_element_t* element, size_t* length) {
    if (!element) {
        if (length) *length = 0;
        return NULL;
    }

    size_t text_len = 0;
    lxb_char_t* text = lxb_dom_node_text_content(lxb_dom_interface_node(element), &text_len);
    if (length) *length = text_len;

    char* copy = NULL;
    if (text && text_len > 0) {
        copy = safe_strndup((const char*)text, text_len);
    }

    if (text) {
        lxb_dom_node_t* node = lxb_dom_interface_node(element);
        lxb_dom_document_t* owner = node ? node->owner_document : NULL;
        if (owner) {
            lxb_dom_document_destroy_text(owner, text);
        } else {
            free(text);
        }
    }

    return copy;
}

static const char* html_parser_get_element_attr(lxb_dom_element_t* element, const char* attr_name, size_t* length) {
    return (const char*)lxb_dom_element_get_attribute(element,
                                                     (const lxb_char_t*)attr_name,
                                                     strlen(attr_name),
                                                     length);
}

static lxb_dom_node_t* html_parser_get_next_sibling(lxb_dom_node_t* node) {
    return lxb_dom_node_next(node);
}

static lxb_dom_node_t* html_parser_get_first_child(lxb_dom_node_t* node) {
    return lxb_dom_node_first_child(node);
}

// Global HTML parser instance
HtmlParserInterface html_parser = {
    .download_html = html_parser_download_html,
    .parse_html = html_parser_parse_html,
    .extract_title = html_parser_extract_title,
    .extract_text_content = html_parser_extract_text_content,
    .find_body_element = html_parser_find_body_element,
    .get_element_tag = html_parser_get_element_tag,
    .get_element_text = html_parser_get_element_text,
    .get_element_attr = html_parser_get_element_attr,
    .get_next_sibling = html_parser_get_next_sibling,
    .get_first_child = html_parser_get_first_child
};

// Initialize HTML parser
bool html_parser_init(void) {
    // Initialize lexbor if needed
    return true;
}

// Cleanup HTML parser
void html_parser_cleanup(void) {
    // Cleanup lexbor if needed
}

// Determine element type from Lexbor element using native tag IDs
ElementType get_element_type_from_element(lxb_dom_element_t* element) {
    if (!element) return ELEMENT_UNKNOWN;

    lxb_tag_id_t tag_id = lxb_dom_element_tag_id(element);

    switch (tag_id) {
        case LXB_TAG_H1: return ELEMENT_HEADING1;
        case LXB_TAG_H2: return ELEMENT_HEADING2;
        case LXB_TAG_H3: return ELEMENT_HEADING3;
        case LXB_TAG_H4: return ELEMENT_HEADING4;
        case LXB_TAG_H5: return ELEMENT_HEADING5;
        case LXB_TAG_H6: return ELEMENT_HEADING6;
        case LXB_TAG_P: return ELEMENT_PARAGRAPH;
        case LXB_TAG_A: return ELEMENT_LINK;
        case LXB_TAG_BUTTON: return ELEMENT_BUTTON;
        case LXB_TAG_INPUT: return ELEMENT_INPUT_TEXT;
        case LXB_TAG_TEXTAREA: return ELEMENT_TEXTAREA;
        case LXB_TAG_DIV: return ELEMENT_DIV;
        case LXB_TAG_BODY:
        case LXB_TAG_MAIN:
        case LXB_TAG_SECTION:
        case LXB_TAG_ARTICLE:
        case LXB_TAG_HEADER:
        case LXB_TAG_FOOTER:
        case LXB_TAG_NAV:
        case LXB_TAG_ASIDE:
        case LXB_TAG_HTML:
            return ELEMENT_CONTAINER;
        case LXB_TAG_SPAN: return ELEMENT_SPAN;
        case LXB_TAG_STRONG: return ELEMENT_STRONG;
        case LXB_TAG_EM: return ELEMENT_EM;
        case LXB_TAG_B: return ELEMENT_BOLD;
        case LXB_TAG_I: return ELEMENT_ITALIC;
        case LXB_TAG_U: return ELEMENT_UNDERLINE;
        case LXB_TAG_LI: return ELEMENT_LIST_ITEM;
        case LXB_TAG_OL: return ELEMENT_ORDERED_LIST;
        case LXB_TAG_UL: return ELEMENT_UNORDERED_LIST;
        case LXB_TAG_IMG: return ELEMENT_IMAGE;
        case LXB_TAG_BR: return ELEMENT_BREAK;
        case LXB_TAG_HR: return ELEMENT_HORIZONTAL_RULE;
        default: return ELEMENT_UNKNOWN;
    }
}