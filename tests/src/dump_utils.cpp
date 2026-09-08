#include "dump_utils.h"

#include <cstdio>
#include <sstream>

namespace {

std::string escape_text(const std::string &text) {
  std::string out;
  out.reserve(text.size());
  for (char c : text) {
    switch (c) {
    case '"':
      out += "\\\"";
      break;
    case '\\':
      out += "\\\\";
      break;
    case '\n':
      out += "\\n";
      break;
    case '\t':
      out += "\\t";
      break;
    case '\r':
      break;
    default:
      out += c;
      break;
    }
  }
  return out;
}

std::string hex_color(uint32_t color) {
  char buf[8];
  std::snprintf(buf, sizeof(buf), "%06X", color);
  return std::string(buf);
}

std::string format_float(float v) {
  char buf[24];
  std::snprintf(buf, sizeof(buf), "%g", (double)v);
  return std::string(buf);
}

struct LineSink {
  std::string text;
  void put(const char *line) {
    text += line;
    text += '\n';
  }
};

void emit_layout_cb(void *user_data, const char *line) {
  static_cast<LineSink *>(user_data)->put(line);
}

} // namespace

std::string dump_ops_text(const TestRendererState &state) {
  std::ostringstream out;
  if (state.ops.empty()) {
    out << "(no ops)\n";
  }
  for (const Op &op : state.ops) {
    switch (op.kind) {
    case OpKind::BeginFrame:
      out << "begin " << op.w << "x" << op.h << "\n";
      break;
    case OpKind::EndFrame:
      out << "end\n";
      break;
    case OpKind::FillRect:
      out << "fill " << op.x << "," << op.y << " " << op.w << "x" << op.h
          << " #" << hex_color(op.color) << "\n";
      break;
    case OpKind::FillGradient:
      out << "gradient " << op.x << "," << op.y << " " << op.w << "x" << op.h;
      for (size_t i = 0; i < op.stop_count; ++i) {
        out << (i == 0 ? " angle=" : ",") << hex_color(op.stop_colors[i])
            << "@" << format_float(op.stop_positions[i]);
      }
      out << "\n";
      break;
    case OpKind::DrawText:
      out << "text " << op.x << "," << op.y << " w=" << op.w
          << " fs=" << op.font_size << " #" << hex_color(op.color)
          << " align=" << op.align << " u=" << (op.underline ? 1 : 0) << " \""
          << escape_text(op.text) << "\"\n";
      break;
    }
  }
  return out.str();
}

std::string dump_layout_text(const TestRendererState &state) {
  LineSink sink;
  fdm_dump_layout(const_cast<FdmSurface *>(&state.surface), emit_layout_cb,
                  &sink);
  if (sink.text.empty()) {
    sink.put("(no layout tree)");
  }
  return sink.text;
}