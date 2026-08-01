#include "fdm.h"
#include "fdm_internal.h"
#include "css_parser.h"
#include "dom_renderer.h"
#include "html_parser.h"
#include "layout_engine.h"
#include "url_utils.h"
#include <stdlib.h>
#include <string.h>

// Memory helpers
char *fdm_strdup(const char *str) {
  if (!str)
    return NULL;
  size_t len = strlen(str);
  char *copy = (char *)malloc(len + 1);
  if (!copy)
    return NULL;
  memcpy(copy, str, len + 1);
  return copy;
}

char *fdm_strndup(const char *str, size_t n) {
  if (!str)
    return NULL;
  size_t len = 0;
  while (len < n && str[len] != '\0') {
    len++;
  }
  char *copy = (char *)malloc(len + 1);
  if (!copy)
    return NULL;
  memcpy(copy, str, len);
  copy[len] = '\0';
  return copy;
}

void fdm_buffer_free(FdmBuffer *buffer) {
  if (!buffer)
    return;
  free(buffer->data);
  buffer->data = NULL;
  buffer->size = 0;
}

// Session interaction storage (used by the layout engine during painting)
static void free_runs(FdmSession *session) {
  for (size_t i = 0; i < session->run_count; ++i) {
    free(session->runs[i].text);
  }
  free(session->runs);
  session->runs = NULL;
  session->run_count = 0;
  session->run_cap = 0;
}

static void free_regions(FdmSession *session) {
  for (size_t i = 0; i < session->region_count; ++i) {
    free(session->regions[i].url);
    free(session->regions[i].value);
    free(session->regions[i].placeholder);
  }
  free(session->regions);
  session->regions = NULL;
  session->region_count = 0;
  session->region_cap = 0;
}

// Clears per-frame interaction data. focus_region is preserved so the focused
// input keeps its highlight across repaints.
void fdm_session_reset_interaction(FdmSession *session) {
  if (!session)
    return;
  free_runs(session);
  free_regions(session);
  session->sel_run = -1;
  session->sel_anchor = 0;
  session->sel_focus = 0;
  session->dragging = false;
  session->press_region = -1;
}

void fdm_session_add_run(FdmSession *session, int x, int y, int w, int h,
                         int font_size, FdmColor color, bool underline,
                         const char *text) {
  if (!session || !text)
    return;

  char *copy = fdm_strdup(text);
  if (!copy)
    return;

  if (session->run_count >= session->run_cap) {
    size_t new_cap = session->run_cap ? session->run_cap * 2 : 16;
    FdmTextRun *runs =
        (FdmTextRun *)realloc(session->runs, new_cap * sizeof(FdmTextRun));
    if (!runs) {
      free(copy);
      return;
    }
    session->runs = runs;
    session->run_cap = new_cap;
  }

  FdmTextRun *run = &session->runs[session->run_count++];
  run->x = x;
  run->y = y;
  run->w = w;
  run->h = h;
  run->font_size = font_size;
  run->color = color;
  run->underline = underline;
  run->text = copy;
}

void fdm_session_add_region(FdmSession *session, int x, int y, int w, int h,
                            bool is_input, const char *url, const char *value,
                            const char *placeholder, int font_size,
                            LayoutNode *node) {
  if (!session)
    return;

  if (session->region_count >= session->region_cap) {
    size_t new_cap = session->region_cap ? session->region_cap * 2 : 8;
    FdmHitRegion *regions = (FdmHitRegion *)realloc(
        session->regions, new_cap * sizeof(FdmHitRegion));
    if (!regions)
      return;
    session->regions = regions;
    session->region_cap = new_cap;
  }

  FdmHitRegion *region = &session->regions[session->region_count++];
  memset(region, 0, sizeof(*region));
  region->x = x;
  region->y = y;
  region->w = w;
  region->h = h;
  region->is_input = is_input;
  region->url = is_input ? NULL : fdm_strdup(url);
  region->value = is_input ? fdm_strdup(value) : NULL;
  region->placeholder = is_input ? fdm_strdup(placeholder) : NULL;
  region->font_size = font_size;
  region->node = node;
}

// Session lifecycle

static FdmSession *g_sessions = NULL;

static FdmSession *find_session(FdmSurface *surface) {
  if (!surface)
    return NULL;
  for (FdmSession *session = g_sessions; session; session = session->next) {
    if (session->surface == surface)
      return session;
  }
  return NULL;
}

static FdmSession *get_session(FdmSurface *surface) {
  FdmSession *session = find_session(surface);
  if (session)
    return session;

  session = (FdmSession *)calloc(1, sizeof(FdmSession));
  if (!session)
    return NULL;
  session->surface = surface;
  session->sel_run = -1;
  session->press_region = -1;
  session->focus_region = -1;
  session->next = g_sessions;
  g_sessions = session;
  return session;
}

static void destroy_session(FdmSession *session) {
  if (!session)
    return;
  free(session->url);
  if (session->root) {
    layout_node_destroy(session->root);
  }
  free_runs(session);
  free_regions(session);
  free(session);
}

// Link handling
static FdmLinkHandler g_link_handler = NULL;
static void *g_link_handler_user_data = NULL;

// Core API

bool fdm_init(void) {
  return html_parser_init() && css_parser_init() && layout_engine_init() &&
         dom_renderer_init();
}

void fdm_cleanup(void) {
  FdmSession *session = g_sessions;
  while (session) {
    FdmSession *next = session->next;
    destroy_session(session);
    session = next;
  }
  g_sessions = NULL;
  dom_renderer_cleanup();
  layout_engine_cleanup();
  css_parser_cleanup();
  html_parser_cleanup();
}

void fdm_set_html_downloader(FdmHtmlDownloader downloader) {
  html_parser.download_html = downloader;
}

void fdm_set_link_handler(FdmLinkHandler handler, void *user_data) {
  g_link_handler = handler;
  g_link_handler_user_data = user_data;
}

static FdmResult render_document(FdmSession *session, const char *html,
                                 size_t length) {
  lxb_html_document_t *document = html_parser.parse_html(html, length);
  if (!document)
    return FDM_ERR_PARSE;
  FdmResult result = dom_renderer_render_document(document, session);
  lxb_html_document_destroy(document);
  return result;
}

FdmResult fdm_render_url(FdmSurface *surface, const char *url, int width,
                         int height) {
  if (!surface || !url)
    return FDM_ERR_UNKNOWN;

  FdmSession *session = get_session(surface);
  if (!session)
    return FDM_ERR_MEMORY;

  FdmBuffer buffer = {0};
  FdmResult result = html_parser.download_html(url, &buffer);
  if (result != FDM_OK) {
    fdm_buffer_free(&buffer);
    return result;
  }
  if (!buffer.data || buffer.size == 0) {
    fdm_buffer_free(&buffer);
    return FDM_ERR_NETWORK;
  }

  session->viewport_w = width;
  session->viewport_h = height;
  free(session->url);
  session->url = fdm_strdup(url);

  result = render_document(session, buffer.data, buffer.size);
  fdm_buffer_free(&buffer);
  return result;
}

FdmResult fdm_render_html(FdmSurface *surface, const char *url,
                          const char *html, size_t length, int width,
                          int height) {
  if (!surface || !html)
    return FDM_ERR_UNKNOWN;

  FdmSession *session = get_session(surface);
  if (!session)
    return FDM_ERR_MEMORY;

  session->viewport_w = width;
  session->viewport_h = height;
  free(session->url);
  session->url = url ? fdm_strdup(url) : NULL;

  return render_document(session, html, length);
}

void fdm_repaint(FdmSurface *surface) {
  FdmSession *session = find_session(surface);
  if (!session)
    return;
  fdm_paint_document(session);
}

int fdm_content_height(FdmSurface *surface) {
  FdmSession *session = find_session(surface);
  if (!session || !session->root)
    return 0;
  return session->root->box.y + layout_get_total_height(&session->root->box);
}

void fdm_scroll_by(FdmSurface *surface, int delta_y) {
  FdmSession *session = find_session(surface);
  if (!session || !session->root)
    return;

  int max_scroll = fdm_content_height(surface) - session->viewport_h;
  if (max_scroll < 0)
    max_scroll = 0;

  int new_scroll = session->scroll_y + delta_y;
  if (new_scroll < 0)
    new_scroll = 0;
  if (new_scroll > max_scroll)
    new_scroll = max_scroll;

  if (new_scroll != session->scroll_y) {
    session->scroll_y = new_scroll;
    fdm_paint_document(session);
  }
}

// Pointer / input handling
static int hit_test_region(FdmSession *session, int doc_x, int doc_y) {
  for (size_t i = 0; i < session->region_count; ++i) {
    FdmHitRegion *region = &session->regions[i];
    if (doc_x >= region->x && doc_x < region->x + region->w &&
        doc_y >= region->y && doc_y < region->y + region->h) {
      return (int)i;
    }
  }
  return -1;
}

static int hit_test_run(FdmSession *session, int doc_x, int doc_y) {
  for (size_t i = 0; i < session->run_count; ++i) {
    FdmTextRun *run = &session->runs[i];
    if (doc_x >= run->x && doc_x < run->x + run->w &&
        doc_y >= run->y && doc_y < run->y + run->h) {
      return (int)i;
    }
  }
  return -1;
}

bool fdm_handle_pointer(FdmSurface *surface, int x, int y, int action) {
  FdmSession *session = find_session(surface);
  if (!session)
    return false;

  int doc_x = x;
  int doc_y = y + session->scroll_y;

  switch (action) {
  case FDM_POINTER_DOWN: {
    session->press_x = x;
    session->press_y = y;
    int region = hit_test_region(session, doc_x, doc_y);
    session->press_region = region;

    if (region >= 0 && session->regions[region].is_input) {
      if (session->focus_region != region) {
        session->focus_region = region;
        fdm_paint_document(session);
      }
      return true;
    }
    if (region >= 0)
      return true; // link press; activation deferred until UP

    // Press on plain text: start a selection drag.
    int run = hit_test_run(session, doc_x, doc_y);
    session->sel_run = run;
    session->sel_anchor = doc_x;
    session->sel_focus = doc_x;
    session->dragging = run >= 0;
    fdm_paint_document(session);
    return session->dragging;
  }

  case FDM_POINTER_MOVE:
    if (session->dragging) {
      int run = hit_test_run(session, doc_x, doc_y);
      if (run != session->sel_run) {
        session->sel_run = run;
        session->sel_focus = doc_x;
        fdm_paint_document(session);
      }
      return true;
    }
    return false;

  case FDM_POINTER_UP: {
    bool consumed = session->dragging;
    if (session->dragging) {
      session->sel_run = hit_test_run(session, doc_x, doc_y);
      session->sel_focus = doc_x;
      session->dragging = false;
    }

    int region = session->press_region;
    session->press_region = -1;

    if (region >= 0 && !session->regions[region].is_input &&
        session->regions[region].url && g_link_handler &&
        abs(x - session->press_x) < 8 && abs(y - session->press_y) < 8) {
      g_link_handler(g_link_handler_user_data, session->regions[region].url);
      consumed = true;
    }

    fdm_paint_document(session);
    return consumed;
  }

  default:
    return false;
  }
}

void fdm_input_text(FdmSurface *surface, const char *text) {
  FdmSession *session = find_session(surface);
  if (!session || !text || text[0] == '\0')
    return;
  if (session->focus_region < 0 ||
      (size_t)session->focus_region >= session->region_count)
    return;

  FdmHitRegion *region = &session->regions[session->focus_region];
  if (!region->is_input || !region->node)
    return;

  LayoutNode *node = region->node;
  size_t current_len = node->form_value ? strlen(node->form_value) : 0;
  size_t add_len = strlen(text);

  char *value = (char *)realloc(node->form_value, current_len + add_len + 1);
  if (!value)
    return;
  node->form_value = value;
  memcpy(node->form_value + current_len, text, add_len + 1);

  fdm_paint_document(session);
}

void fdm_input_backspace(FdmSurface *surface) {
  FdmSession *session = find_session(surface);
  if (!session)
    return;
  if (session->focus_region < 0 ||
      (size_t)session->focus_region >= session->region_count)
    return;

  FdmHitRegion *region = &session->regions[session->focus_region];
  if (!region->is_input || !region->node || !region->node->form_value)
    return;

  size_t len = strlen(region->node->form_value);
  if (len == 0)
    return;
  region->node->form_value[len - 1] = '\0';

  fdm_paint_document(session);
}
