#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "tactilebrowser_core.h"

struct TestWidget {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
  bool is_container = false;
  bool is_text = false;
  bool is_input = false;
  bool bg_set = false;
  uint32_t bg_color = 0;
  bool gradient_set = false;
  LinearGradientFill gradient{};
  std::string text;
  TestWidget *parent = nullptr;
  std::vector<TestWidget *> children;
};

struct TestRendererState {
  TestWidget root;
  std::vector<std::unique_ptr<TestWidget>> owned;
  std::vector<std::string> link_targets;

  void reset();
};

TestRendererState &test_renderer_state();
RenderInterface *test_renderer_interface();
void test_renderer_setup();
TestWidget *test_renderer_root();
TestWidget *test_find_child_by_text(TestWidget *parent,
                                    const std::string &text);
TestWidget *test_find_first_container(TestWidget *parent);
