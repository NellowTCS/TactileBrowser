#include "css_parser.h"
#include "tactilebrowser_core.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

// Color table for named colors
const ColorEntry color_table[] = {
    {"black", 0x000000},   {"silver", 0xC0C0C0}, {"gray", 0x808080},
    {"white", 0xFFFFFF},   {"maroon", 0x800000}, {"red", 0xFF0000},
    {"purple", 0x800080},  {"fuchsia", 0xFF00FF}, {"green", 0x008000},
    {"lime", 0x00FF00},    {"olive", 0x808000}, {"yellow", 0xFFFF00},
    {"navy", 0x000080},    {"blue", 0x0000FF},  {"teal", 0x008080},
    {"aqua", 0x00FFFF},    {"cyan", 0x00FFFF},  {"magenta", 0xFF00FF},
    {"orange", 0xFFA500},  {"brown", 0xA52A2A}, {"pink", 0xFFC0CB},
    {"orchid", 0xDA70D6},  {"salmon", 0xFA8072}, {"indigo", 0x4B0082},
    {"gold", 0xFFD700},    {"lightgray", 0xD3D3D3}, {"darkgray", 0xA9A9A9}
};

const size_t color_table_size = sizeof(color_table) / sizeof(color_table[0]);

static bool strings_equal_case_insensitive(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) {
            return false;
        }
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static bool starts_with_case_insensitive(const char* text, const char* prefix) {
    if (!text || !prefix) return false;
    while (*prefix && *text) {
        if (tolower((unsigned char)*text) != tolower((unsigned char)*prefix)) {
            return false;
        }
        ++text;
        ++prefix;
    }
    return *prefix == '\0';
}

static void trim_whitespace_inplace(char* text) {
    if (!text) return;
    char* start = text;
    while (*start && isspace((unsigned char)*start)) start++;
    char* end = start + strlen(start);
    while (end > start && isspace((unsigned char)*(end - 1))) end--;
    size_t len = (size_t)(end - start);
    if (start != text) {
        memmove(text, start, len);
    }
    text[len] = '\0';
}

static int hex_digit(char ch) {
    if (ch >= '0' && ch <= '9') return ch - '0';
    ch = (char)tolower((unsigned char)ch);
    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

static bool parse_hex_color_value(const char* text, uint32_t* color) {
    if (!text || text[0] != '#') return false;
    size_t len = strlen(text);
    if (len != 4 && len != 7) return false;

    auto read_component = [](char hi, char lo) -> int {
        int high = hex_digit(hi);
        int low = hex_digit(lo);
        if (high < 0 || low < 0) return -1;
        return (high << 4) | low;
    };

    uint32_t r = 0, g = 0, b = 0;
    if (len == 4) {
        int rh = hex_digit(text[1]);
        int gh = hex_digit(text[2]);
        int bh = hex_digit(text[3]);
        if (rh < 0 || gh < 0 || bh < 0) return false;
        r = (uint32_t)(rh << 4 | rh);
        g = (uint32_t)(gh << 4 | gh);
        b = (uint32_t)(bh << 4 | bh);
    } else {
        int r_comp = read_component(text[1], text[2]);
        int g_comp = read_component(text[3], text[4]);
        int b_comp = read_component(text[5], text[6]);
        if (r_comp < 0 || g_comp < 0 || b_comp < 0) return false;
        r = (uint32_t)r_comp;
        g = (uint32_t)g_comp;
        b = (uint32_t)b_comp;
    }

    *color = (r << 16) | (g << 8) | b;
    return true;
}

static bool parse_rgb_component(const char* token, int* out_value) {
    if (!token || !out_value) return false;
    char* end = NULL;
    double value = strtod(token, &end);
    if (token == end) return false;
    bool is_percent = false;
    if (end && *end == '%') {
        is_percent = true;
        end++;
    }
    while (end && *end && isspace((unsigned char)*end)) end++;
    if (end && *end != '\0') return false;

    if (is_percent) {
        value = (value < 0.0 ? 0.0 : (value > 100.0 ? 100.0 : value));
        value = (value / 100.0) * 255.0;
    }

    if (value < 0.0) value = 0.0;
    if (value > 255.0) value = 255.0;
    *out_value = (int)(value + 0.5);
    return true;
}

static bool parse_alpha_component(const char* token, float* out_alpha) {
    if (!token || !out_alpha) return false;
    char* end = NULL;
    double value = strtod(token, &end);
    if (token == end) return false;

    if (end && *end == '%') {
        value = value / 100.0;
        end++;
    }
    while (end && *end && isspace((unsigned char)*end)) end++;
    if (end && *end != '\0') return false;

    if (value < 0.0) value = 0.0;
    if (value > 1.0) value = 1.0;
    *out_alpha = (float)value;
    return true;
}

static bool parse_rgb_or_rgba_color(const char* text, uint32_t* color) {
    if (!text) return false;
    const char* ptr = text;
    while (*ptr && isspace((unsigned char)*ptr)) ptr++;
    if (!starts_with_case_insensitive(ptr, "rgb")) return false;

    bool has_alpha = false;
    ptr += 3;
    if (*ptr == 'a' || *ptr == 'A') {
        has_alpha = true;
        ptr++;
    }

    while (*ptr && isspace((unsigned char)*ptr)) ptr++;
    if (*ptr != '(') return false;
    ptr++;

    const char* end = strchr(ptr, ')');
    if (!end) return false;
    size_t len = (size_t)(end - ptr);
    char* body = (char*)malloc(len + 1);
    if (!body) return false;
    memcpy(body, ptr, len);
    body[len] = '\0';

    for (char* ch = body; *ch; ++ch) {
        if (*ch == '/') *ch = ',';
    }

    int rgb[3] = {0};
    float alpha = 1.0f;
    int required = has_alpha ? 4 : 3;
    int parsed = 0;

    char* saveptr = NULL;
    char* token = strtok_r(body, ",", &saveptr);
    while (token && parsed < required) {
        trim_whitespace_inplace(token);
        if (*token == '\0') {
            token = strtok_r(NULL, ",", &saveptr);
            continue;
        }

        if (parsed < 3) {
            int component = 0;
            if (!parse_rgb_component(token, &component)) {
                free(body);
                return false;
            }
            rgb[parsed] = component;
        } else {
            if (!parse_alpha_component(token, &alpha)) {
                free(body);
                return false;
            }
        }

        parsed++;
        token = strtok_r(NULL, ",", &saveptr);
    }

    free(body);

    if (parsed < 3) return false;
    if (has_alpha && parsed < 4) {
        // explicitly provided rgba() but missing alpha; treat as opaque
        alpha = 1.0f;
    }

    if (alpha < 1.0f) {
        for (int i = 0; i < 3; ++i) {
            float scaled = (float)rgb[i] * alpha;
            if (scaled < 0.0f) scaled = 0.0f;
            if (scaled > 255.0f) scaled = 255.0f;
            rgb[i] = (int)(scaled + 0.5f);
        }
    }

    *color = ((uint32_t)rgb[0] << 16) | ((uint32_t)rgb[1] << 8) | (uint32_t)rgb[2];
    return true;
}

static bool parse_named_color_value(const char* text, uint32_t* color) {
    if (!text) return false;
    for (size_t i = 0; i < color_table_size; ++i) {
        if (strings_equal_case_insensitive(text, color_table[i].name)) {
            *color = color_table[i].color;
            return true;
        }
    }
    return false;
}

static bool parse_color_value(const char* text, uint32_t* color) {
    if (!text) return false;
    while (*text && isspace((unsigned char)*text)) text++;
    if (*text == '\0') return false;

    if (parse_hex_color_value(text, color)) return true;
    if (parse_rgb_or_rgba_color(text, color)) return true;
    if (parse_named_color_value(text, color)) return true;
    if (strings_equal_case_insensitive(text, "transparent")) {
        *color = 0x000000;
        return true;
    }
    return false;
}

typedef struct {
    char* selector;
    char* declarations;
    size_t declarations_len;
} CssRule;

static CssRule* css_rules = NULL;
static size_t css_rule_count = 0;
static size_t css_rule_capacity = 0;

static char* strndup_lower(const char* src, size_t len) {
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; ++i) {
        unsigned char ch = (unsigned char)src[i];
        out[i] = (char)tolower(ch);
    }
    out[len] = '\0';
    return out;
}

static void ensure_capacity(void) {
    if (css_rule_count < css_rule_capacity) return;
    size_t new_capacity = css_rule_capacity == 0 ? 8 : css_rule_capacity * 2;
    CssRule* resized = (CssRule*)realloc(css_rules, new_capacity * sizeof(CssRule));
    if (!resized) {
        return;
    }
    for (size_t i = css_rule_capacity; i < new_capacity; ++i) {
        resized[i].selector = NULL;
        resized[i].declarations = NULL;
        resized[i].declarations_len = 0;
    }
    css_rules = resized;
    css_rule_capacity = new_capacity;
}

static CssRule* find_rule(const char* selector, size_t length) {
    if (!selector || length == 0) return NULL;
    for (size_t i = 0; i < css_rule_count; ++i) {
        if (css_rules[i].selector && strlen(css_rules[i].selector) == length &&
            strncmp(css_rules[i].selector, selector, length) == 0) {
            return &css_rules[i];
        }
    }
    return NULL;
}

static CssRule* add_rule(const char* selector, size_t length) {
    if (!selector || length == 0) return NULL;

    char* lowered = strndup_lower(selector, length);
    if (!lowered) return NULL;
    size_t lowered_len = strlen(lowered);

    CssRule* existing = find_rule(lowered, lowered_len);
    if (existing) {
        free(lowered);
        return existing;
    }

    ensure_capacity();
    if (css_rule_count >= css_rule_capacity) {
        free(lowered);
        return NULL;
    }

    CssRule* rule = &css_rules[css_rule_count++];
    rule->selector = lowered;
    rule->declarations = NULL;
    rule->declarations_len = 0;
    return rule;
}

static void append_declarations(CssRule* rule, const char* declarations, size_t length) {
    if (!rule || !declarations || length == 0) return;

    // Ensure declarations end with ';' for easier parsing later
    bool append_semicolon = declarations[length - 1] != ';';
    size_t extra = length + (append_semicolon ? 1 : 0);
    size_t separator = rule->declarations_len > 0 ? 1 : 0; // space between blocks
    size_t new_len = rule->declarations_len + separator + extra;
    char* buffer = (char*)realloc(rule->declarations, new_len + 1);
    if (!buffer) return;

    rule->declarations = buffer;
    char* write_ptr = rule->declarations + rule->declarations_len;
    if (separator) {
        *write_ptr++ = ' ';
    }
    memcpy(write_ptr, declarations, length);
    write_ptr += length;
    if (append_semicolon) {
        *write_ptr++ = ';';
    }
    *write_ptr = '\0';
    rule->declarations_len = new_len;
}

static void trim_bounds(const char** start, const char** end) {
    while (*start <= *end && isspace((unsigned char)**start)) (*start)++;
    while (*end >= *start && isspace((unsigned char)**end)) (*end)--;
}

static void parse_rule_block(const char* selectors, size_t selectors_len,
                             const char* body_start, const char* body_end) {
    const char* body_ptr = body_start;
    while (body_ptr <= body_end && isspace((unsigned char)*body_ptr)) body_ptr++;
    const char* trimmed_end = body_end;
    while (trimmed_end >= body_ptr && isspace((unsigned char)*trimmed_end)) trimmed_end--;
    if (body_ptr > trimmed_end) {
        return;
    }

    size_t body_len = (size_t)(trimmed_end - body_ptr + 1);

    const char* selector_begin = selectors;
    const char* selectors_limit = selectors + selectors_len;
    while (selector_begin < selectors_limit) {
        const char* selector_end = selector_begin;
        while (selector_end < selectors_limit && *selector_end != ',') selector_end++;

        const char* trimmed_begin = selector_begin;
        const char* trimmed_selector_end = selector_end - 1;
        trim_bounds(&trimmed_begin, &trimmed_selector_end);
        if (trimmed_begin <= trimmed_selector_end) {
            size_t sel_len = (size_t)(trimmed_selector_end - trimmed_begin + 1);
            CssRule* rule = add_rule(trimmed_begin, sel_len);
            if (rule) {
                append_declarations(rule, body_ptr, body_len);
            }
        }

        selector_begin = selector_end + 1;
    }
}

static void skip_comment(const char** cursor, const char* end) {
    const char* ptr = *cursor;
    while (ptr < end) {
        if (*ptr == '*' && (ptr + 1) < end && *(ptr + 1) == '/') {
            *cursor = ptr + 2;
            return;
        }
        ptr++;
    }
    *cursor = end;
}

bool css_parser_init(void) {
    css_rules = NULL;
    css_rule_count = 0;
    css_rule_capacity = 0;
    return true;
}

void css_parser_reset(void) {
    if (!css_rules) return;
    for (size_t i = 0; i < css_rule_count; ++i) {
        free(css_rules[i].selector);
        free(css_rules[i].declarations);
        css_rules[i].selector = NULL;
        css_rules[i].declarations = NULL;
        css_rules[i].declarations_len = 0;
    }
    css_rule_count = 0;
}

void css_parser_cleanup(void) {
    css_parser_reset();
    free(css_rules);
    css_rules = NULL;
    css_rule_capacity = 0;
}

void css_parser_add_stylesheet(const char* css, size_t length) {
    if (!css || length == 0) return;

    const char* cursor = css;
    const char* end = css + length;

    while (cursor < end) {
        if (isspace((unsigned char)*cursor)) {
            cursor++;
            continue;
        }
        if (*cursor == '/' && (cursor + 1) < end && *(cursor + 1) == '*') {
            cursor += 2;
            skip_comment(&cursor, end);
            continue;
        }

        const char* selector_start = cursor;
        while (cursor < end && *cursor != '{') {
            if (*cursor == '/' && (cursor + 1) < end && *(cursor + 1) == '*') {
                // Stop selector at comment
                break;
            }
            cursor++;
        }
        if (cursor >= end || *cursor != '{') {
            break;
        }

        const char* selector_end = cursor - 1;
        cursor++; // skip '{'

        int depth = 1;
        const char* body_start = cursor;
        while (cursor < end && depth > 0) {
            if (*cursor == '/' && (cursor + 1) < end && *(cursor + 1) == '*') {
                cursor += 2;
                skip_comment(&cursor, end);
                continue;
            }
            if (*cursor == '{') depth++;
            else if (*cursor == '}') depth--;
            cursor++;
        }

        const char* body_end = cursor - 2; // position before closing '}'
        if (body_end >= body_start && depth == 0) {
            parse_rule_block(selector_start, (size_t)(selector_end - selector_start + 1),
                             body_start, body_end);
        }
    }
}

const char* css_parser_get_declarations(const char* selector, size_t length) {
    if (!selector || length == 0) return NULL;
    char* lowered = strndup_lower(selector, length);
    if (!lowered) return NULL;
    CssRule* rule = find_rule(lowered, strlen(lowered));
    free(lowered);
    return rule ? rule->declarations : NULL;
}

// Parse inline style attribute and apply to widget
void css_parser_parse_inline_style(const char* style, RenderContext* context, void* widget) {
    if (!style || !context || !widget) return;

    char* style_copy = safe_strdup(style);
    if (!style_copy) return;

    char* p = style_copy;
    char* key_start, *val_start;

    while (*p) {
        // Skip whitespace
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        // Find key
        key_start = p;
        while (*p && *p != ':' && !isspace((unsigned char)*p)) p++;
        if (*p != ':') continue;
        *p++ = 0;

        for (char* walk = key_start; *walk; ++walk) {
            *walk = (char)tolower((unsigned char)*walk);
        }

        // Skip whitespace after colon
        while (*p && isspace((unsigned char)*p)) p++;
        val_start = p;

        // Find value end
        while (*p && *p != ';') p++;
        if (*p == ';') *p++ = 0;

        // Remove trailing whitespace from value
        size_t value_len = strlen(val_start);
        if (value_len == 0) continue;
        char* val_end = val_start + value_len - 1;
        while (val_end > val_start && isspace((unsigned char)*val_end)) *val_end-- = 0;
        if (*val_start == '\0') continue;

        // Apply styles
        if (strcmp(key_start, "color") == 0) {
            uint32_t parsed_color = 0;
            if (parse_color_value(val_start, &parsed_color) && context->renderer->interface->set_text_color) {
                context->renderer->interface->set_text_color(context->renderer, widget, parsed_color);
            }
        }
        else if (strcmp(key_start, "background-color") == 0 || strcmp(key_start, "background") == 0) {
            uint32_t parsed_color = 0;
            bool parsed = parse_color_value(val_start, &parsed_color);

            if (!parsed && strchr(val_start, ' ')) {
                char* background_copy = safe_strdup(val_start);
                if (background_copy) {
                    char* token_end = background_copy;
                    while (*token_end && !isspace((unsigned char)*token_end)) token_end++;
                    char saved = *token_end;
                    *token_end = '\0';
                    parsed = parse_color_value(background_copy, &parsed_color);
                    *token_end = saved;
                    free(background_copy);
                }
            }

            if (parsed && context->renderer->interface->set_bg_color) {
                context->renderer->interface->set_bg_color(context->renderer, widget, parsed_color);
            }
        }
        else if (strcmp(key_start, "text-align") == 0) {
            if (strcmp(val_start, "center") == 0) {
                if (context->renderer->interface->set_text_align) {
                    context->renderer->interface->set_text_align(context->renderer, widget, 1); // Center
                }
            } else if (strcmp(val_start, "right") == 0) {
                if (context->renderer->interface->set_text_align) {
                    context->renderer->interface->set_text_align(context->renderer, widget, 2); // Right
                }
            }
        }
        else if (strcmp(key_start, "padding") == 0) {
            // Padding handling could be added here
        }
        else if (strcmp(key_start, "margin") == 0) {
            // Margin handling could be added here
        }
    }

    free(style_copy);
}