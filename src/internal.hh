#pragma once

#include "aview.hh"

#include <algorithm>
#include <cerrno>
#include <cctype>
#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <limits>
#include <optional>
#include <set>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

namespace archspec {
namespace detail {

struct ReadResult {
  Status status = Status::not_found;
  std::string value;
};

inline bool starts_with(const std::string& value, const std::string& prefix) {
  return value.rfind(prefix, 0) == 0;
}

inline std::string trim(std::string value) {
  auto is_space = [](unsigned char c) { return std::isspace(c) != 0; };

  auto begin = std::find_if_not(value.begin(), value.end(), is_space);
  auto end = std::find_if_not(value.rbegin(), value.rend(), is_space).base();

  if (begin >= end) {
    return {};
  }

  return std::string(begin, end);
}

inline std::string basename(const std::string& path) {
  if (path.empty()) {
    return {};
  }

  std::size_t end = path.find_last_not_of("/\\");
  if (end == std::string::npos) {
    return "/";
  }

  std::size_t slash = path.find_last_of("/\\", end);
  if (slash == std::string::npos) {
    return path.substr(0, end + 1);
  }

  return path.substr(slash + 1, end - slash);
}

inline std::string join_path(const std::string& root, const std::string& suffix) {
  if (suffix.empty()) {
    return root;
  }

  if (root.empty() || root == "/") {
    return "/" + (suffix[0] == '/' ? suffix.substr(1) : suffix);
  }

  if (suffix[0] == '/') {
    return root + suffix;
  }

  return root + "/" + suffix;
}

inline std::string proc_path(const CollectOptions& options, const std::string& suffix) {
  return join_path(options.procfs_root, suffix);
}

inline std::string sys_path(const CollectOptions& options, const std::string& suffix) {
  return join_path(options.sysfs_root, suffix);
}

inline std::string etc_path(const CollectOptions& options, const std::string& suffix) {
  return join_path(options.etc_root, suffix);
}

inline std::string dev_path(const CollectOptions& options, const std::string& suffix) {
  return join_path(options.dev_root, suffix);
}

inline Status status_for_path_error(const std::string& path) {
  std::error_code error;
  if (std::filesystem::exists(path, error)) {
    return Status::perm_denied;
  }
  if (!error || error == std::errc::no_such_file_or_directory ||
      error == std::errc::not_a_directory) {
    return Status::not_found;
  }
  if (error == std::errc::permission_denied || error == std::errc::operation_not_permitted) {
    return Status::perm_denied;
  }
  return Status::internal_error;
}

inline bool path_exists(const std::string& path) {
  std::error_code error;
  return std::filesystem::exists(path, error);
}

inline bool path_is_dir(const std::string& path) {
  std::error_code error;
  return std::filesystem::is_directory(path, error);
}

inline ReadResult read_file(const std::string& path, std::ios::openmode mode = std::ios::in) {
  std::ifstream file(path, mode);
  if (!file.is_open()) {
    return {status_for_path_error(path), {}};
  }

  std::ostringstream out;
  out << file.rdbuf();

  if (file.bad()) {
    return {Status::internal_error, {}};
  }

  return {Status::ok, out.str()};
}

inline ReadResult read_first_line(const std::string& path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    return {status_for_path_error(path), {}};
  }

  std::string line;
  if (!std::getline(file, line) && file.bad()) {
    return {Status::internal_error, {}};
  }

  return {Status::ok, line};
}

inline StringField read_string_field(const std::string& path, bool trim_value = true) {
  ReadResult result = read_first_line(path);
  if (result.status != Status::ok) {
    return StringField::unavailable(result.status);
  }

  return StringField::value(trim_value ? trim(result.value) : result.value);
}

inline bool parse_u64(const std::string& text, std::uint64_t& value) {
  std::string trimmed = trim(text);
  // strtoull accepts a leading minus sign and returns its modulo-2^N
  // representation.  That is useful for C compatibility, but never a valid
  // representation for the non-negative values exposed by procfs and sysfs.
  if (trimmed.empty() || trimmed.front() == '-') {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  unsigned long long parsed = std::strtoull(trimmed.c_str(), &end, 0);
  if (errno != 0 || end == trimmed.c_str()) {
    return false;
  }

  while (*end != '\0') {
    if (!std::isspace(static_cast<unsigned char>(*end))) {
      return false;
    }
    ++end;
  }

  value = static_cast<std::uint64_t>(parsed);
  return true;
}

inline bool parse_i64(const std::string& text, std::int64_t& value) {
  std::string trimmed = trim(text);
  if (trimmed.empty()) {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  long long parsed = std::strtoll(trimmed.c_str(), &end, 0);
  if (errno != 0 || end == trimmed.c_str()) {
    return false;
  }

  while (*end != '\0') {
    if (!std::isspace(static_cast<unsigned char>(*end))) {
      return false;
    }
    ++end;
  }

  value = static_cast<std::int64_t>(parsed);
  return true;
}

inline U64Field read_u64_field(const std::string& path, std::uint64_t multiplier = 1) {
  ReadResult result = read_first_line(path);
  if (result.status != Status::ok) {
    return U64Field::unavailable(result.status);
  }

  std::uint64_t value = 0;
  if (!parse_u64(result.value, value)) {
    return U64Field::unavailable(Status::parse_error);
  }

  if (multiplier != 0 && value > std::numeric_limits<std::uint64_t>::max() / multiplier) {
    return U64Field::unavailable(Status::parse_error);
  }
  return U64Field::value(value * multiplier);
}

inline U32Field read_u32_field(const std::string& path) {
  ReadResult result = read_first_line(path);
  if (result.status != Status::ok) {
    return U32Field::unavailable(result.status);
  }

  std::uint64_t value = 0;
  if (!parse_u64(result.value, value) || value > UINT32_MAX) {
    return U32Field::unavailable(Status::parse_error);
  }

  return U32Field::value(static_cast<std::uint32_t>(value));
}

inline I64Field read_i64_field(const std::string& path) {
  ReadResult result = read_first_line(path);
  if (result.status != Status::ok) {
    return I64Field::unavailable(result.status);
  }

  std::int64_t value = 0;
  if (!parse_i64(result.value, value)) {
    return I64Field::unavailable(Status::parse_error);
  }

  return I64Field::value(value);
}

inline std::vector<std::string> list_dir(const std::string& path) {
  std::vector<std::string> entries;
  std::error_code error;
  std::filesystem::directory_iterator iterator(path, error);
  if (error) {
    return entries;
  }

  const std::filesystem::directory_iterator end;
  while (iterator != end) {
    entries.push_back(iterator->path().filename().string());
    iterator.increment(error);
    if (error) {
      break;
    }
  }

  std::sort(entries.begin(), entries.end());
  return entries;
}

inline std::optional<std::string> read_symlink_target(const std::string& path) {
  std::error_code error;
  std::filesystem::path target = std::filesystem::read_symlink(path, error);
  if (error) {
    return std::nullopt;
  }
  return target.string();
}

inline StringField read_symlink_basename_field(const std::string& path) {
  std::optional<std::string> target = read_symlink_target(path);
  if (!target) {
    return StringField::unavailable(status_for_path_error(path));
  }

  return StringField::value(basename(*target));
}

inline std::vector<std::string> split_words(const std::string& text) {
  std::istringstream in(text);
  std::vector<std::string> words;
  std::string word;

  while (in >> word) {
    words.push_back(word);
  }

  return words;
}

inline std::set<std::string> word_set(const std::string& text) {
  std::vector<std::string> words = split_words(text);
  return std::set<std::string>(words.begin(), words.end());
}

inline std::optional<std::uint64_t> cpu_list_count(const std::string& text) {
  std::string value = trim(text);
  if (value.empty()) {
    return 0;
  }

  std::uint64_t count = 0;
  std::size_t start = 0;

  while (start <= value.size()) {
    std::size_t comma = value.find(',', start);
    std::string token = value.substr(
        start,
        comma == std::string::npos ? std::string::npos : comma - start
    );
    token = trim(token);

    std::size_t dash = token.find('-');
    if (dash == std::string::npos) {
      std::uint64_t cpu = 0;
      if (!parse_u64(token, cpu)) {
        return std::nullopt;
      }
      if (count == std::numeric_limits<std::uint64_t>::max()) {
        return std::nullopt;
      }
      ++count;
    } else {
      std::uint64_t first = 0;
      std::uint64_t last = 0;
      if (!parse_u64(token.substr(0, dash), first) ||
          !parse_u64(token.substr(dash + 1), last) ||
          last < first) {
        return std::nullopt;
      }
      const std::uint64_t range_count = last - first + 1;
      if (range_count == 0 ||
          count > std::numeric_limits<std::uint64_t>::max() - range_count) {
        return std::nullopt;
      }
      count += range_count;
    }

    if (comma == std::string::npos) {
      break;
    }
    start = comma + 1;
  }

  return count;
}

inline U64Field read_cpu_list_count_field(const std::string& path) {
  ReadResult result = read_first_line(path);
  if (result.status != Status::ok) {
    return U64Field::unavailable(result.status);
  }

  std::optional<std::uint64_t> count = cpu_list_count(result.value);
  if (!count) {
    return U64Field::unavailable(Status::parse_error);
  }

  return U64Field::value(*count);
}

inline std::string unquote_value(std::string value) {
  value = trim(value);
  if (value.size() < 2) {
    return value;
  }

  char quote = value.front();
  if ((quote != '"' && quote != '\'') || value.back() != quote) {
    return value;
  }

  std::string out;
  bool escaped = false;
  for (std::size_t i = 1; i + 1 < value.size(); ++i) {
    char c = value[i];
    if (escaped) {
      out.push_back(c);
      escaped = false;
    } else if (quote == '"' && c == '\\') {
      escaped = true;
    } else {
      out.push_back(c);
    }
  }

  return out;
}

using KeyValueMap = std::map<std::string, std::string>;

inline std::vector<KeyValueMap> parse_colon_blocks(const std::string& text) {
  std::vector<KeyValueMap> blocks;
  KeyValueMap current;
  std::istringstream in(text);
  std::string line;

  while (std::getline(in, line)) {
    if (trim(line).empty()) {
      if (!current.empty()) {
        blocks.push_back(current);
        current.clear();
      }
      continue;
    }

    std::size_t colon = line.find(':');
    if (colon == std::string::npos) {
      continue;
    }

    current[trim(line.substr(0, colon))] = trim(line.substr(colon + 1));
  }

  if (!current.empty()) {
    blocks.push_back(current);
  }

  return blocks;
}

inline std::optional<std::string> map_value(const KeyValueMap& values, const std::string& key) {
  auto it = values.find(key);
  if (it == values.end() || it->second.empty()) {
    return std::nullopt;
  }

  return it->second;
}

inline StringField string_from_map(
    const KeyValueMap& values,
    const std::vector<std::string>& keys
) {
  for (const std::string& key : keys) {
    auto value = map_value(values, key);
    if (value) {
      return StringField::value(*value);
    }
  }

  return StringField::unavailable(Status::not_found);
}

inline U64Field u64_from_map(
    const KeyValueMap& values,
    const std::vector<std::string>& keys
) {
  for (const std::string& key : keys) {
    auto text = map_value(values, key);
    if (!text) {
      continue;
    }

    std::uint64_t value = 0;
    if (!parse_u64(*text, value)) {
      return U64Field::unavailable(Status::parse_error);
    }
    return U64Field::value(value);
  }

  return U64Field::unavailable(Status::not_found);
}

inline bool parse_size_bytes(const std::string& text, std::uint64_t& bytes) {
  std::string value = trim(text);
  if (value.empty() || value.front() == '-') {
    return false;
  }

  errno = 0;
  char* end = nullptr;
  unsigned long long parsed = std::strtoull(value.c_str(), &end, 10);
  if (errno != 0 || end == value.c_str()) {
    return false;
  }

  std::string suffix = trim(end);
  std::transform(suffix.begin(), suffix.end(), suffix.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });

  std::uint64_t multiplier = 1;
  if (suffix == "k" || suffix == "kb" || suffix == "kib") {
    multiplier = 1024ull;
  } else if (suffix == "m" || suffix == "mb" || suffix == "mib") {
    multiplier = 1024ull * 1024ull;
  } else if (suffix == "g" || suffix == "gb" || suffix == "gib") {
    multiplier = 1024ull * 1024ull * 1024ull;
  } else if (!suffix.empty() && suffix != "b") {
    return false;
  }

  if (multiplier != 0 && parsed > std::numeric_limits<std::uint64_t>::max() / multiplier) {
    return false;
  }
  bytes = static_cast<std::uint64_t>(parsed) * multiplier;
  return true;
}

inline U64Field read_size_bytes_field(const std::string& path) {
  ReadResult result = read_first_line(path);
  if (result.status != Status::ok) {
    return U64Field::unavailable(result.status);
  }

  std::uint64_t bytes = 0;
  if (!parse_size_bytes(result.value, bytes)) {
    return U64Field::unavailable(Status::parse_error);
  }

  return U64Field::value(bytes);
}

inline BoolField bool_from_flag(const std::set<std::string>& flags, const std::string& name) {
  return BoolField::value(flags.find(name) != flags.end());
}

inline std::string first_existing_rooted(
    const CollectOptions& options,
    const std::vector<std::string>& paths
) {
  for (const std::string& path : paths) {
    std::string rooted;
    if (starts_with(path, "/proc")) {
      rooted = proc_path(options, path.substr(5));
    } else if (starts_with(path, "/sys")) {
      rooted = sys_path(options, path.substr(4));
    } else if (starts_with(path, "/etc")) {
      rooted = etc_path(options, path.substr(4));
    } else if (starts_with(path, "/dev")) {
      rooted = dev_path(options, path.substr(4));
    } else {
      rooted = path;
    }

    if (path_exists(rooted)) {
      return rooted;
    }
  }

  return {};
}

} // namespace detail
} // namespace archspec
