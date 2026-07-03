#include "aview.hh"
#include "internal.hh"

namespace archspec {
namespace {

bool is_pseudo_block_device(const std::string& name) {
  return detail::starts_with(name, "loop") || detail::starts_with(name, "ram");
}

BoolField read_bool_number(const std::string& path) {
  U64Field value = detail::read_u64_field(path);
  if (!value.valid()) {
    return BoolField::unavailable(value.status());
  }

  return BoolField::value(value.value() != 0);
}

} // namespace

BlockDeviceList Collector::collect_block() const {
  BlockDeviceList list;
  std::string root = detail::sys_path(options_, "/block");

  for (const std::string& name : detail::list_dir(root)) {
    if (is_pseudo_block_device(name)) {
      continue;
    }

    std::string path = detail::join_path(root, name);
    if (!detail::path_is_dir(path)) {
      continue;
    }

    BlockDeviceInfo block;
    block.name = StringField::value(name);
    block.path = StringField::value(detail::dev_path(options_, "/" + name));
    block.model = detail::read_string_field(detail::join_path(path, "device/model"));
    block.vendor = detail::read_string_field(detail::join_path(path, "device/vendor"));
    block.rotational = read_bool_number(detail::join_path(path, "queue/rotational"));
    block.size_bytes = detail::read_u64_field(detail::join_path(path, "size"), 512);
    block.logical_block_size = detail::read_u64_field(
        detail::join_path(path, "queue/logical_block_size")
    );
    block.physical_block_size = detail::read_u64_field(
        detail::join_path(path, "queue/physical_block_size")
    );
    block.scheduler = detail::read_string_field(detail::join_path(path, "queue/scheduler"));

    list.entries.push_back(block);
  }

  return list;
}

BlockDeviceList collect_block() { return Collector{}.collect_block(); }

} // namespace archspec
