#include "css_parser.h"
#include "tactilebrowser_core.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

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
