#include "layout_engine.h"
#include "css_parser.h"
#include "fdm.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Default box model values
static const LayoutBox DEFAULT_BOX = {
    .margin = {0, 0, 0, 0},
    .padding = {8, 8, 8, 8},
    .border = {0, 0, 0, 0},
    .width = 0,
    .height = 0,
    .width_auto = true,
    .height_auto = true,
    .x = 0,
    .y = 0,
    .is_block = true,
    .is_inline_block = false,
    .color = 0xF0F6FC,
    .bg_color = 0x000000,
    .has_explicit_color = false,
    .has_explicit_bg_color = false,
    .background = {.type = BACKGROUND_FILL_NONE},
    .font_size = 14,
    .line_height = 20,
    .text_align = 0,
    .scroll_y = false,
    .scroll_x = false};

static char *trim_whitespace_inplace(char *str) {
  if (!str)
    return NULL;
  while (*str && isspace((unsigned char)*str)) {
    str++;
  }
  char *end = str + strlen(str);
  while (end > str && isspace((unsigned char)*(end - 1))) {
    *(--end) = '\0';
  }
  return str;
}

static bool layout_parse_css_color(const char *value, FdmColor *color) {
  if (!value || !color)
    return false;
  return css_parser_parse_color_value(value, color);
}

static bool parse_angle_keyword(const char *token, float *angle_deg) {
  if (!token || !angle_deg)
    return false;
  char *copy = fdm_strdup(token);
  if (!copy)
    return false;
  char *trimmed = trim_whitespace_inplace(copy);
  for (char *c = trimmed; *c; ++c) {
    *c = (char)tolower((unsigned char)*c);
  }

  bool handled = false;
  if (strstr(trimmed, "deg")) {
    double value = atof(trimmed);
    *angle_deg = (float)value;
    handled = true;
  } else if (strncmp(trimmed, "to ", 3) == 0) {
    bool to_top = strstr(trimmed, "top") != NULL;
    bool to_bottom = strstr(trimmed, "bottom") != NULL;
    bool to_left = strstr(trimmed, "left") != NULL;
    bool to_right = strstr(trimmed, "right") != NULL;
    if (to_top && to_right) {
      *angle_deg = 45.0f;
      handled = true;
    } else if (to_top && to_left) {
      *angle_deg = 315.0f;
      handled = true;
    } else if (to_bottom && to_right) {
      *angle_deg = 135.0f;
      handled = true;
    } else if (to_bottom && to_left) {
      *angle_deg = 225.0f;
      handled = true;
    } else if (to_top) {
      *angle_deg = 0.0f;
      handled = true;
    } else if (to_right) {
      *angle_deg = 90.0f;
      handled = true;
    } else if (to_bottom) {
      *angle_deg = 180.0f;
      handled = true;
    } else if (to_left) {
      *angle_deg = 270.0f;
      handled = true;
    }
  }

  free(copy);
  return handled;
}

static bool parse_gradient_color_token(const char *token, FdmColor *color) {
  if (!token || !color)
    return false;
  char *copy = fdm_strdup(token);
  if (!copy)
    return false;
  char *trimmed = trim_whitespace_inplace(copy);
  bool parsed = css_parser_parse_color_value(trimmed, color);
  if (!parsed) {
    char *last_space = strrchr(trimmed, ' ');
    if (last_space) {
      *last_space = '\0';
      parsed = css_parser_parse_color_value(trimmed, color);
    }
  }
  free(copy);
  return parsed;
}

static bool layout_parse_linear_gradient(const char *value,
                                         FdmLinearGradient *gradient) {
  if (!value || !gradient)
    return false;
  const char *keyword = strstr(value, "linear-gradient");
  if (!keyword)
    return false;
  const char *open = strchr(keyword, '(');
  const char *close = strrchr(keyword, ')');
  if (!open || !close || close <= open)
    return false;

  char *inner = fdm_strndup(open + 1, (size_t)(close - open - 1));
  if (!inner)
    return false;

  float angle = 180.0f;
  FdmColor colors[FDM_MAX_GRADIENT_STOPS] = {0};
  size_t stop_count = 0;

  char *saveptr = NULL;
  char *token = strtok_r(inner, ",", &saveptr);
  while (token && stop_count < FDM_MAX_GRADIENT_STOPS) {
    char *trimmed = trim_whitespace_inplace(token);
    if (*trimmed == '\0') {
      token = strtok_r(NULL, ",", &saveptr);
      continue;
    }

    if (stop_count == 0) {
      float parsed_angle = 0.0f;
      if (parse_angle_keyword(trimmed, &parsed_angle)) {
        angle = parsed_angle;
        token = strtok_r(NULL, ",", &saveptr);
        continue;
      }
    }

    FdmColor color = 0;
    if (parse_gradient_color_token(trimmed, &color)) {
      colors[stop_count++] = color;
    }

    token = strtok_r(NULL, ",", &saveptr);
  }

  free(inner);

  if (stop_count < 2) {
    return false;
  }

  gradient->angle_deg = angle;
  gradient->stop_count = stop_count;
  for (size_t i = 0; i < stop_count; ++i) {
    gradient->stops[i].color = colors[i];
    gradient->stops[i].position =
        (stop_count == 1) ? 0.0f : (float)i / (float)(stop_count - 1);
  }

  return true;
}

// Initialize layout engine
bool layout_engine_init(void) { return true; }

// Cleanup layout engine
void layout_engine_cleanup(void) {
  // Nothing to clean up yet
}

// Create a new layout node
LayoutNode *layout_node_create(ElementType type) {
  LayoutNode *node = (LayoutNode *)calloc(1, sizeof(LayoutNode));
  if (!node)
    return NULL;

  node->type = type;
  node->box = DEFAULT_BOX;

  // Set display properties based on element type
  switch (type) {
  case ELEMENT_HEADING1:
    node->box.font_size = 32;
    node->box.line_height = 40;
    node->box.margin.top = 20;
    node->box.margin.bottom = 16;
    node->box.is_block = true;
    break;

  case ELEMENT_HEADING2:
    node->box.font_size = 24;
    node->box.line_height = 32;
    node->box.margin.top = 16;
    node->box.margin.bottom = 12;
    node->box.is_block = true;
    break;

  case ELEMENT_HEADING3:
    node->box.font_size = 20;
    node->box.line_height = 28;
    node->box.margin.top = 12;
    node->box.margin.bottom = 10;
    node->box.is_block = true;
    break;

  case ELEMENT_HEADING4:
  case ELEMENT_HEADING5:
  case ELEMENT_HEADING6:
    node->box.font_size = 16;
    node->box.line_height = 24;
    node->box.margin.top = 10;
    node->box.margin.bottom = 8;
    node->box.is_block = true;
    break;

  case ELEMENT_PARAGRAPH:
    node->box.margin.top = 8;
    node->box.margin.bottom = 8;
    node->box.is_block = true;
    break;

  case ELEMENT_DIV:
  case ELEMENT_CONTAINER:
    node->box.is_block = true;
    node->box.padding = (BoxSpacing){0, 0, 0, 0};
    break;

  case ELEMENT_UNORDERED_LIST:
  case ELEMENT_ORDERED_LIST:
    node->box.margin.top = 8;
    node->box.margin.bottom = 8;
    node->box.padding.left = 24;
    node->box.is_block = true;
    break;

  case ELEMENT_LIST_ITEM:
    node->box.margin.bottom = 4;
    node->box.is_block = true;
    break;

  case ELEMENT_SPAN:
  case ELEMENT_STRONG:
  case ELEMENT_EM:
  case ELEMENT_BOLD:
  case ELEMENT_ITALIC:
  case ELEMENT_UNDERLINE:
  case ELEMENT_LINK:
    node->box.is_block = false;
    node->box.padding = (BoxSpacing){0, 0, 0, 0};
    node->box.margin = (BoxSpacing){0, 0, 0, 0};
    break;

  case ELEMENT_BUTTON:
    node->box.padding = (BoxSpacing){8, 16, 8, 16};
    node->box.margin = (BoxSpacing){4, 4, 4, 4};
    node->box.is_inline_block = true;
    break;
  case ELEMENT_INPUT_TEXT:
    node->box.padding = (BoxSpacing){6, 10, 6, 10};
    node->box.margin = (BoxSpacing){6, 6, 6, 6};
    node->box.is_inline_block = true;
    node->box.width = 240;
    node->box.width_auto = false;
    node->box.height = 32;
    node->box.height_auto = false;
    break;
  case ELEMENT_TEXTAREA:
    node->box.padding = (BoxSpacing){8, 8, 8, 8};
    node->box.margin = (BoxSpacing){8, 0, 8, 0};
    node->box.is_block = true;
    node->box.width_auto = true;
    node->box.height = 120;
    node->box.height_auto = false;
    break;
  case ELEMENT_BREAK:
    node->box.is_block = true;
    node->box.height = 10;
    node->box.height_auto = false;
    break;

  case ELEMENT_HORIZONTAL_RULE:
    node->box.is_block = true;
    node->box.height = 2;
    node->box.height_auto = false;
    node->box.margin.top = 12;
    node->box.margin.bottom = 12;
    break;

  default:
    node->box.is_block = true;
    break;
  }

  return node;
}

// Destroy layout node and its children
void layout_node_destroy(LayoutNode *node) {
  if (!node)
    return;

  // Destroy children recursively
  LayoutNode *child = node->first_child;
  while (child) {
    LayoutNode *next = child->next_sibling;
    layout_node_destroy(child);
    child = next;
  }

  free(node->text_content);
  free(node->href);
  free(node->href_resolved);
  free(node->href_path);
  free(node->form_value);
  free(node->placeholder);
  free(node);
}

// Add child node
void layout_node_add_child(LayoutNode *parent, LayoutNode *child) {
  if (!parent || !child)
    return;

  child->parent = parent;

  if (!parent->first_child) {
    parent->first_child = child;
  } else {
    LayoutNode *last = parent->first_child;
    while (last->next_sibling) {
      last = last->next_sibling;
    }
    last->next_sibling = child;
  }
}

// Parse CSS spacing value (e.g., "10px", "10px 20px", "10px 20px 30px 40px")
BoxSpacing layout_parse_spacing(const char *value) {
  BoxSpacing spacing = {0, 0, 0, 0};
  if (!value)
    return spacing;

  int values[4] = {0, 0, 0, 0};
  int count = 0;

  const char *p = value;
  while (*p && count < 4) {
    while (*p && isspace((unsigned char)*p))
      p++;
    if (!*p)
      break;

    values[count++] = atoi(p);

    while (*p && !isspace((unsigned char)*p))
      p++;
  }

  // Apply CSS spacing rules
  if (count == 1) {
    // All sides
    spacing.top = spacing.right = spacing.bottom = spacing.left = values[0];
  } else if (count == 2) {
    // Vertical | Horizontal
    spacing.top = spacing.bottom = values[0];
    spacing.left = spacing.right = values[1];
  } else if (count == 3) {
    // Top | Horizontal | Bottom
    spacing.top = values[0];
    spacing.left = spacing.right = values[1];
    spacing.bottom = values[2];
  } else if (count == 4) {
    // Top | Right | Bottom | Left
    spacing.top = values[0];
    spacing.right = values[1];
    spacing.bottom = values[2];
    spacing.left = values[3];
  }

  return spacing;
}

// Apply CSS property to layout box
void layout_apply_css_property(LayoutBox *box, const char *property,
                               const char *value) {
  if (!box || !property || !value)
    return;

  if (strcmp(property, "margin") == 0) {
    box->margin = layout_parse_spacing(value);
  } else if (strcmp(property, "margin-top") == 0) {
    box->margin.top = atoi(value);
  } else if (strcmp(property, "margin-right") == 0) {
    box->margin.right = atoi(value);
  } else if (strcmp(property, "margin-bottom") == 0) {
    box->margin.bottom = atoi(value);
  } else if (strcmp(property, "margin-left") == 0) {
    box->margin.left = atoi(value);
  } else if (strcmp(property, "padding") == 0) {
    box->padding = layout_parse_spacing(value);
  } else if (strcmp(property, "padding-top") == 0) {
    box->padding.top = atoi(value);
  } else if (strcmp(property, "padding-right") == 0) {
    box->padding.right = atoi(value);
  } else if (strcmp(property, "padding-bottom") == 0) {
    box->padding.bottom = atoi(value);
  } else if (strcmp(property, "padding-left") == 0) {
    box->padding.left = atoi(value);
  } else if (strcmp(property, "width") == 0) {
    if (strcmp(value, "auto") == 0) {
      box->width_auto = true;
    } else {
      box->width = atoi(value);
      box->width_auto = false;
    }
  } else if (strcmp(property, "height") == 0) {
    if (strcmp(value, "auto") == 0) {
      box->height_auto = true;
    } else {
      box->height = atoi(value);
      box->height_auto = false;
    }
  } else if (strcmp(property, "font-size") == 0) {
    box->font_size = atoi(value);
    if (box->line_height < box->font_size + 4) {
      box->line_height = box->font_size + 6;
    }
  } else if (strcmp(property, "line-height") == 0) {
    box->line_height = atoi(value);
  } else if (strcmp(property, "display") == 0) {
    if (strcmp(value, "block") == 0) {
      box->is_block = true;
      box->is_inline_block = false;
    } else if (strcmp(value, "inline") == 0) {
      box->is_block = false;
      box->is_inline_block = false;
    } else if (strcmp(value, "inline-block") == 0) {
      box->is_block = false;
      box->is_inline_block = true;
    }
  } else if (strcmp(property, "text-align") == 0) {
    if (strcmp(value, "center") == 0) {
      box->text_align = 1;
    } else if (strcmp(value, "right") == 0) {
      box->text_align = 2;
    } else {
      box->text_align = 0;
    }
  } else if (strcmp(property, "color") == 0) {
    FdmColor parsed = 0;
    if (layout_parse_css_color(value, &parsed)) {
      box->color = parsed;
      box->has_explicit_color = true;
    }
  } else if (strcmp(property, "background") == 0 ||
             strcmp(property, "background-image") == 0) {
    FdmLinearGradient gradient = {0};
    if (layout_parse_linear_gradient(value, &gradient)) {
      box->background.type = BACKGROUND_FILL_LINEAR_GRADIENT;
      box->background.data.linear = gradient;
      box->bg_color = gradient.stops[0].color;
      box->has_explicit_bg_color = true;
    } else {
      FdmColor parsed = 0;
      if (layout_parse_css_color(value, &parsed)) {
        box->bg_color = parsed;
        box->has_explicit_bg_color = true;
        box->background.type = BACKGROUND_FILL_SOLID;
      }
    }
  } else if (strcmp(property, "background-color") == 0) {
    FdmColor parsed = 0;
    if (layout_parse_css_color(value, &parsed)) {
      box->bg_color = parsed;
      box->has_explicit_bg_color = true;
      box->background.type = BACKGROUND_FILL_SOLID;
    }
  }
}

// Total width including padding, border, margin
int layout_get_total_width(const LayoutBox *box) {
  return box->margin.left + box->border.left + box->padding.left + box->width +
         box->padding.right + box->border.right + box->margin.right;
}

// Total height including padding, border, margin
int layout_get_total_height(const LayoutBox *box) {
  return box->margin.top + box->border.top + box->padding.top + box->height +
         box->padding.bottom + box->border.bottom + box->margin.bottom;
}

// Default text-width heuristic when no measure callback is available
static int default_measure(void *ctx, const char *text, int font_size) {
  (void)ctx;
  if (!text)
    return 0;
  return (int)(strlen(text) * font_size * 0.6);
}

// Measure text via the session's surface ops (falls back to heuristic).
static int measure_surface(void *ctx, const char *text, int font_size) {
  FdmSurface *surface = (FdmSurface *)ctx;
  if (surface && surface->ops && surface->ops->measure_text)
    return surface->ops->measure_text(surface, text, font_size);
  return default_measure(ctx, text, font_size);
}

// Wrap text to fit within max_width, measuring per word.
TextLines *layout_wrap_text(const char *text, int max_width, int font_size,
                            FdmMeasureFn measure, void *measure_ctx) {
  if (!text)
    return NULL;

  FdmMeasureFn m = measure ? measure : default_measure;
  if (!m)
    m = default_measure;

  TextLines *result = (TextLines *)calloc(1, sizeof(TextLines));
  if (!result)
    return NULL;

  int text_len = strlen(text);
  if (text_len == 0)
    return result;

  int capacity = 4;
  result->lines = (char **)calloc(capacity, sizeof(char *));
  result->line_widths = (int *)calloc(capacity, sizeof(int));
  if (!result->lines || !result->line_widths) {
    layout_free_text_lines(result);
    return NULL;
  }

  const char *end = text + text_len;
  const char *p = text;

  while (p < end) {
    // Skip leading whitespace
    while (p < end && isspace((unsigned char)*p))
      p++;
    if (p >= end)
      break;

    // Growable line buffer
    int line_cap = 64;
    int line_len = 0;
    char *line = (char *)malloc(line_cap);
    if (!line)
      break;
    int width = 0;
    int words = 0;

    while (p < end) {
      if (*p == '\n') {
        p++;
        break;
      }
      // Skip inter-word whitespace
      while (p < end && isspace((unsigned char)*p) && *p != '\n')
        p++;
      if (p >= end || *p == '\n')
        break;

      const char *word_start = p;
      while (p < end && !isspace((unsigned char)*p) && *p != '\n')
        p++;
      size_t word_len = (size_t)(p - word_start);
      if (word_len == 0)
        break;

      char *word = (char *)malloc(word_len + 1);
      if (!word)
        break;
      memcpy(word, word_start, word_len);
      word[word_len] = '\0';

      int word_width = m(measure_ctx, word, font_size);
      int sep_width = words > 0 ? m(measure_ctx, " ", font_size) : 0;

      // Start a new line if this word doesn't fit (unless line is empty)
      if (words > 0 && width + sep_width + word_width > max_width) {
        free(word);
        break;
      }

      // Append separator + word to line
      if (words > 0) {
        if (line_len + 1 >= line_cap) {
          line_cap *= 2;
          char *bigger = (char *)realloc(line, line_cap);
          if (!bigger) {
            free(line);
            free(word);
            goto wrap_done;
          }
          line = bigger;
        }
        line[line_len++] = ' ';
      }
      if (line_len + (int)word_len >= line_cap) {
        while (line_len + (int)word_len >= line_cap)
          line_cap *= 2;
        char *bigger = (char *)realloc(line, line_cap);
        if (!bigger) {
          free(line);
          free(word);
          goto wrap_done;
        }
        line = bigger;
      }
      memcpy(line + line_len, word, word_len);
      line_len += (int)word_len;

      width += sep_width + word_width;
      words++;
      free(word);
    }

    line[line_len] = '\0';

    if (result->line_count >= capacity) {
      capacity *= 2;
      char **new_lines =
          (char **)realloc(result->lines, capacity * sizeof(char *));
      int *new_widths = (int *)realloc(result->line_widths, capacity * sizeof(int));
      if (!new_lines || !new_widths) {
        free(line);
        goto wrap_done;
      }
      result->lines = new_lines;
      result->line_widths = new_widths;
    }

    result->lines[result->line_count] = line;
    result->line_widths[result->line_count] = width;
    result->line_count++;
  }

wrap_done:
  return result;
}

// Free text lines
void layout_free_text_lines(TextLines *lines) {
  if (!lines)
    return;

  for (int i = 0; i < lines->line_count; i++) {
    free(lines->lines[i]);
  }
  free(lines->lines);
  free(lines->line_widths);
  free(lines);
}

// Calculate dimensions for a node
void layout_calculate_dimensions(LayoutNode *node, int available_width,
                                 FdmMeasureFn measure, void *measure_ctx) {
  if (!node)
    return;

  FdmMeasureFn m = measure ? measure : default_measure;

  // Calculate content width
  if (node->box.width_auto) {
    int inner_width = available_width - node->box.margin.left -
                      node->box.margin.right - node->box.padding.left -
                      node->box.padding.right - node->box.border.left -
                      node->box.border.right;

    if (node->box.is_block) {
      // Block elements take full available width
      node->box.width = inner_width > 0 ? inner_width : available_width;
    } else {
      // Inline elements fit to content
      if (node->text_content) {
        node->box.width =
            m(measure_ctx, node->text_content, node->box.font_size);
      } else {
        node->box.width = 0;
      }
    }
  }

  // Calculate content height
  if (node->box.height_auto) {
    if (node->text_content) {
      // Wrap text and calculate height
      int content_width = node->box.width > 0 ? node->box.width : available_width;
      TextLines *lines = layout_wrap_text(node->text_content, content_width,
                                          node->box.font_size, m, measure_ctx);
      if (lines) {
        node->box.height = lines->line_count * node->box.line_height;
        layout_free_text_lines(lines);
      } else {
        node->box.height = node->box.line_height;
      }
    } else if (node->first_child) {
      // Calculate height based on children
      int child_height = 0;
      LayoutNode *child = node->first_child;
      while (child) {
        layout_calculate_dimensions(child, node->box.width, m, measure_ctx);
        child_height += layout_get_total_height(&child->box);
        child = child->next_sibling;
      }
      node->box.height = child_height;
    } else {
      node->box.height = node->box.line_height;
    }
  }
}

// Position node and children
void layout_position_node(LayoutNode *node, int parent_x, int parent_y) {
  if (!node)
    return;

  node->box.x = parent_x + node->box.margin.left;
  node->box.y = parent_y + node->box.margin.top;

  if (node->first_child) {
    int child_x = node->box.x + node->box.padding.left + node->box.border.left;
    int child_y = node->box.y + node->box.padding.top + node->box.border.top;

    LayoutNode *child = node->first_child;
    while (child) {
      layout_position_node(child, child_x, child_y);

      // Move down for next block-level child
      if (child->box.is_block) {
        child_y += layout_get_total_height(&child->box);
      } else {
        // Inline elements flow horizontally
        child_x += layout_get_total_width(&child->box);
      }

      child = child->next_sibling;
    }
  }
}

// ---- Painting ----

static bool node_draws_text(ElementType type) {
  switch (type) {
  case ELEMENT_PARAGRAPH:
  case ELEMENT_HEADING1:
  case ELEMENT_HEADING2:
  case ELEMENT_HEADING3:
  case ELEMENT_HEADING4:
  case ELEMENT_HEADING5:
  case ELEMENT_HEADING6:
  case ELEMENT_LINK:
  case ELEMENT_BUTTON:
  case ELEMENT_SPAN:
  case ELEMENT_STRONG:
  case ELEMENT_EM:
  case ELEMENT_BOLD:
  case ELEMENT_ITALIC:
  case ELEMENT_UNDERLINE:
  case ELEMENT_LIST_ITEM:
    return true;
  default:
    return false;
  }
}

static void emit_fill(FdmSession *session, int x, int y, int w, int h,
                      FdmColor color) {
  FdmSurface *surface = session->surface;
  if (!surface || !surface->ops || !surface->ops->fill_rect)
    return;

  int vy = y - session->scroll_y;
  if (vy >= session->viewport_h || vy + h <= 0 || x >= session->viewport_w ||
      x + w <= 0)
    return;
  surface->ops->fill_rect(surface, x, vy, w, h, color);
}

static void emit_fill_gradient(FdmSession *session, int x, int y, int w, int h,
                               const FdmLinearGradient *gradient) {
  FdmSurface *surface = session->surface;
  if (!surface || !surface->ops || !surface->ops->fill_rect_gradient)
    return;

  int vy = y - session->scroll_y;
  if (vy >= session->viewport_h || vy + h <= 0 || x >= session->viewport_w ||
      x + w <= 0)
    return;
  surface->ops->fill_rect_gradient(surface, x, vy, w, h, gradient);
}

static void emit_text(FdmSession *session, const char *text, int x, int y,
                      int max_width, int font_size, FdmColor color, int align,
                      bool underline) {
  FdmSurface *surface = session->surface;
  if (!surface || !surface->ops || !surface->ops->draw_text)
    return;

  int vy = y - session->scroll_y;
  if (vy >= session->viewport_h || x >= session->viewport_w)
    return;
  surface->ops->draw_text(surface, text, x, vy, max_width, font_size, color,
                          align, underline);
}

static void emit_border(FdmSession *session, int x, int y, int w, int h,
                        int border, FdmColor color) {
  if (border <= 0)
    return;
  emit_fill(session, x, y, w, border, color); // top
  emit_fill(session, x, y + h - border, w, border, color); // bottom
  emit_fill(session, x, y, border, h, color); // left
  emit_fill(session, x + w - border, y, border, h, color); // right
}

static void paint_text_node(FdmSession *session, LayoutNode *node) {
  FdmSurface *surface = session->surface;
  if (!surface || !surface->ops || !surface->ops->draw_text)
    return;

  if (!node->text_content || node->text_content[0] == '\0')
    return;

  int font_size = node->box.font_size;
  int line_height = node->box.line_height;
  int max_width = node->box.width > 0 ? node->box.width : session->viewport_w;

  // Background covers the whole node box (drawn first so text sits on top).
  FdmLinearGradient *gradient =
      (node->box.background.type == BACKGROUND_FILL_LINEAR_GRADIENT)
          ? &node->box.background.data.linear
          : NULL;
  if (gradient) {
    emit_fill_gradient(session, node->box.x, node->box.y, node->box.width,
                       node->box.height, gradient);
  } else if (node->box.has_explicit_bg_color) {
    emit_fill(session, node->box.x, node->box.y, node->box.width,
              node->box.height, node->box.bg_color);
  }

  // Wrap using the surface's measure callback
  TextLines *lines =
      layout_wrap_text(node->text_content, max_width, font_size,
                       measure_surface, surface);

  int region_left = 0, region_top = 0, region_right = 0, region_bottom = 0;
  bool region_valid = false;

  bool underline = (node->type == ELEMENT_LINK ||
                    node->type == ELEMENT_UNDERLINE);

  int line_y = node->box.y;
  for (int i = 0; i < lines->line_count; i++) {
    const char *line = lines->lines[i];
    int line_width = lines->line_widths[i];

    int line_x = node->box.x;
    if (node->box.text_align == FDM_ALIGN_CENTER) {
      line_x = node->box.x + (max_width - line_width) / 2;
    } else if (node->box.text_align == FDM_ALIGN_RIGHT) {
      line_x = node->box.x + max_width - line_width;
    }
    if (line_x < node->box.x)
      line_x = node->box.x;

    FdmColor color = node->box.has_explicit_color ? node->box.color : 0xF0F6FC;
    if (node->type == ELEMENT_LINK)
      color = 0x4EA1FF;

    if ((int)session->run_count == session->sel_run) {
      emit_fill(session, line_x, line_y, line_width, line_height, 0x1F6FEB);
    }

    emit_text(session, line, line_x, line_y, line_width, font_size, color,
              FDM_ALIGN_LEFT, underline);
    fdm_session_add_run(session, line_x, line_y, line_width, line_height,
                        font_size, color, underline, line);

    if (!region_valid) {
      region_left = line_x;
      region_top = line_y;
      region_right = line_x + line_width;
      region_bottom = line_y + line_height;
      region_valid = true;
    } else {
      if (line_x < region_left)
        region_left = line_x;
      if (line_y < region_top)
        region_top = line_y;
      if (line_x + line_width > region_right)
        region_right = line_x + line_width;
      if (line_y + line_height > region_bottom)
        region_bottom = line_y + line_height;
    }

    line_y += line_height;
  }

  layout_free_text_lines(lines);

  if (node->type == ELEMENT_LINK && region_valid) {
    const char *link_target =
        node->href_resolved ? node->href_resolved : node->href;
    if (link_target && link_target[0] != '\0') {
      fdm_session_add_region(
          session, region_left, region_top, region_right - region_left,
          region_bottom - region_top, false, link_target, NULL, NULL,
          font_size, node);
    }
  }
}

static void paint_input_node(FdmSession *session, LayoutNode *node,
                             bool is_textarea) {
  FdmSurface *surface = session->surface;
  if (!surface || !surface->ops || !surface->ops->draw_text)
    return;

  int x = node->box.x;
  int y = node->box.y;
  int w = node->box.width;
  int h = node->box.height;

  // Box + border
  emit_fill(session, x, y, w, h, 0x141414);
  bool focused = (int)session->region_count == session->focus_region;
  FdmColor border_color = focused ? 0x4EA1FF : 0x2D333B;
  emit_border(session, x, y, w, h, 1, border_color);

  const char *value = node->form_value ? node->form_value : "";
  const char *display = (value[0] != '\0') ? value
                        : (node->placeholder ? node->placeholder : "");
  FdmColor text_color = (value[0] != '\0') ? 0xF0F6FC : 0x7D8590;

  int text_x = x + node->box.padding.left;
  int text_y = y + node->box.padding.top;

  if (display[0] != '\0' && surface->ops->measure_text) {
    // Truncate long values to fit
    int avail = w - node->box.padding.left - node->box.padding.right;
    // Find how many chars fit
    int n = 0;
    int total = 0;
    while (display[n] != '\0') {
      char single[2] = {display[n], '\0'};
      total += surface->ops->measure_text(surface, single, node->box.font_size);
      if (total > avail && n > 0)
        break;
      n++;
    }
    char *clipped = fdm_strndup(display, (size_t)n);
    if (clipped) {
      emit_text(session, clipped, text_x, text_y, avail,
                node->box.font_size, text_color, FDM_ALIGN_LEFT, false);
      free(clipped);
    }
  }

  fdm_session_add_region(session, x, y, w, h, true, NULL,
                         node->form_value, node->placeholder,
                         node->box.font_size, node);
  (void)is_textarea;
}

static void paint_button_node(FdmSession *session, LayoutNode *node) {
  emit_fill(session, node->box.x, node->box.y, node->box.width,
            node->box.height, 0x21262D);
  emit_border(session, node->box.x, node->box.y, node->box.width,
              node->box.height, 1, 0x2D333B);

  if (!node->text_content || node->text_content[0] == '\0')
    return;

  int text_y = node->box.y + node->box.padding.top;
  emit_text(session, node->text_content, node->box.x + node->box.padding.left,
            text_y, node->box.width - node->box.padding.left -
                         node->box.padding.right,
            node->box.font_size, 0xF0F6FC, FDM_ALIGN_LEFT, false);
}

static void paint_node(FdmSession *session, LayoutNode *node) {
  if (!node)
    return;

  switch (node->type) {
  case ELEMENT_INPUT_TEXT:
    paint_input_node(session, node, false);
    break;
  case ELEMENT_TEXTAREA:
    paint_input_node(session, node, true);
    break;
  case ELEMENT_BUTTON:
    paint_button_node(session, node);
    break;
  default:
    if (node->text_content && node_draws_text(node->type)) {
      paint_text_node(session, node);
    } else if (node->box.background.type == BACKGROUND_FILL_LINEAR_GRADIENT) {
      emit_fill_gradient(session, node->box.x, node->box.y, node->box.width,
                         node->box.height, &node->box.background.data.linear);
    } else if (node->box.has_explicit_bg_color) {
      emit_fill(session, node->box.x, node->box.y, node->box.width,
                node->box.height, node->box.bg_color);
    }
    break;
  }

  LayoutNode *child = node->first_child;
  while (child) {
    paint_node(session, child);
    child = child->next_sibling;
  }
}

// Run layout (dimensions + positioning) for a session's document.
void fdm_layout_document(FdmSession *session) {
  if (!session || !session->root)
    return;

  FdmMeasureFn measure = NULL;
  void *ctx = NULL;
  FdmSurface *surface = session->surface;
  if (surface && surface->ops && surface->ops->measure_text) {
    measure = measure_surface;
    ctx = surface;
  }

  layout_calculate_dimensions(session->root, session->viewport_w, measure, ctx);
  layout_position_node(session->root, 0, 10);
}

// Paint a session's document onto its surface.
void fdm_paint_document(FdmSession *session) {
  if (!session)
    return;
  FdmSurface *surface = session->surface;
  if (!surface || !surface->ops || !surface->ops->begin_frame ||
      !surface->ops->end_frame)
    return;

  surface->ops->begin_frame(surface, session->viewport_w, session->viewport_h);

  fdm_session_reset_interaction(session);

  // Default page background
  if (surface->ops->fill_rect) {
    surface->ops->fill_rect(surface, 0, 0, session->viewport_w,
                            session->viewport_h, 0xFFFFFF);
  }

  if (session->root) {
    paint_node(session, session->root);
  }

  surface->ops->end_frame(surface);
}
