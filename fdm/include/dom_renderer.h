#pragma once

#include "fdm_internal.h"
#include "html_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

// Render an already-parsed HTML document into a session: collects
// stylesheets, builds the layout tree, runs layout and paints.
FdmResult dom_renderer_render_document(lxb_html_document_t *document,
                                       FdmSession *session);

// Initialize/cleanup the DOM renderer (delegates to the layout engine).
bool dom_renderer_init(void);
void dom_renderer_cleanup(void);

#ifdef __cplusplus
}
#endif
