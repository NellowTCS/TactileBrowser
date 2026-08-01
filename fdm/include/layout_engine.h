#pragma once

#include "fdm_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the layout engine.
bool layout_engine_init(void);

// Cleanup the layout engine.
void layout_engine_cleanup(void);

// Create a new layout node with defaults for its element type.
LayoutNode *layout_node_create(ElementType type);

// Destroy a layout node and its children.
void layout_node_destroy(LayoutNode *node);

// Add a child node.
void layout_node_add_child(LayoutNode *parent, LayoutNode *child);

// Apply a CSS property to a layout box.
void layout_apply_css_property(LayoutBox *box, const char *property,
                               const char *value);

// Parse CSS spacing (margin, padding, border).
BoxSpacing layout_parse_spacing(const char *value);

// Total width/height including padding, border and margin.
int layout_get_total_width(const LayoutBox *box);
int layout_get_total_height(const LayoutBox *box);

// Wrap text into lines fitting max_width, using measure for metrics
// (falls back to a font-size heuristic when measure is NULL).
TextLines *layout_wrap_text(const char *text, int max_width, int font_size,
                            FdmMeasureFn measure, void *measure_ctx);
void layout_free_text_lines(TextLines *lines);

// Calculate box dimensions for a node and its children.
void layout_calculate_dimensions(LayoutNode *node, int available_width,
                                 FdmMeasureFn measure, void *measure_ctx);

// Position a node and its children.
void layout_position_node(LayoutNode *node, int parent_x, int parent_y);

// Run layout (dimensions + positioning) for a session's document.
void fdm_layout_document(FdmSession *session);

// Paint a session's document onto its surface.
void fdm_paint_document(FdmSession *session);

#ifdef __cplusplus
}
#endif
