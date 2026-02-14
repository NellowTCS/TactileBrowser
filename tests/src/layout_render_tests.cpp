#include <cstdio>
#include <string>

#include "TestRenderer.h"

#define CHECK_TRUE(expr)                                                       \
  do {                                                                         \
    if (!(expr)) {                                                             \
      std::fprintf(stderr, "CHECK failed: %s (%s:%d)\n", #expr, __FILE__,      \
                   __LINE__);                                                  \
      return false;                                                            \
    }                                                                          \
  } while (0)

namespace {

bool render_markup(const std::string &html) {
  RenderResult result = tactilebrowser_render_html_string(
      "https://example.com", html.c_str(), html.size(), test_renderer_root(),
      800, 600);
  CHECK_TRUE(result == RENDER_SUCCESS);
  return true;
}

bool body_background_colors_root() {
  test_renderer_setup();
  const std::string html = R"HTML(
        <html>
            <body style="background-color:#0000ff">
                <p>Hello world</p>
            </body>
        </html>)HTML";

  CHECK_TRUE(render_markup(html));

  TestRendererState &state = test_renderer_state();
  CHECK_TRUE(state.root.bg_set);
  CHECK_TRUE(state.root.bg_color == 0x0000ff);
  CHECK_TRUE(!state.root.children.empty());
  CHECK_TRUE(test_find_child_by_text(&state.root, "Hello world") != nullptr);
  return true;
}

bool div_background_scoped_to_container() {
  test_renderer_setup();
  const std::string html = R"HTML(
        <html>
            <body>
                <div style="background:#ff0000">
                    <p>Inner text</p>
                </div>
            </body>
        </html>)HTML";

  CHECK_TRUE(render_markup(html));

  TestRendererState &state = test_renderer_state();
  TestWidget *div_widget = test_find_first_container(&state.root);
  CHECK_TRUE(div_widget != nullptr);
  CHECK_TRUE(div_widget->bg_set);
  CHECK_TRUE(div_widget->bg_color == 0xff0000);
  TestWidget *paragraph = test_find_child_by_text(div_widget, "Inner text");
  CHECK_TRUE(paragraph != nullptr);
  CHECK_TRUE(paragraph->parent == div_widget);
  return true;
}

bool stylesheet_background_applied() {
  test_renderer_setup();
  const std::string html = R"HTML(
        <html>
            <head>
                <style>
                    body { background: #123456; }
                    .note { background: #abcdef; }
                </style>
            </head>
            <body>
                <div class="note">
                    <p>Styled text</p>
                </div>
            </body>
        </html>)HTML";

  CHECK_TRUE(render_markup(html));

  TestRendererState &state = test_renderer_state();
  CHECK_TRUE(state.root.bg_set);
  CHECK_TRUE(state.root.bg_color == 0x123456);
  TestWidget *div_widget = test_find_first_container(&state.root);
  CHECK_TRUE(div_widget != nullptr);
  CHECK_TRUE(div_widget->bg_set);
  CHECK_TRUE(div_widget->bg_color == 0xabcdef);
  return true;
}

bool gradient_background_propagates() {
  test_renderer_setup();
  const std::string html = R"HTML(
        <html>
            <body>
                <div style="background-image:linear-gradient(45deg, #111111, #222222)">
                    <p>Gradient</p>
                </div>
            </body>
        </html>)HTML";

  CHECK_TRUE(render_markup(html));

  TestWidget *div_widget = test_find_first_container(test_renderer_root());
  CHECK_TRUE(div_widget != nullptr);
  CHECK_TRUE(div_widget->gradient_set);
  CHECK_TRUE(div_widget->gradient.stop_count == 2);
  CHECK_TRUE(div_widget->gradient.stops[0].color == 0x111111);
  CHECK_TRUE(div_widget->gradient.stops[1].color == 0x222222);
  return true;
}

struct TestCase {
  const char *name;
  bool (*fn)();
};

} // namespace

int main() {
  if (!tactilebrowser_core_init()) {
    std::fprintf(stderr, "Failed to initialize tactilebrowser core\n");
    return 1;
  }

  const TestCase cases[] = {
      {"body_background_colors_root", body_background_colors_root},
      {"div_background_scoped_to_container",
       div_background_scoped_to_container},
      {"stylesheet_background_applied", stylesheet_background_applied},
      {"gradient_background_propagates", gradient_background_propagates},
  };

  int failures = 0;
  for (const TestCase &test : cases) {
    if (!test.fn()) {
      std::fprintf(stderr, "Test failed: %s\n", test.name);
      failures++;
    }
  }

  tactilebrowser_core_cleanup();
  return failures == 0 ? 0 : 1;
}
