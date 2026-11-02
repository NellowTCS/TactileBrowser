#include "TestRenderer.h"

#include <algorithm>
#include <cstring>

namespace {

void op_begin_frame(FdmSurface *, int width, int height) {
  TestRendererState &state = test_renderer_state();
  Op op;
  op.kind = OpKind::BeginFrame;
  op.w = width;
  op.h = height;
  state.ops.push_back(op);
}

void op_end_frame(FdmSurface *) {
  Op op;
  op.kind = OpKind::EndFrame;
  test_renderer_state().ops.push_back(op);
}

void op_fill_rect(FdmSurface *, int x, int y, int w, int h, uint32_t color) {
  Op op;
  op.kind = OpKind::FillRect;
  op.x = x;
  op.y = y;
  op.w = w;
  op.h = h;
  op.color = color;
  test_renderer_state().ops.push_back(op);
}

void op_fill_rect_gradient(FdmSurface *, int x, int y, int w, int h,
                           const FdmLinearGradient *gradient) {
  Op op;
  op.kind = OpKind::FillGradient;
  op.x = x;
  op.y = y;
  op.w = w;
  op.h = h;
  if (gradient) {
    op.angle_deg = gradient->angle_deg;
    op.stop_count = gradient->stop_count;
    for (size_t i = 0; i < gradient->stop_count; ++i) {
      op.stop_colors[i] = gradient->stops[i].color;
      op.stop_positions[i] = gradient->stops[i].position;
    }
  }
  test_renderer_state().ops.push_back(op);
}

void op_draw_text(FdmSurface *, const char *text, int x, int y, int max_width,
                  int font_size, uint32_t color, int align, bool underline) {
  Op op;
  op.kind = OpKind::DrawText;
  op.x = x;
  op.y = y;
  op.w = max_width;
  op.color = color;
  op.font_size = font_size;
  op.align = align;
  op.underline = underline;
  if (text) {
    op.text = text;
  }
  test_renderer_state().ops.push_back(op);
}

int op_measure_text(FdmSurface *, const char *text, int font_size) {
  if (!text) {
    return 0;
  }
  return (int)(std::strlen(text) * font_size * 0.6);
}

const FdmSurfaceOps &surface_ops() {
  static FdmSurfaceOps ops = {
      op_begin_frame,
      op_end_frame,
      op_fill_rect,
      op_fill_rect_gradient,
      op_draw_text,
      op_measure_text,
  };
  return ops;
}

void test_link_handler(void *user_data, const char *url) {
  TestRendererState *state = static_cast<TestRendererState *>(user_data);
  if (state && url) {
    state->link_targets.emplace_back(url);
  }
}

} // namespace

void TestRendererState::reset() {
  ops.clear();
  link_targets.clear();
  surface.ops = &surface_ops();
  surface.platform_data = nullptr;
  surface.user_data = nullptr;
}

void TestRendererState::clear_ops() { ops.clear(); }

const Op *TestRendererState::find_fill(uint32_t color) const {
  auto it = std::find_if(ops.begin(), ops.end(), [color](const Op &op) {
    return op.kind == OpKind::FillRect && op.color == color;
  });
  return it == ops.end() ? nullptr : &(*it);
}

const Op *TestRendererState::find_gradient() const {
  auto it = std::find_if(ops.begin(), ops.end(),
                         [](const Op &op) { return op.kind == OpKind::FillGradient; });
  return it == ops.end() ? nullptr : &(*it);
}

const Op *TestRendererState::find_text(const std::string &text) const {
  auto it = std::find_if(ops.begin(), ops.end(), [&text](const Op &op) {
    return op.kind == OpKind::DrawText && op.text == text;
  });
  return it == ops.end() ? nullptr : &(*it);
}

bool TestRendererState::has_text(const std::string &text) const {
  return find_text(text) != nullptr;
}

TestRendererState &test_renderer_state() {
  static TestRendererState state;
  return state;
}

void test_renderer_setup() {
  TestRendererState &state = test_renderer_state();
  state.reset();
  fdm_set_link_handler(test_link_handler, &state);
}
