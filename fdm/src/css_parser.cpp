#include "css_parser.h"
#include "fdm.h"
#include <ctype.h>
#include <lexbor/css/css.h>
#include <lexbor/css/declaration.h>
#include <lexbor/css/property.h>
#include <lexbor/css/rule.h>
#include <lexbor/css/stylesheet.h>
#include <lexbor/css/value.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

// Static parser instance for stylesheet parsing
static lxb_css_parser_t *css_parser = NULL;
static lxb_css_memory_t *css_memory = NULL;
typedef struct CssRuleEntry {
  char *selector;
  size_t length;
  bool case_insensitive;
  char *declarations;
  struct CssRuleEntry *next;
} CssRuleEntry;

static CssRuleEntry *css_rules_head = NULL;

static void css_rules_clear(void) {
  CssRuleEntry *entry = css_rules_head;
  while (entry) {
    CssRuleEntry *next = entry->next;
    free(entry->selector);
    free(entry->declarations);
    free(entry);
    entry = next;
  }
  css_rules_head = NULL;
}

static bool css_parser_prepare_memory(void) {
  if (!css_memory) {
    css_memory = lxb_css_memory_create();
    if (!css_memory) {
      return false;
    }
    if (lxb_css_memory_init(css_memory, 128) != LXB_STATUS_OK) {
      lxb_css_memory_destroy(css_memory, true);
      css_memory = NULL;
      return false;
    }
  } else {
    lxb_css_memory_clean(css_memory);
  }
  return true;
}

static char *copy_trimmed_range(const char *start, const char *end) {
  if (!start || !end || end <= start)
    return NULL;
  const char *trimmed_start = start;
  const char *trimmed_end = end;

  while (trimmed_start < trimmed_end &&
         isspace((unsigned char)*trimmed_start)) {
    trimmed_start++;
  }
  while (trimmed_end > trimmed_start &&
         isspace((unsigned char)*(trimmed_end - 1))) {
    trimmed_end--;
  }

  size_t length = (size_t)(trimmed_end - trimmed_start);
  if (length == 0)
    return NULL;

  char *buffer = (char *)malloc(length + 1);
  if (!buffer)
    return NULL;
  memcpy(buffer, trimmed_start, length);
  buffer[length] = '\0';
  return buffer;
}

static bool selector_is_simple(const char *selector) {
  if (!selector)
    return false;
  for (const char *c = selector; *c; ++c) {
    if (isspace((unsigned char)*c) || *c == '>' || *c == '+' || *c == '~' ||
        *c == '[' || *c == ':' || *c == '*') {
      return false;
    }
  }
  return true;
}

static const char *skip_whitespace_and_comments(const char *ptr,
                                                const char *end) {
  while (ptr < end) {
    if (isspace((unsigned char)*ptr)) {
      ptr++;
      continue;
    }

    if (*ptr == '/' && (ptr + 1) < end && ptr[1] == '*') {
      ptr += 2;
      while (ptr < end && !(*ptr == '*' && (ptr + 1) < end && ptr[1] == '/')) {
        ptr++;
      }
      if (ptr < end)
        ptr += 2;
      continue;
    }
    break;
  }
  return ptr;
}

static const char *skip_string_literal(const char *ptr, const char *end,
                                       char quote) {
  if (*ptr != quote)
    return ptr;
  ptr++;
  while (ptr < end) {
    if (*ptr == '\\' && (ptr + 1) < end) {
      ptr += 2;
      continue;
    }
    if (*ptr == quote) {
      ptr++;
      break;
    }
    ptr++;
  }
  return ptr;
}

static void css_rules_store_entry(char *selector, size_t selector_len,
                                  char *declarations, bool case_insensitive) {
  if (!selector || selector_len == 0 || !declarations) {
    free(selector);
    free(declarations);
    return;
  }

  CssRuleEntry *entry = css_rules_head;
  while (entry) {
    if (entry->length == selector_len &&
        entry->case_insensitive == case_insensitive) {
      bool match = false;
      if (case_insensitive) {
        match = (strncasecmp(entry->selector, selector, selector_len) == 0);
      } else {
        match = (strncmp(entry->selector, selector, selector_len) == 0);
      }

      if (match) {
        size_t existing_len = strlen(entry->declarations);
        size_t new_len = strlen(declarations);
        size_t needs_semicolon =
            (existing_len > 0 && entry->declarations[existing_len - 1] != ';')
                ? 1
                : 0;
        char *buffer = (char *)realloc(
            entry->declarations, existing_len + needs_semicolon + new_len + 1);
        if (!buffer) {
          free(selector);
          free(declarations);
          return;
        }
        entry->declarations = buffer;
        size_t offset = existing_len;
        if (needs_semicolon) {
          entry->declarations[offset++] = ';';
        }
        memcpy(entry->declarations + offset, declarations, new_len + 1);
        free(selector);
        free(declarations);
        return;
      }
    }
    entry = entry->next;
  }

  CssRuleEntry *new_entry = (CssRuleEntry *)calloc(1, sizeof(CssRuleEntry));
  if (!new_entry) {
    free(selector);
    free(declarations);
    return;
  }

  new_entry->selector = selector;
  new_entry->length = selector_len;
  new_entry->case_insensitive = case_insensitive;
  new_entry->declarations = declarations;
  new_entry->next = css_rules_head;
  css_rules_head = new_entry;
}

// Helper function to convert Lexbor color to uint32_t
static uint32_t lexbor_color_to_uint32(const lxb_css_value_color_t *color) {
  if (!color)
    return 0;

  switch (color->type) {
  case LXB_CSS_VALUE_HEX: {
    const lxb_css_value_color_hex_rgba_t *rgba = &color->u.hex.rgba;
    return ((uint32_t)rgba->r << 16) | ((uint32_t)rgba->g << 8) |
           (uint32_t)rgba->b;
  }
  case LXB_CSS_VALUE_RGB: {
    const lxb_css_value_color_rgba_t *rgba = &color->u.rgb;
    // Convert percentage/number values to 0-255 range
    uint8_t r = (rgba->r.type == LXB_CSS_VALUE__PERCENTAGE)
                    ? (uint8_t)((rgba->r.u.percentage.num / 100.0) * 255.0)
                    : (uint8_t)rgba->r.u.number.num;
    uint8_t g = (rgba->g.type == LXB_CSS_VALUE__PERCENTAGE)
                    ? (uint8_t)((rgba->g.u.percentage.num / 100.0) * 255.0)
                    : (uint8_t)rgba->g.u.number.num;
    uint8_t b = (rgba->b.type == LXB_CSS_VALUE__PERCENTAGE)
                    ? (uint8_t)((rgba->b.u.percentage.num / 100.0) * 255.0)
                    : (uint8_t)rgba->b.u.number.num;
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
  }
  case LXB_CSS_VALUE_CURRENTCOLOR:
    // Return a default color for currentColor (could be made configurable)
    return 0x000000;
  case LXB_CSS_VALUE_INHERIT:
  case LXB_CSS_VALUE_INITIAL:
  case LXB_CSS_VALUE_UNSET:
  default:
    return 0x000000;
  }
}

bool css_parser_init(void) {
  if (css_parser)
    return true;

  css_parser = lxb_css_parser_create();
  if (!css_parser)
    return false;

  lxb_status_t status = lxb_css_parser_init(css_parser, NULL);
  if (status != LXB_STATUS_OK) {
    lxb_css_parser_destroy(css_parser, true);
    css_parser = NULL;
    return false;
  }

  return true;
}

void css_parser_reset(void) { css_rules_clear(); }

void css_parser_cleanup(void) {
  css_rules_clear();
  if (css_parser) {
    lxb_css_parser_destroy(css_parser, true);
    css_parser = NULL;
  }
  if (css_memory) {
    lxb_css_memory_destroy(css_memory, true);
    css_memory = NULL;
  }
}

void css_parser_add_stylesheet(const char *css, size_t length) {
  if (!css_parser || !css || length == 0)
    return;

  const char *ptr = css;
  const char *end = css + length;

  while (ptr < end) {
    ptr = skip_whitespace_and_comments(ptr, end);
    if (ptr >= end)
      break;

    const char *selector_start = ptr;
    while (ptr < end && *ptr != '{') {
      if (*ptr == '\"' || *ptr == '\'') {
        ptr = skip_string_literal(ptr, end, *ptr);
        continue;
      }
      if (*ptr == '/' && (ptr + 1) < end && ptr[1] == '*') {
        ptr += 2;
        while (ptr < end &&
               !(*ptr == '*' && (ptr + 1) < end && ptr[1] == '/')) {
          ptr++;
        }
        if (ptr < end)
          ptr += 2;
        continue;
      }
      ptr++;
    }

    if (ptr >= end)
      break;

    const char *selector_end = ptr;
    ptr++; // Skip '{'

    const char *block_start = ptr;
    int depth = 1;
    while (ptr < end && depth > 0) {
      if (*ptr == '\"' || *ptr == '\'') {
        ptr = skip_string_literal(ptr, end, *ptr);
        continue;
      }
      if (*ptr == '/' && (ptr + 1) < end && ptr[1] == '*') {
        ptr += 2;
        while (ptr < end &&
               !(*ptr == '*' && (ptr + 1) < end && ptr[1] == '/')) {
          ptr++;
        }
        if (ptr < end)
          ptr += 2;
        continue;
      }
      if (*ptr == '{') {
        depth++;
        ptr++;
        continue;
      }
      if (*ptr == '}') {
        depth--;
        if (depth == 0) {
          break;
        }
      }
      ptr++;
    }

    const char *block_end = ptr;
    if (ptr < end && *ptr == '}') {
      ptr++;
    }

    char *declarations = copy_trimmed_range(block_start, block_end);
    if (!declarations) {
      continue;
    }

    bool block_consumed = false;
    const char *sel_ptr = selector_start;
    while (sel_ptr < selector_end) {
      const char *comma = sel_ptr;
      while (comma < selector_end && *comma != ',') {
        comma++;
      }

      char *selector = copy_trimmed_range(sel_ptr, comma);
      if (selector) {
        if (selector[0] != '@' && selector_is_simple(selector)) {
          bool case_insensitive = selector[0] != '.' && selector[0] != '#';
          if (case_insensitive) {
            for (char *c = selector; *c; ++c) {
              *c = (char)tolower((unsigned char)*c);
            }
          }

          char *block_value = NULL;
          if (!block_consumed) {
            block_value = declarations;
            block_consumed = true;
          } else {
            block_value = fdm_strdup(declarations);
          }

          if (block_value) {
            css_rules_store_entry(selector, strlen(selector), block_value,
                                  case_insensitive);
          } else {
            free(selector);
          }
        } else {
          free(selector);
        }
      }

      sel_ptr = (comma < selector_end) ? comma + 1 : selector_end;
    }

    if (!block_consumed) {
      free(declarations);
    }
  }
}

const char *css_parser_get_declarations(const char *selector, size_t length) {
  if (!selector || length == 0)
    return NULL;

  CssRuleEntry *entry = css_rules_head;
  while (entry) {
    if (entry->length == length) {
      if (entry->case_insensitive) {
        if (strncasecmp(entry->selector, selector, length) == 0) {
          return entry->declarations;
        }
      } else {
        if (strncmp(entry->selector, selector, length) == 0) {
          return entry->declarations;
        }
      }
    }
    entry = entry->next;
  }

  return NULL;
}

bool css_parser_parse_color_value(const char *value, FdmColor *color_out) {
  if (!value || !color_out || !css_parser) {
    return false;
  }

  if (!css_parser_prepare_memory()) {
    return false;
  }

  const char *prefix = "color:";
  const char *suffix = ";";
  size_t value_len = strlen(value);
  size_t buffer_len = strlen(prefix) + value_len + strlen(suffix) + 1;

  char *buffer = (char *)malloc(buffer_len);
  if (!buffer) {
    return false;
  }

  snprintf(buffer, buffer_len, "%s%s%s", prefix, value, suffix);

  lxb_css_rule_declaration_list_t *decl_list = lxb_css_declaration_list_parse(
      css_parser, css_memory, (const lxb_char_t *)buffer, strlen(buffer));

  free(buffer);

  if (!decl_list) {
    lxb_css_memory_clean(css_memory);
    return false;
  }

  bool parsed = false;
  lxb_css_rule_t *rule = decl_list->first;
  while (rule) {
    if (rule->type == LXB_CSS_RULE_DECLARATION) {
      lxb_css_rule_declaration_t *decl = (lxb_css_rule_declaration_t *)rule;
      const lxb_css_value_color_t *color_value = NULL;

      if (decl->type == LXB_CSS_PROPERTY_COLOR && decl->u.color) {
        color_value = decl->u.color;
      } else if (decl->type == LXB_CSS_PROPERTY_BACKGROUND_COLOR &&
                 decl->u.background_color) {
        color_value = decl->u.background_color;
      }

      if (color_value) {
        *color_out = lexbor_color_to_uint32(color_value);
        parsed = true;
        break;
      }
    }
    rule = rule->next;
  }

  lxb_css_rule_declaration_list_destroy(decl_list, true);
  lxb_css_memory_clean(css_memory);
  return parsed;
}

