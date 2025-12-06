#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Resolve a possibly relative URL against a base URL.
// Returns a newly allocated absolute URL string that the caller must free.
// Returns NULL if resolution fails or if the candidate URL is empty/unsupported.
char* tactilebrowser_resolve_url(const char* base_url, const char* candidate_url);

#ifdef __cplusplus
}
#endif
