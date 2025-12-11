#include "TestRenderer.h"

#include <algorithm>

namespace {

TestWidget* widget_from(void* ptr) {
    return reinterpret_cast<TestWidget*>(ptr);
}

TestWidget* ensure_parent(Renderer* renderer) {
    TestWidget* parent = widget_from(renderer->platform_data);
    if (!parent) {
        parent = test_renderer_root();
    }
    return parent;
}

TestWidget* make_child(TestWidget* parent,
                       const char* text,
                       bool is_container,
                       bool is_input,
                       int x,
                       int y,
                       int width,
                       int height) {
    TestRendererState& state = test_renderer_state();

    auto owned = std::make_unique<TestWidget>();
    TestWidget* child = owned.get();
    child->parent = parent;
    child->is_container = is_container;
    child->is_input = is_input;
    child->x = x;
    child->y = y;
    child->width = width;
    child->height = height;
    if (text) {
        child->text = text;
    }
    parent->children.push_back(child);
    state.owned.push_back(std::move(owned));
    return child;
}

bool test_renderer_init(Renderer*) {
    return true;
}

void test_renderer_cleanup(Renderer*) {}

void* test_create_label(Renderer* renderer, const char* text, int x, int y) {
    TestWidget* parent = ensure_parent(renderer);
    TestWidget* child = make_child(parent, text, false, false, x, y, 0, 0);
    child->is_text = true;
    return child;
}

void* test_create_button(Renderer* renderer, const char* text, int x, int y) {
    return test_create_label(renderer, text, x, y);
}

void* test_create_text_input(Renderer* renderer,
                             const char* value,
                             const char* placeholder,
                             int x,
                             int y,
                             int width,
                             int height) {
    (void)placeholder;
    TestWidget* parent = ensure_parent(renderer);
    TestWidget* child = make_child(parent, value, false, true, x, y, width, height);
    return child;
}

void* test_create_text_area(Renderer* renderer,
                            const char* value,
                            int x,
                            int y,
                            int width,
                            int height) {
    return test_create_text_input(renderer, value, nullptr, x, y, width, height);
}

void* test_create_container(Renderer* renderer, int x, int y, int width, int height) {
    TestWidget* parent = ensure_parent(renderer);
    return make_child(parent, nullptr, true, false, x, y, width, height);
}

void test_register_link(Renderer*, void* widget, const char* url) {
    if (!widget || !url) return;
    test_renderer_state().link_targets.emplace_back(url);
    widget_from(widget)->text = url;
}

void test_set_text_color(Renderer*, void* widget, uint32_t) {
    if (!widget) return;
    widget_from(widget)->is_text = true;
}

void test_set_bg_color(Renderer*, void* widget, uint32_t color) {
    if (!widget) return;
    TestWidget* w = widget_from(widget);
    w->bg_set = true;
    w->bg_color = color;
    w->gradient_set = false;
}

void test_set_bg_gradient(Renderer*, void* widget, const LinearGradientFill* gradient) {
    if (!widget || !gradient) return;
    TestWidget* w = widget_from(widget);
    w->gradient_set = true;
    w->gradient = *gradient;
}

void test_set_text_align(Renderer*, void*, int) {}

void test_clear_container(Renderer*, void* container) {
    TestWidget* widget = widget_from(container);
    if (!widget) return;
    TestRendererState& state = test_renderer_state();
    if (widget == &state.root) {
        state.reset();
    }
}

int test_get_height(Renderer*, void* widget) {
    if (!widget) return 0;
    return widget_from(widget)->height;
}

RenderInterface& renderer_interface() {
    static RenderInterface iface = {
        test_renderer_init,
        test_renderer_cleanup,
        test_create_label,
        test_create_button,
        test_create_text_input,
        test_create_text_area,
        test_register_link,
        test_create_container,
        test_set_text_color,
        test_set_bg_color,
        test_set_bg_gradient,
        test_set_text_align,
        test_clear_container,
        test_get_height,
        nullptr
    };
    return iface;
}

} // namespace

void TestRendererState::reset() {
    owned.clear();
    link_targets.clear();
    root = TestWidget{};
    root.is_container = true;
    root.text = "root";
}

TestRendererState& test_renderer_state() {
    static TestRendererState state;
    return state;
}

RenderInterface* test_renderer_interface() {
    return &renderer_interface();
}

void test_renderer_setup() {
    test_renderer_state().reset();
    tactilebrowser_set_renderer(test_renderer_interface());
}

TestWidget* test_renderer_root() {
    return &test_renderer_state().root;
}

TestWidget* test_find_child_by_text(TestWidget* parent, const std::string& text) {
    if (!parent) return nullptr;
    auto it = std::find_if(parent->children.begin(), parent->children.end(),
                           [&text](TestWidget* child) {
                               return child && child->text == text;
                           });
    return it == parent->children.end() ? nullptr : *it;
}

TestWidget* test_find_first_container(TestWidget* parent) {
    if (!parent) return nullptr;
    auto it = std::find_if(parent->children.begin(), parent->children.end(),
                           [](TestWidget* child) {
                               return child && child->is_container;
                           });
    return it == parent->children.end() ? nullptr : *it;
}
