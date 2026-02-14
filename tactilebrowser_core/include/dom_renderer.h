#pragma once

#include "common_types.h"
#include "html_parser.h"

#ifdef __cplusplus
extern "C" {
#endif

// DOM renderer interface
typedef struct {
  // Render HTML document to container
  RenderResult (*render_document)(lxb_html_document_t *document,
                                  RenderContext *context);
  // Render DOM node recursively
  void (*render_node)(lxb_dom_node_t *node, RenderContext *context);
  // Create widget for element
  void *(*create_element_widget)(ElementType type, RenderContext *context,
                                 const char *text);
  // Apply CSS styles to widget
  void (*apply_styles)(void *widget, RenderContext *context, const char *style);
} DomRendererInterface;

// Global DOM renderer instance
extern DomRendererInterface dom_renderer;

// Initialize DOM renderer
bool dom_renderer_init(void);

// Cleanup DOM renderer
void dom_renderer_cleanup(void);

// Main rendering function
RenderResult render_html_to_container(const char *url, RenderContext *context);

#ifdef __cplusplus
}
#endif