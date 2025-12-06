#include "url_utils.h"
#include <cstdlib>
#include <cstring>
#include <cctype>
#include <lexbor/url/url.h>

char* tactilebrowser_resolve_url_lexbor(const char* base_url, const char* candidate_url) {
    if (!candidate_url) {
        return nullptr;
    }

    lxb_url_t *url = lxb_url_create();
    if (!url) {
        return nullptr;
    }

    lxb_status_t status;
    
    // If we have a base URL, use it
    if (base_url) {
        status = lxb_url_parse(url, (const lxb_char_t*)base_url, strlen(base_url));
        if (status != LXB_STATUS_OK) {
            lxb_url_destroy(url);
            return nullptr;
        }
        
        // Now resolve the candidate against the base
        status = lxb_url_resolve(url, (const lxb_char_t*)candidate_url, strlen(candidate_url));
    } else {
        // Just parse the candidate directly
        status = lxb_url_parse(url, (const lxb_char_t*)candidate_url, strlen(candidate_url));
    }

    if (status != LXB_STATUS_OK) {
        lxb_url_destroy(url);
        return nullptr;
    }

    // Get the serialized URL
    lxb_char_t buffer[4096];
    size_t url_len = lxb_url_serialize(url, buffer, sizeof(buffer));
    
    if (url_len == 0 || url_len >= sizeof(buffer)) {
        lxb_url_destroy(url);
        return nullptr;
    }

    // Allocate result
    char* result = (char*)malloc(url_len + 1);
    if (!result) {
        lxb_url_destroy(url);
        return nullptr;
    }

    memcpy(result, buffer, url_len);
    result[url_len] = '\0';
    
    lxb_url_destroy(url);
    return result;
}

char* tactilebrowser_resolve_url(const char* base_url, const char* candidate_url) {
    return tactilebrowser_resolve_url_lexbor(base_url, candidate_url);
}
