#include "aview.hh"

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

namespace archspec {

namespace {

std::string escape_json(const std::string& input) {
  std::ostringstream out;

  for (unsigned char c : input) {
    switch (c) {
      case '"':
        out << "\\\"";
        break;
      case '\\':
        out << "\\\\";
        break;
      case '\b':
        out << "\\b";
        break;
      case '\f':
        out << "\\f";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\r':
        out << "\\r";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        if (c < 0x20) {
          out << "\\u"
              << std::hex
              << std::setw(4)
              << std::setfill('0')
              << static_cast<int>(c)
              << std::dec;
        } else {
          out << static_cast<char>(c);
        }
        break;
    }
  }

  return out.str();
}

std::string json_string(const std::string& value) {
  return "\"" + escape_json(value) + "\"";
}

std::string json_bool(bool value) {
  return value ? "true" : "false";
}

class JsonObject {
public:
  void add_raw(const std::string& key, const std::string& raw_value){
    add_comma();
    out_ << json_string(key) << ":" << raw_value;
  }

  void add_string(const std::string& key, const std::string& value){
    add_raw(key, json_string(value));
  }

  void add_u64(const std::string& key, const std::uint64_t value){
    add_comma();
    out_ << json_string(key) << ":" << value;
  }

  void add_i64(const std::string& key, const std::int64_t value){
    add_comma();
    out_ << json_string(key) << ":" << value;
  }

  void add_bool(const std::string& key, const bool value){
    add_raw(key, json_bool(value));
  }

  std::string str() const {
    return "{" + out_.str() + "}";
  }
private:
  void add_comma() {
    if (!first_) {
      out_ << ",";
    } else {
      first_ = false;
    }
  }
  bool first_ = true;
  std::ostringstream out_;
};

std::string field_to_json(const StringField& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));
  if (field.valid()) {
    obj.add_string("value", field.value());
  }
  return obj.str();
}

std::string field_to_json(const BoolField& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_bool("value", field.value());
  }

  return obj.str();
}

std::string field_to_json(const U32Field& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_u64("value", field.value());
  }

  return obj.str();
}

std::string field_to_json(const U64Field& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_u64("value", field.value());
  }

  return obj.str();
}

std::string field_to_json(const I64Field& field) {
  JsonObject obj;

  obj.add_bool("valid", field.valid());
  obj.add_string("status", to_string(field.status()));

  if (field.valid()) {
    obj.add_i64("value", field.value());
  }

  return obj.str();
}

} // anonymous namespace

std::string to_json(const SystemInfo& info) {
  return "{}";
}

std::string to_json(const OsInfo& info) {
  return "{}";
}

std::string to_json(const CpuInfo& info) {
  return "{}";
}

std::string to_json(const IsaFeatures& features) {
  return "{}";
}

std::string to_json(const CacheList& cache_list) {
  return "{}";
}

std::string to_json(const MemoryInfo& memory_info) {
  return "{}";
}

std::string to_json(const PciDeviceList& pci_devices) {
  return "{}";
}

std::string to_json(const GpuList& gpu_list) {
  return "{}";
}

std::string to_json(const PerfCounterInfo& perf_info) {
  return "{}";
}

std::string to_json(const BlockDeviceList& block_devices) {
  return "{}";
}

std::string to_json(const NetInterfaceList& net_interfaces) {
  return "{}";
}

std::string to_json(const ThermalInfo& thermal_info) {
  return "{}";
}

std::string to_json(const PowerInfo& power_info) {
  return "{}";
}

std::string to_json(const VirtualizationInfo& virtualization_info) {
  return "{}";
}

std::string to_json(const PlatformInfo& platform_info) {
  return "{}";
}

} // namespace archspec