#include "url_utils.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

namespace {

std::string trim_copy(const std::string &value) {
    size_t start = 0;
    size_t end = value.size();
    while (start < end && std::isspace(static_cast<unsigned char>(value[start]))) {
        ++start;
    }
    while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }
    return value.substr(start, end - start);
}

bool has_scheme(const std::string &url) {
    auto pos = url.find("://");
    if (pos == std::string::npos || pos == 0) {
        return false;
    }
    for (size_t i = 0; i < pos; ++i) {
        unsigned char ch = static_cast<unsigned char>(url[i]);
        if (!std::isalpha(ch) && !std::isdigit(ch) && ch != '+' && ch != '-' && ch != '.') {
            return false;
        }
    }
    return true;
}

bool starts_with_case_insensitive(const std::string &value, const char *prefix) {
    size_t len = std::char_traits<char>::length(prefix);
    if (value.size() < len) {
        return false;
    }
    for (size_t i = 0; i < len; ++i) {
        if (std::tolower(static_cast<unsigned char>(value[i])) !=
            std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

void split_origin_and_path(const std::string &url, std::string &origin, std::string &path) {
    origin.clear();
    path.clear();

    auto scheme_pos = url.find("://");
    if (scheme_pos == std::string::npos) {
        path = url;
        return;
    }

    size_t host_start = scheme_pos + 3;
    size_t path_start = url.find('/', host_start);
    if (path_start == std::string::npos) {
        origin = url;
        path = "/";
    } else {
        origin = url.substr(0, path_start);
        path = url.substr(path_start);
    }
}

std::string normalize_path(const std::string &path) {
    std::vector<std::string> segments;
    size_t i = 0;
    bool trailing_slash = !path.empty() && path.back() == '/';

    while (i < path.size()) {
        while (i < path.size() && path[i] == '/') {
            ++i;
        }
        size_t start = i;
        while (i < path.size() && path[i] != '/') {
            ++i;
        }
        if (start == i) {
            continue;
        }
        std::string segment = path.substr(start, i - start);
        if (segment == ".") {
            continue;
        }
        if (segment == "..") {
            if (!segments.empty()) {
                segments.pop_back();
            }
            continue;
        }
        segments.push_back(segment);
    }

    std::string normalized = "/";
    for (size_t idx = 0; idx < segments.size(); ++idx) {
        normalized += segments[idx];
        if (idx + 1 < segments.size()) {
            normalized += "/";
        }
    }

    if (segments.empty()) {
        normalized = "/";
    }

    if (trailing_slash && normalized.back() != '/') {
        normalized += "/";
    }

    return normalized;
}

std::string remove_fragment_and_query(const std::string &input) {
    auto fragment_pos = input.find('#');
    std::string result = fragment_pos == std::string::npos ? input : input.substr(0, fragment_pos);
    auto query_pos = result.find('?');
    if (query_pos != std::string::npos) {
        result.erase(query_pos);
    }
    return result;
}

std::string resolve_internal(const std::string &base_url, const std::string &raw_href) {
    std::string href = trim_copy(raw_href);
    if (href.empty() || href[0] == '#') {
        return std::string();
    }

    if (has_scheme(href) || starts_with_case_insensitive(href, "data:") ||
        starts_with_case_insensitive(href, "mailto:") ||
        starts_with_case_insensitive(href, "javascript:")) {
        return href;
    }

    if (href.rfind("//", 0) == 0) {
        if (has_scheme(base_url)) {
            auto scheme_end = base_url.find(':');
            return base_url.substr(0, scheme_end) + ":" + href;
        }
        return std::string("https:") + href;
    }

    if (href[0] == '?') {
        std::string origin;
        std::string path;
        split_origin_and_path(base_url, origin, path);
        if (origin.empty()) {
            return std::string();
        }
        path = remove_fragment_and_query(path);
        if (path.empty()) {
            path = "/";
        }
        return origin + path + href;
    }

    std::string clean_base = remove_fragment_and_query(base_url);
    if (!has_scheme(clean_base)) {
        return href;
    }

    std::string origin;
    std::string base_path;
    split_origin_and_path(clean_base, origin, base_path);
    if (origin.empty()) {
        return href;
    }

    if (!href.empty() && href[0] == '/') {
        return origin + href;
    }

    if (base_path.empty()) {
        base_path = "/";
    }

    size_t last_slash = base_path.rfind('/');
    if (last_slash == std::string::npos) {
        base_path = "/";
    } else {
        base_path.erase(last_slash + 1);
    }

    std::string combined = origin + base_path + href;
    std::string path_only = combined.substr(origin.size());
    std::string normalized = normalize_path(path_only);
    return origin + normalized;
}

} // namespace

char* tactilebrowser_resolve_url(const char* base_url, const char* candidate_url) {
    if (!candidate_url) {
        return nullptr;
    }

    std::string base = base_url ? base_url : std::string();
    std::string candidate = candidate_url;
    std::string resolved = resolve_internal(base, candidate);
    if (resolved.empty()) {
        return nullptr;
    }

    char* out = static_cast<char*>(std::malloc(resolved.size() + 1));
    if (!out) {
        return nullptr;
    }
    std::copy(resolved.begin(), resolved.end(), out);
    out[resolved.size()] = '\0';
    return out;
}
