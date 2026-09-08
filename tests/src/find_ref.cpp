#include "find_ref.h"

#include <cstring>

#include <lexbor/dom/interfaces/document.h>
#include <lexbor/dom/interfaces/element.h>
#include <lexbor/dom/interfaces/node.h>
#include <lexbor/html/interfaces/document.h>
#include <lexbor/tag/tag.h>

namespace {

bool rel_has(const char *rel, size_t rel_len, const char *token) {
  if (!rel)
    return false;
  size_t token_len = std::strlen(token);
  // Token match on whitespace-separated rel values, case-insensitively.
  const unsigned char *p = (const unsigned char *)rel;
  const unsigned char *end = p + rel_len;
  while (p < end) {
    while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
      p++;
    const unsigned char *start = p;
    while (p < end && !(*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r'))
      p++;
    if ((size_t)(p - start) == token_len) {
      bool eq = true;
      for (size_t i = 0; i < token_len; ++i) {
        unsigned char a = start[i];
        if (a >= 'A' && a <= 'Z')
          a = (unsigned char)(a + 32);
        if (a != (unsigned char)token[i]) {
          eq = false;
          break;
        }
      }
      if (eq)
        return true;
    }
  }
  return false;
}

} // namespace

RefInfo find_reference(const std::string &html) {
  RefInfo info;

  lxb_html_document_t *document = lxb_html_document_create();
  if (!document)
    return info;

  lxb_status_t status =
      lxb_html_document_parse(document, (const lxb_char_t *)html.data(),
                              html.size());
  if (status != LXB_STATUS_OK) {
    lxb_html_document_destroy(document);
    return info;
  }

  lxb_dom_document_t *dom_doc = lxb_dom_interface_document(document);
  lxb_dom_element_t *root = lxb_dom_document_element(dom_doc);
  if (root) {
    lxb_dom_collection_t *collection =
        lxb_dom_collection_make(dom_doc, 4);
    if (collection) {
      if (lxb_dom_elements_by_tag_name(root, collection,
                                       (const lxb_char_t *)"link", 4) ==
          LXB_STATUS_OK) {
        size_t n = lxb_dom_collection_length(collection);
        for (size_t i = 0; i < n; ++i) {
          lxb_dom_element_t *element = lxb_dom_collection_element(collection, i);
          if (!element)
            continue;

          size_t rel_len = 0;
          const lxb_char_t *rel =
              lxb_dom_element_get_attribute(element, (const lxb_char_t *)"rel",
                                            3, &rel_len);
          size_t href_len = 0;
          const lxb_char_t *href =
              lxb_dom_element_get_attribute(element, (const lxb_char_t *)"href",
                                            4, &href_len);
          if (!href || href_len == 0)
            continue;

          if (rel_has((const char *)rel, rel_len, "match")) {
            info.kind = RefKind::Match;
            info.href.assign((const char *)href, href_len);
            break;
          }
          if (rel_has((const char *)rel, rel_len, "mismatch")) {
            info.kind = RefKind::Mismatch;
            info.href.assign((const char *)href, href_len);
            break;
          }
        }
      }
      lxb_dom_collection_destroy(collection, true);
    }
  }

  lxb_html_document_destroy(document);
  return info;
}