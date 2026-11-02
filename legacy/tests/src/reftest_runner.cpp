#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "TestRenderer.h"
#include "dump_utils.h"
#include "find_ref.h"
#include "fdm.h"

namespace fs = std::filesystem;

namespace {

struct Options {
  std::vector<std::string> dirs;
  std::vector<std::string> skip_files;
  std::string xfail_file;
  std::string write_xfail_file;
  bool update = false;
  bool verbose = false;
  bool allow_failures = false;
  int width = 800;
  int height = 600;
};

struct Stats {
  int found = 0;
  int pass = 0;
  int fail = 0;   // new, unexpected failures
  int known = 0;  // failures matching the xfail baseline
  int xpass = 0;  // baseline-expected failures that now pass
  int skip = 0;
};

bool matches_glob(const std::string &pattern, const std::string &path) {
  size_t p = 0, s = 0;
  size_t star = std::string::npos, mark = 0;
  while (s < path.size()) {
    if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == path[s])) {
      p++;
      s++;
    } else if (p < pattern.size() && pattern[p] == '*') {
      star = p++;
      mark = s;
    } else if (star != std::string::npos) {
      p = star + 1;
      s = ++mark;
    } else {
      return false;
    }
  }
  while (p < pattern.size() && pattern[p] == '*')
    p++;
  return p == pattern.size();
}

std::vector<std::string> load_skip_patterns(const std::string &file) {
  std::vector<std::string> patterns;
  std::ifstream in(file);
  if (!in) {
    std::fprintf(stderr, "warning: cannot read skip file %s\n", file.c_str());
    return patterns;
  }
  std::string line;
  while (std::getline(in, line)) {
    std::string trimmed = line;
    trimmed.erase(trimmed.begin(),
                  std::find_if(trimmed.begin(), trimmed.end(),
                               [](unsigned char c) { return !std::isspace(c); }));
    if (trimmed.empty() || trimmed[0] == '#')
      continue;
    patterns.push_back(trimmed);
  }
  return patterns;
}

bool is_skipped(const std::vector<std::string> &patterns,
                const std::string &rel_path) {
  for (const std::string &pattern : patterns) {
    if (matches_glob(pattern, rel_path))
      return true;
  }
  return false;
}

bool is_reference_file(const fs::path &path) {
  std::string stem = path.stem().string();
  static const char *suffixes[] = {"-ref",      "-notref", "-reference",
                                   "ref",       "reference",
                                   "-reference"};
  for (const char *s : suffixes) {
    std::string suffix(s);
    if (stem.size() > suffix.size() &&
        stem.compare(stem.size() - suffix.size(), suffix.size(), suffix) == 0) {
      return true;
    }
  }
  return false;
}

bool is_support_dir(const fs::path &dir) {
  std::string name = dir.filename().string();
  if (name.empty() || name[0] == '.')
    return true;
  return name == "support" || name == "resources" || name == "reference" ||
         name == "reference_support" || name == "ref" || name == "tools";
}

std::string read_file(const fs::path &path) {
  std::ifstream in(path, std::ios::binary);
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

bool write_file(const fs::path &path, const std::string &content) {
  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  if (!out)
    return false;
  out << content;
  return out.good();
}

std::string relative_to(const fs::path &path, const fs::path &base) {
  fs::path rel = fs::relative(path, base);
  std::string out = rel.generic_string();
  if (out.size() >= 3 && out.compare(0, 3, "../") == 0) {
    out = out.substr(3);
  }
  return out;
}

class TestApp {
public:
  explicit TestApp(const Options &options) : options_(options) {}

  int run() {
    if (!fdm_init()) {
      std::fprintf(stderr, "failed to initialize fdm core\n");
      return 2;
    }
    fdm_set_link_handler(nullptr, nullptr);

    for (const std::string &skip_file : options_.skip_files) {
      std::vector<std::string> patterns = load_skip_patterns(skip_file);
      skip_patterns_.insert(skip_patterns_.end(), patterns.begin(),
                            patterns.end());
    }
    if (!options_.xfail_file.empty()) {
      xfail_patterns_ = load_skip_patterns(options_.xfail_file);
    }

    std::fprintf(stdout, "=== fdm_reftest ===\n");
    for (const std::string &dir : options_.dirs) {
      if (!run_dir(fs::path(dir))) {
        fdm_cleanup();
        return 2;
      }
    }

    if (!options_.write_xfail_file.empty()) {
      write_xfail_baseline();
    }

    print_summary();

    fdm_cleanup();
    return (stats_.fail > 0 && !options_.allow_failures) ? 1 : 0;
  }

private:
  struct DocumentDumps {
    bool ok = false;
    std::string ops;
    std::string layout;
  };

  bool run_dir(const fs::path &dir) {
    if (!fs::is_directory(dir)) {
      std::fprintf(stderr, "error: not a directory: %s\n", dir.c_str());
      return false;
    }

    int dir_found = 0, dir_pass = 0, dir_fail = 0, dir_skip = 0;
    std::error_code ec;
    for (fs::recursive_directory_iterator it(
             dir, fs::directory_options::skip_permission_denied, ec),
         end;
         it != end; it.increment(ec)) {
      if (ec) {
        ec.clear();
        continue;
      }
      const fs::path &path = it->path();
      if (it->is_directory(ec)) {
        if (is_support_dir(path)) {
          it.disable_recursion_pending();
        }
        continue;
      }
      if (path.extension() != ".html" && path.extension() != ".htm")
        continue;
      if (is_reference_file(path))
        continue;

      std::string rel_path = relative_to(path, dir);
      if (is_skipped(skip_patterns_, rel_path)) {
        dir_skip++;
        if (options_.verbose)
          std::fprintf(stdout, "SKIP  %s\n", rel_path.c_str());
        continue;
      }

      dir_found++;
      stats_.found++;
      switch (run_test(path, rel_path)) {
      case 0:
        dir_pass++;
        break;
      case 1:
        dir_fail++;
        break;
      default:
        dir_skip++;
        break;
      }
    }

    char buf[256];
    std::snprintf(buf, sizeof(buf),
                  "dir: %-40s found=%d pass=%d fail=%d skip=%d\n",
                  dir.c_str(), dir_found, dir_pass, dir_fail, dir_skip);
    std::fprintf(stdout, "%s", buf);
    return true;
  }

  DocumentDumps render(const fs::path &file) {
    DocumentDumps dumps;
    std::string html = read_file(file);
    if (html.empty())
      return dumps;

    TestRendererState &state = test_renderer_state();
    state.reset();
    fdm_set_link_handler(nullptr, nullptr);
    FdmResult result =
        fdm_render_html(&state.surface, "file://reftest", html.c_str(),
                        html.size(), options_.width, options_.height);
    if (result != FDM_OK)
      return dumps;

    dumps.ok = true;
    dumps.ops = dump_ops_text(state);
    dumps.layout = dump_layout_text(state);
    return dumps;
  }

  bool in_baseline(const std::string &rel_path) const {
    return is_skipped(xfail_patterns_, rel_path);
  }

  // 0 = pass, 1 = fail, 2 = skip
  int run_test(const fs::path &path, const std::string &rel_path) {
    DocumentDumps test = render(path);
    if (!test.ok) {
      return record_failure(rel_path, "render error");
    }

    std::string html = read_file(path);
    RefInfo ref = find_reference(html);

    if (ref.kind != RefKind::None) {
      fs::path ref_path = path.parent_path() / fs::path(ref.href);
      if (!fs::exists(ref_path)) {
        return record_failure(rel_path, "missing reference " + ref.href);
      }
      DocumentDumps ref_dumps = render(ref_path);
      if (!ref_dumps.ok) {
        return record_failure(rel_path, "reference render error");
      }

      bool identical = test.ops == ref_dumps.ops &&
                       test.layout == ref_dumps.layout;
      bool pass =
          (ref.kind == RefKind::Match) ? identical : !identical;
      if (pass) {
        return record_pass(rel_path,
                           ref.kind == RefKind::Match ? "match" : "mismatch");
      }
      std::string detail =
          ref.kind == RefKind::Match ? "match" : "mismatch";
      if (options_.verbose) {
        if (test.ops != ref_dumps.ops)
          detail += " [ops differ]";
        if (test.layout != ref_dumps.layout)
          detail += " [layout differs]";
        detail += " ref=" + ref.href;
      }
      return record_failure(rel_path, detail);
    }

    // No <link> reference: use golden snapshots, else skip.
    fs::path ops_golden = path.string() + ".ops";
    fs::path lay_golden = path.string() + ".lay";
    if (options_.update) {
      write_file(ops_golden, test.ops);
      write_file(lay_golden, test.layout);
      return record_pass(rel_path, "golden written");
    }
    if (fs::exists(ops_golden) && fs::exists(lay_golden)) {
      bool ok = read_file(ops_golden) == test.ops &&
                read_file(lay_golden) == test.layout;
      if (ok) {
        return record_pass(rel_path, "golden");
      }
      return record_failure(rel_path, "golden mismatch");
    }

    stats_.skip++;
    if (options_.verbose)
      std::fprintf(stdout, "SKIP  %s (no reference)\n", rel_path.c_str());
    return 2;
  }

  int record_pass(const std::string &rel_path, const char *detail) {
    if (in_baseline(rel_path)) {
      stats_.xpass++;
      std::fprintf(stdout, "XPASS %s (%s)\n", rel_path.c_str(), detail);
      return 0;
    }
    stats_.pass++;
    if (options_.verbose)
      std::fprintf(stdout, "PASS  %s (%s)\n", rel_path.c_str(), detail);
    return 0;
  }

  int record_failure(const std::string &rel_path, const std::string &detail) {
    failing_paths_.push_back(rel_path);
    if (in_baseline(rel_path)) {
      stats_.known++;
      if (options_.verbose)
        std::fprintf(stdout, "KNOWN %s (%s)\n", rel_path.c_str(),
                     detail.c_str());
      return 1;
    }
    stats_.fail++;
    std::fprintf(stdout, "FAIL  %s (%s)\n", rel_path.c_str(), detail.c_str());
    return 1;
  }

  void write_xfail_baseline() {
    std::ofstream out(options_.write_xfail_file,
                      std::ios::binary | std::ios::trunc);
    if (!out) {
      std::fprintf(stderr, "error: cannot write xfail file %s\n",
                   options_.write_xfail_file.c_str());
      return;
    }
    std::sort(failing_paths_.begin(), failing_paths_.end());
    failing_paths_.erase(
        std::unique(failing_paths_.begin(), failing_paths_.end()),
        failing_paths_.end());
    for (const std::string &path : failing_paths_) {
      out << path << "\n";
    }
  }

  void print_summary() {
    double pct = stats_.found > 0
                     ? (100.0 * (stats_.pass + stats_.xpass)) /
                           (double)(stats_.pass + stats_.fail + stats_.known +
                                    stats_.xpass)
                     : 0.0;
    std::fprintf(
        stdout,
        "----------------------------------------\n"
        "TOTAL  found=%d pass=%d xpass=%d known=%d fail=%d skip=%d "
        "(pass rate %.2f%%)\n",
        stats_.found, stats_.pass, stats_.xpass, stats_.known, stats_.fail,
        stats_.skip, pct);
    if (!options_.xfail_file.empty()) {
      std::fprintf(stdout, "xfail baseline: %s\n",
                   options_.xfail_file.c_str());
    }
  }

  const Options &options_;
  Stats stats_;
  std::vector<std::string> skip_patterns_;
  std::vector<std::string> xfail_patterns_;
  std::vector<std::string> failing_paths_;
};

void usage(const char *prog) {
  std::fprintf(
      stderr,
      "usage: %s [--dir <path>]... [--skip <file>]... [--update]\n"
      "       [--xfail <file>] [--write-xfail <file>]\n"
      "       [--width <px>] [--height <px>] [--allow-failures] [--verbose]\n",
      prog);
}

} // namespace

int main(int argc, char **argv) {
  Options options;

  auto need_value = [&](int &i) -> const char * {
    if (i + 1 >= argc) {
      usage(argv[0]);
      exit(2);
    }
    return argv[++i];
  };

  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--dir") {
      options.dirs.push_back(need_value(i));
    } else if (arg == "--skip") {
      options.skip_files.push_back(need_value(i));
    } else if (arg == "--xfail") {
      options.xfail_file = need_value(i);
    } else if (arg == "--write-xfail") {
      options.write_xfail_file = need_value(i);
    } else if (arg == "--update") {
      options.update = true;
    } else if (arg == "--verbose") {
      options.verbose = true;
    } else if (arg == "--allow-failures") {
      options.allow_failures = true;
    } else if (arg == "--width") {
      options.width = std::atoi(need_value(i));
    } else if (arg == "--height") {
      options.height = std::atoi(need_value(i));
    } else if (arg == "--help" || arg == "-h") {
      usage(argv[0]);
      return 0;
    } else {
      std::fprintf(stderr, "unknown argument: %s\n", arg.c_str());
      usage(argv[0]);
      return 2;
    }
  }

  if (options.dirs.empty()) {
    usage(argv[0]);
    return 2;
  }

  TestApp app(options);
  return app.run();
}