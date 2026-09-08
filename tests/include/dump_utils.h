#pragma once

#include <string>

#include "TestRenderer.h"

// Serialize the recorded surface ops into a canonical, line-per-op text form
// that is stable across runs for the same document + viewport. Suitable for
// reftest/snapshot comparison.
std::string dump_ops_text(const TestRendererState &state);

// Serialize the layout tree of the current document (via fdm_dump_layout) into
// canonical text.
std::string dump_layout_text(const TestRendererState &state);