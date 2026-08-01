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

bool render_markup(const std::string &html, int width = 800, int height = 600) {
  TestRendererState &state = test_renderer_state();
  FdmResult result = fdm_render_html(&state.surface, "https://example.com",
                                     html.c_str(), html.size(), width, height);
  CHECK_TRUE(result == FDM_OK);
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
  const Op *fill = state.find_fill(0x0000ff);
  CHECK_TRUE(fill != nullptr);
  CHECK_TRUE(fill->w > 0);
  CHECK_TRUE(state.has_text("Hello world"));
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
  const Op *fill = state.find_fill(0xff0000);
  CHECK_TRUE(fill != nullptr);
  const Op *paragraph = state.find_text("Inner text");
  CHECK_TRUE(paragraph != nullptr);
  // The inner paragraph must be painted inside the div's vertical range.
  CHECK_TRUE(paragraph->y >= fill->y);
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
  CHECK_TRUE(state.find_fill(0x123456) != nullptr);
  CHECK_TRUE(state.find_fill(0xabcdef) != nullptr);
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

  TestRendererState &state = test_renderer_state();
  const Op *gradient = state.find_gradient();
  CHECK_TRUE(gradient != nullptr);
  CHECK_TRUE(gradient->stop_count == 2);
  CHECK_TRUE(gradient->stop_colors[0] == 0x111111);
  CHECK_TRUE(gradient->stop_colors[1] == 0x222222);
  return true;
}

bool link_click_fires_handler() {
  test_renderer_setup();
  const std::string html = R"HTML(
        <html>
            <body>
                <a href="/page2">Next</a>
            </body>
        </html>)HTML";

  CHECK_TRUE(render_markup(html));

  TestRendererState &state = test_renderer_state();
  const Op *text = state.find_text("Next");
  CHECK_TRUE(text != nullptr);

  int cx = text->x + 5;
  int cy = text->y + 5;
  CHECK_TRUE(fdm_handle_pointer(&state.surface, cx, cy, FDM_POINTER_DOWN));
  CHECK_TRUE(fdm_handle_pointer(&state.surface, cx, cy, FDM_POINTER_UP));

  CHECK_TRUE(state.link_targets.size() == 1);
  CHECK_TRUE(state.link_targets[0] == "https://example.com/page2");
  return true;
}

bool input_focus_and_typing() {
  test_renderer_setup();
  const std::string html = R"HTML(
        <html>
            <body>
                <input type="text" value="abc" placeholder="Enter name">
            </body>
        </html>)HTML";

  CHECK_TRUE(render_markup(html));

  TestRendererState &state = test_renderer_state();
  const Op *text = state.find_text("abc");
  CHECK_TRUE(text != nullptr);

  int cx = text->x + 5;
  int cy = text->y + 5;
  CHECK_TRUE(fdm_handle_pointer(&state.surface, cx, cy, FDM_POINTER_DOWN));

  fdm_input_text(&state.surface, "X");
  CHECK_TRUE(state.has_text("abcX"));

  state.clear_ops();
  fdm_input_backspace(&state.surface);
  CHECK_TRUE(state.has_text("abc"));
  CHECK_TRUE(!state.has_text("abcX"));
  return true;
}

bool scroll_repaints_content() {
  test_renderer_setup();

  std::string html = "<html><body>";
  for (int i = 0; i < 40; ++i) {
    html += "<p>Paragraph " + std::to_string(i) + "</p>";
  }
  html += "</body></html>";

  CHECK_TRUE(render_markup(html, 800, 300));

  TestRendererState &state = test_renderer_state();
  int total = fdm_content_height(&state.surface);
  CHECK_TRUE(total > 300);

  state.clear_ops();
  fdm_scroll_by(&state.surface, 100000);

  // Scrolling clamps to the max scroll offset, so the top is pushed off-screen.
  const Op *first = state.find_text("Paragraph 0");
  CHECK_TRUE(first != nullptr);
  CHECK_TRUE(first->y <= 0);

  // The last paragraph should now be visible inside the viewport.
  const Op *last = state.find_text("Paragraph 39");
  CHECK_TRUE(last != nullptr);
  CHECK_TRUE(last->y >= 0 && last->y < 300);
  return true;
}

struct TestCase {
  const char *name;
  bool (*fn)();
};

} // namespace

int main() {
  if (!fdm_init()) {
    std::fprintf(stderr, "Failed to initialize fdm core\n");
    return 1;
  }

  const TestCase cases[] = {
      {"body_background_colors_root", body_background_colors_root},
      {"div_background_scoped_to_container",
       div_background_scoped_to_container},
      {"stylesheet_background_applied", stylesheet_background_applied},
      {"gradient_background_propagates", gradient_background_propagates},
      {"link_click_fires_handler", link_click_fires_handler},
      {"input_focus_and_typing", input_focus_and_typing},
      {"scroll_repaints_content", scroll_repaints_content},
  };

  int failures = 0;
  for (const TestCase &test : cases) {
    if (!test.fn()) {
      std::fprintf(stderr, "Test failed: %s\n", test.name);
      failures++;
    }
  }

  fdm_cleanup();
  return failures == 0 ? 0 : 1;
}
