#include "url_utils.h"
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <lexbor/url/url.h>

// Callback for URL serialization
static unsigned int serialize_callback(const lxb_char_t *data, size_t len, void *ctx) {
    if (!ctx || !data || len == 0) {
        return len;
    }
    
    char** buffer_ptr = (char**)ctx;
    if (!*buffer_ptr) return 0;
    
    strncat(*buffer_ptr, (const char*)data, len);
    return len;
}

char* tactilebrowser_resolve_url(const char* base_url, const char* candidate_url) {
    if (!candidate_url) {
        return nullptr;
    }

    // Create and initialize parser
    lxb_url_parser_t *parser = lxb_url_parser_create();
    if (!parser) {
        return nullptr;
    }

    lxb_status_t status = lxb_url_parser_init(parser, nullptr);
    if (status != LXB_STATUS_OK) {
        lxb_url_parser_destroy(parser, true);
        return nullptr;
    }

    // Parse the URL with optional base
    lxb_url_t *url;
    if (base_url) {
        // First parse base URL
        url = lxb_url_parse(parser, nullptr, (const lxb_char_t*)base_url, strlen(base_url));
        if (!url) {
            lxb_url_parser_destroy(parser, true);
            return nullptr;
        }
        
        // Then parse candidate with base
        lxb_url_parser_clean(parser);
        url = lxb_url_parse(parser, url, (const lxb_char_t*)candidate_url, strlen(candidate_url));
    } else {
        // Just parse candidate
        url = lxb_url_parse(parser, nullptr, (const lxb_char_t*)candidate_url, strlen(candidate_url));
    }

    if (!url) {
        lxb_url_parser_destroy(parser, true);
        return nullptr;
    }

    // Allocate buffer for serialized URL
    char* result = (char*)malloc(4096);
    if (!result) {
        lxb_url_parser_destroy(parser, true);
        return nullptr;
    }

    result[0] = '\0';

    // Serialize the URL using callback
    lxb_url_serialize(url, serialize_callback, (void*)&result, false);

    // Clean up parser (this will also destroy the URL)
    lxb_url_parser_destroy(parser, true);

    return result;
}

char* tactilebrowser_extract_path(const char* absolute_url) {
    if (!absolute_url) {
        return nullptr;
    }

    lxb_url_parser_t* parser = lxb_url_parser_create();
    if (!parser) {
        return nullptr;
    }

    if (lxb_url_parser_init(parser, nullptr) != LXB_STATUS_OK) {
        lxb_url_parser_destroy(parser, true);
        return nullptr;
    }

    lxb_url_t* url = lxb_url_parse(parser, nullptr,
                                    (const lxb_char_t*)absolute_url,
                                    strlen(absolute_url));
    if (!url) {
        lxb_url_parser_destroy(parser, true);
        return nullptr;
    }

    size_t path_len = url->path.length;
    const lxb_char_t* path_data = url->path.str.data;
    bool needs_root = (path_len == 0);

    size_t query_len = url->query.length;
    const lxb_char_t* query_data = url->query.data;

    size_t fragment_len = url->fragment.length;
    const lxb_char_t* fragment_data = url->fragment.data;

    size_t total_len = (needs_root ? 1 : path_len);
    if (query_len > 0) {
        total_len += 1 + query_len;
    }
    if (fragment_len > 0) {
        total_len += 1 + fragment_len;
    }

    char* result = (char*)malloc(total_len + 1);
    if (!result) {
        lxb_url_parser_destroy(parser, true);
        return nullptr;
    }

    size_t offset = 0;
    if (needs_root) {
        result[offset++] = '/';
    } else if (path_len > 0 && path_data) {
        memcpy(result + offset, path_data, path_len);
        offset += path_len;
    }

    if (query_len > 0 && query_data) {
        result[offset++] = '?';
        memcpy(result + offset, query_data, query_len);
        offset += query_len;
    }

    if (fragment_len > 0 && fragment_data) {
        result[offset++] = '#';
        memcpy(result + offset, fragment_data, fragment_len);
        offset += fragment_len;
    }

    result[offset] = '\0';

    lxb_url_parser_destroy(parser, true);
    return result;
}
