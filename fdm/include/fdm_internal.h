#pragma once

// Internal types shared across the fdm modules. Not part of the public API.

#include "fdm.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Named color table entry
typedef struct {
  const char *name;
  FdmColor color;
} FdmColorEntry;

// HTML element types
typedef enum {
  ELEMENT_PARAGRAPH,
  ELEMENT_HEADING1,
  ELEMENT_HEADING2,
  ELEMENT_HEADING3,
  ELEMENT_HEADING4,
  ELEMENT_HEADING5,
  ELEMENT_HEADING6,
  ELEMENT_LINK,
  ELEMENT_BUTTON,
  ELEMENT_INPUT_TEXT,
  ELEMENT_TEXTAREA,
  ELEMENT_DIV,
  ELEMENT_SPAN,
  ELEMENT_STRONG,
  ELEMENT_EM,
  ELEMENT_BOLD,
  ELEMENT_ITALIC,
  ELEMENT_UNDERLINE,
  ELEMENT_LIST_ITEM,
  ELEMENT_ORDERED_LIST,
  ELEMENT_UNORDERED_LIST,
  ELEMENT_IMAGE,
  ELEMENT_CONTAINER,
  ELEMENT_BREAK,
  ELEMENT_HORIZONTAL_RULE,
  ELEMENT_UNKNOWN
} ElementType;

// CSS box spacing
typedef struct {
  int top;
  int right;
  int bottom;
  int left;
} BoxSpacing;

// Background fill kinds
typedef enum {
  BACKGROUND_FILL_NONE = 0,
  BACKGROUND_FILL_SOLID,
  BACKGROUND_FILL_LINEAR_GRADIENT
} FdmBackgroundFillType;

typedef struct {
  FdmBackgroundFillType type;
  union {
    FdmLinearGradient linear;
  } data;
} FdmBackgroundFill;

// Layout box model with CSS properties
typedef struct {
  BoxSpacing margin;
  BoxSpacing padding;
  BoxSpacing border;

  int width;        // Content width
  int height;       // Content height
  bool width_auto;  // Auto width calculation
  bool height_auto; // Auto height calculation

  int x;
  int y;

  bool is_block;         // Block vs inline
  bool is_inline_block;  // inline-block

  FdmColor color;
  FdmColor bg_color;
  bool has_explicit_color;
  bool has_explicit_bg_color;
  FdmBackgroundFill background;
  int font_size;
  int line_height;
  int text_align; // FDM_ALIGN_*

  bool scroll_y;
  bool scroll_x;
} LayoutBox;

// Layout node representing an element in the layout tree
typedef struct LayoutNode {
  ElementType type;
  LayoutBox box;

  char *text_content;
  char *href;          // raw href attribute
  char *href_resolved; // absolute URL
  char *href_path;     // path-only form
  char *form_value;    // inputs/textareas
  char *placeholder;

  struct LayoutNode *parent;
  struct LayoutNode *first_child;
  struct LayoutNode *next_sibling;
} LayoutNode;

// Text metrics callback used during layout/painting.
typedef int (*FdmMeasureFn)(void *ctx, const char *text, int font_size);

// Text wrapping result
typedef struct {
  char **lines;
  int line_count;
  int *line_widths;
} TextLines;

// A single wrapped line of text, recorded for hit-testing/selection.
typedef struct {
  int x;
  int y;
  int w;
  int h;
  int font_size;
  FdmColor color;
  bool underline;
  char *text; // owned
} FdmTextRun;

// An interactive region (link or form input), recorded for hit-testing.
typedef struct FdmHitRegion {
  int x;
  int y;
  int w;
  int h;
  bool is_input;
  char *url;         // links only (owned)
  char *value;       // inputs only (owned)
  char *placeholder; // inputs only (owned)
  int font_size;
  struct LayoutNode *node; // back-reference for form edits (not owned)
} FdmHitRegion;

// Per-surface retained document state.
typedef struct FdmSession {
  FdmSurface *surface;
  char *url;
  LayoutNode *root;

  int viewport_w;
  int viewport_h;
  int scroll_y;

  FdmTextRun *runs;
  size_t run_count;
  size_t run_cap;

  FdmHitRegion *regions;
  size_t region_count;
  size_t region_cap;

  // Selection state
  int sel_run;   // index into runs, -1 when inactive
  int sel_anchor;
  int sel_focus;
  bool dragging;

  // Pointer press tracking (for link clicks vs drags)
  int press_x;
  int press_y;
  int press_region; // -1 when not pressed on a region

  // Focused form input (region index, -1 when none)
  int focus_region;

  struct FdmSession *next;
} FdmSession;

// Session interaction storage (used by the layout engine during painting).
void fdm_session_reset_interaction(FdmSession *session);
void fdm_session_add_run(FdmSession *session, int x, int y, int w, int h,
                         int font_size, FdmColor color, bool underline,
                         const char *text);
void fdm_session_add_region(FdmSession *session, int x, int y, int w, int h,
                            bool is_input, const char *url, const char *value,
                            const char *placeholder, int font_size,
                            LayoutNode *node);

#ifdef __cplusplus
}
#endif
