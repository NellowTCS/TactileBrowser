#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Fundamental (fdm): a small, portable browser engine core.
//
// The core parses HTML/CSS (via Lexbor), builds a layout tree, and paints it
// onto a platform-provided FdmSurface using a minimal immediate-mode paint
// API. Ports only implement the surface ops and forward input back to the
// core; layout, links, text selection, scrolling and form inputs all live
// here.

// Result codes
typedef enum {
  FDM_OK = 0,
  FDM_ERR_NETWORK,
  FDM_ERR_PARSE,
  FDM_ERR_MEMORY,
  FDM_ERR_UNKNOWN
} FdmResult;

// Raw memory buffer (used for HTML downloads)
typedef struct {
  char *data;
  size_t size;
} FdmBuffer;

// A color, encoded as 0xRRGGBB.
typedef uint32_t FdmColor;

// Linear gradient fill
#define FDM_MAX_GRADIENT_STOPS 4

typedef struct {
  FdmColor color;
  float position; // 0.0 .. 1.0
} FdmGradientStop;

typedef struct {
  float angle_deg;
  size_t stop_count;
  FdmGradientStop stops[FDM_MAX_GRADIENT_STOPS];
} FdmLinearGradient;

// Text alignment
enum {
  FDM_ALIGN_LEFT = 0,
  FDM_ALIGN_CENTER = 1,
  FDM_ALIGN_RIGHT = 2
};

// Pointer actions for fdm_handle_pointer
enum {
  FDM_POINTER_DOWN = 0,
  FDM_POINTER_MOVE = 1,
  FDM_POINTER_UP = 2
};

// An opaque paint surface implemented by the host platform.
typedef struct FdmSurface FdmSurface;

// Paint operations a platform must implement. All coordinates are viewport
// pixels. Text is single-line: the core pre-wraps lines and calls draw_text
// once per line. x/y is the top-left corner of the first glyph.
typedef struct {
  // Begin a new frame; the platform should clear/reset the backing store.
  void (*begin_frame)(FdmSurface *surface, int width, int height);
  // Finish the frame; the platform presents it.
  void (*end_frame)(FdmSurface *surface);
  // Fill a solid rectangle.
  void (*fill_rect)(FdmSurface *surface, int x, int y, int w, int h,
                    FdmColor color);
  // Fill a rectangle with a linear gradient.
  void (*fill_rect_gradient)(FdmSurface *surface, int x, int y, int w, int h,
                             const FdmLinearGradient *gradient);
  // Draw a single wrapped line of text.
  void (*draw_text)(FdmSurface *surface, const char *text, int x, int y,
                    int max_width, int font_size, FdmColor color, int align,
                    bool underline);
  // Measure the pixel width of a single line of text at font_size.
  int (*measure_text)(FdmSurface *surface, const char *text, int font_size);
} FdmSurfaceOps;

struct FdmSurface {
  const FdmSurfaceOps *ops;
  void *platform_data; // backend-specific, e.g. a JS canvas context
  void *user_data;     // free for the host to use
};

// Downloads the HTML at url into *buffer. Return FDM_OK on success; the core
// frees the buffer memory itself.
typedef FdmResult (*FdmHtmlDownloader)(const char *url, FdmBuffer *buffer);

// Called when a link is activated (via fdm_handle_pointer).
typedef void (*FdmLinkHandler)(void *user_data, const char *url);

// Core API

// Initialize/cleanup the core library.
bool fdm_init(void);
void fdm_cleanup(void);

void fdm_set_html_downloader(FdmHtmlDownloader downloader);
void fdm_set_link_handler(FdmLinkHandler handler, void *user_data);

// Render a URL into the surface (requires a registered downloader).
FdmResult fdm_render_url(FdmSurface *surface, const char *url, int width,
                         int height);

// Render an HTML string into the surface.
FdmResult fdm_render_html(FdmSurface *surface, const char *url,
                          const char *html, size_t length, int width,
                          int height);

// Re-paint the current document onto the surface (after scrolling or
// selection changes).
void fdm_repaint(FdmSurface *surface);

// Route a pointer event to the current document. Returns true if the event
// was consumed.
bool fdm_handle_pointer(FdmSurface *surface, int x, int y, int action);

// Scroll the current document vertically (negative scrolls up).
void fdm_scroll_by(FdmSurface *surface, int delta_y);

// Total document height in pixels (for scroll ranges).
int fdm_content_height(FdmSurface *surface);

// Type into the focused form input.
void fdm_input_text(FdmSurface *surface, const char *text);
void fdm_input_backspace(FdmSurface *surface);

// Memory helpers
void fdm_buffer_free(FdmBuffer *buffer);
char *fdm_strdup(const char *str);
char *fdm_strndup(const char *str, size_t n);

#ifdef __cplusplus
}
#endif
