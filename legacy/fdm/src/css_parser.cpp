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

typedef struct {
  const char *name;
  FdmColor color;
} NamedColorEntry;

static const NamedColorEntry named_colors[] = {
    {"aliceblue", 0xF0F8FF}, {"antiquewhite", 0xFAEBD7},
    {"aqua", 0x00FFFF},       {"aquamarine", 0x7FFFD4},
    {"azure", 0xF0FFFF},      {"beige", 0xF5F5DC},
    {"bisque", 0xFFE4C4},     {"black", 0x000000},
    {"blanchedalmond", 0xFFEBCD}, {"blue", 0x0000FF},
    {"blueviolet", 0x8A2BE2}, {"brown", 0xA52A2A},
    {"burlywood", 0xDEB887},  {"cadetblue", 0x5F9EA0},
    {"chartreuse", 0x7FFF00}, {"chocolate", 0xD2691E},
    {"coral", 0xFF7F50},      {"cornflowerblue", 0x6495ED},
    {"cornsilk", 0xFFF8DC},   {"crimson", 0xDC143C},
    {"cyan", 0x00FFFF},       {"darkblue", 0x00008B},
    {"darkcyan", 0x008B8B},   {"darkgoldenrod", 0xB8860B},
    {"darkgray", 0xA9A9A9},   {"darkgreen", 0x006400},
    {"darkgrey", 0xA9A9A9},   {"darkkhaki", 0xBDB76B},
    {"darkmagenta", 0x8B008B}, {"darkolivegreen", 0x556B2F},
    {"darkorange", 0xFF8C00}, {"darkorchid", 0x9932CC},
    {"darkred", 0x8B0000},    {"darksalmon", 0xE9967A},
    {"darkseagreen", 0x8FBC8F}, {"darkslateblue", 0x483D8B},
    {"darkslategray", 0x2F4F4F}, {"darkslategrey", 0x2F4F4F},
    {"darkturquoise", 0x00CED1}, {"darkviolet", 0x9400D3},
    {"deeppink", 0xFF1493},   {"deepskyblue", 0x00BFFF},
    {"dimgray", 0x696969},    {"dimgrey", 0x696969},
    {"dodgerblue", 0x1E90FF}, {"firebrick", 0xB22222},
    {"floralwhite", 0xFFFAF0}, {"forestgreen", 0x228B22},
    {"fuchsia", 0xFF00FF},    {"gainsboro", 0xDCDCDC},
    {"ghostwhite", 0xF8F8FF}, {"gold", 0xFFD700},
    {"goldenrod", 0xDAA520},  {"gray", 0x808080},
    {"green", 0x008000},      {"greenyellow", 0xADFF2F},
    {"grey", 0x808080},       {"honeydew", 0xF0FFF0},
    {"hotpink", 0xFF69B4},    {"indianred", 0xCD5C5C},
    {"indigo", 0x4B0082},     {"ivory", 0xFFFFF0},
    {"khaki", 0xF0E68C},      {"lavender", 0xE6E6FA},
    {"lavenderblush", 0xFFF0F5}, {"lawngreen", 0x7CFC00},
    {"lemonchiffon", 0xFFFACD}, {"lightblue", 0xADD8E6},
    {"lightcoral", 0xF08080}, {"lightcyan", 0xE0FFFF},
    {"lightgoldenrodyellow", 0xFAFAD2}, {"lightgray", 0xD3D3D3},
    {"lightgreen", 0x90EE90}, {"lightgrey", 0xD3D3D3},
    {"lightpink", 0xFFB6C1},  {"lightsalmon", 0xFFA07A},
    {"lightseagreen", 0x20B2AA}, {"lightskyblue", 0x87CEFA},
    {"lightslategray", 0x778899}, {"lightslategrey", 0x778899},
    {"lightsteelblue", 0xB0C4DE}, {"lightyellow", 0xFFFFE0},
    {"lime", 0x00FF00},       {"limegreen", 0x32CD32},
    {"linen", 0xFAF0E6},      {"magenta", 0xFF00FF},
    {"maroon", 0x800000},     {"mediumaquamarine", 0x66CDAA},
    {"mediumblue", 0x0000CD}, {"mediumorchid", 0xBA55D3},
    {"mediumpurple", 0x9370DB}, {"mediumseagreen", 0x3CB371},
    {"mediumslateblue", 0x7B68EE}, {"mediumspringgreen", 0x00FA9A},
    {"mediumturquoise", 0x48D1CC}, {"mediumvioletred", 0xC71585},
    {"midnightblue", 0x191970}, {"mintcream", 0xF5FFFA},
    {"mistyrose", 0xFFE4E1},  {"moccasin", 0xFFE4B5},
    {"navajowhite", 0xFFDEAD}, {"navy", 0x000080},
    {"oldlace", 0xFDF5E6},    {"olive", 0x808000},
    {"olivedrab", 0x6B8E23},  {"orange", 0xFFA500},
    {"orangered", 0xFF4500},  {"orchid", 0xDA70D6},
    {"palegoldenrod", 0xEEE8AA}, {"palegreen", 0x98FB98},
    {"paleturquoise", 0xAFEEEE}, {"palevioletred", 0xDB7093},
    {"papayawhip", 0xFFEFD5}, {"peachpuff", 0xFFDAB9},
    {"peru", 0xCD853F},       {"pink", 0xFFC0CB},
    {"plum", 0xDDA0DD},       {"powderblue", 0xB0E0E6},
    {"purple", 0x800080},     {"rebeccapurple", 0x663399},
    {"red", 0xFF0000},        {"rosybrown", 0xBC8F8F},
    {"royalblue", 0x4169E1},  {"saddlebrown", 0x8B4513},
    {"salmon", 0xFA8072},     {"sandybrown", 0xF4A460},
    {"seagreen", 0x2E8B57},   {"seashell", 0xFFF5EE},
    {"sienna", 0xA0522D},     {"silver", 0xC0C0C0},
    {"skyblue", 0x87CEEB},    {"slateblue", 0x6A5ACD},
    {"slategray", 0x708090},  {"slategrey", 0x708090},
    {"snow", 0xFFFAFA},       {"springgreen", 0x00FF7F},
    {"steelblue", 0x4682B4},  {"tan", 0xD2B48C},
    {"teal", 0x008080},       {"thistle", 0xD8BFD8},
    {"tomato", 0xFF6347},     {"transparent", 0x000000},
    {"turquoise", 0x40E0D0},  {"violet", 0xEE82EE},
    {"wheat", 0xF5DEB3},      {"white", 0xFFFFFF},
    {"whitesmoke", 0xF5F5F5}, {"yellow", 0xFFFF00},
    {"yellowgreen", 0x9ACD32},
};

static const size_t named_color_count =
    sizeof(named_colors) / sizeof(named_colors[0]);

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
    return 0x000000;
  default: {
    // Named color keywords: lexbor reports the keyword id but not its RGB;
    // map the keyword name through the standard CSS named-color table.
    const lxb_css_data_t *data = lxb_css_value_by_id((uintptr_t)color->type);
    if (data && data->name && data->length > 0 && data->length < 32) {
      char name[32];
      for (size_t i = 0; i < data->length; ++i) {
        char c = (char)data->name[i];
        if (c >= 'A' && c <= 'Z')
          c = (char)(c + 32);
        name[i] = c;
      }
      name[data->length] = '\0';
      for (size_t i = 0; i < named_color_count; ++i) {
        if (strcmp(name, named_colors[i].name) == 0) {
          return named_colors[i].color;
        }
      }
    }
    return 0x000000;
  }
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

