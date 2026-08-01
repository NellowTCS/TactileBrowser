#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "fdm.h"

// A test surface that records every FdmSurfaceOps call into a command list.
enum class OpKind {
  BeginFrame,
  EndFrame,
  FillRect,
  FillGradient,
  DrawText,
};

struct Op {
  OpKind kind;
  int x = 0;
  int y = 0;
  int w = 0;
  int h = 0;
  uint32_t color = 0;
  std::string text;
  int font_size = 0;
  int align = 0;
  bool underline = false;
  std::array<uint32_t, FDM_MAX_GRADIENT_STOPS> stop_colors{};
  std::array<float, FDM_MAX_GRADIENT_STOPS> stop_positions{};
  size_t stop_count = 0;
  float angle_deg = 0;
};

struct TestRendererState {
  FdmSurface surface;
  std::vector<Op> ops;
  std::vector<std::string> link_targets;

  void reset();
  void clear_ops();
  const Op *find_fill(uint32_t color) const;
  const Op *find_gradient() const;
  const Op *find_text(const std::string &text) const;
  bool has_text(const std::string &text) const;
};

TestRendererState &test_renderer_state();
void test_renderer_setup();
