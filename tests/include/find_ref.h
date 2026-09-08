#pragma once

#include <string>

// References: a document may carry
//   <link rel="match" href="foo-ref.html">  or  <link rel="mismatch" href="...">
enum class RefKind { None, Match, Mismatch };

struct RefInfo {
  RefKind kind = RefKind::None;
  std::string href;
};

// Parse the HTML and return the first match/mismatch reference found.
RefInfo find_reference(const std::string &html);