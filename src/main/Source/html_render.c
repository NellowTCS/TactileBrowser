#include "html_render.h"
#include <lexbor/html/parser.h>
#include <lexbor/html/interface.h>

void render_html_to_lvgl(AppHandle app, lv_obj_t* parent, const char* html) {
    lxb_html_document_t *document = lxb_html_document_create();
    lxb_status_t status = lxb_html_document_parse(document, (const lxb_char_t *)html, strlen(html));

    if (status != LXB_STATUS_OK) {
        lv_label_set_text(parent, "HTML parse failed.");
        lxb_html_document_destroy(document);
        return;
    }

    const lxb_char_t *title = lxb_html_document_title(document, NULL);
    if (title) {
        lv_label_set_text(parent, (const char*)title);
    } else {
        lv_label_set_text(parent, "No title found.");
    }

    lxb_html_document_destroy(document);
}
