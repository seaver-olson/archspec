#include "aview.hh"
#include "internal.hh"

#include <arpa/inet.h>
#include <cstring>
#include <ifaddrs.h>
#include <map>
#include <netinet/in.h>
#include <sstream>
#include <sys/socket.h>
#include <vector>

namespace archspec {
namespace {

struct InterfaceAddresses {
  std::vector<std::string> ipv4;
  std::vector<std::string> ipv6;
};

std::string join_values(const std::vector<std::string>& values) {
  std::ostringstream out;
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values[i];
  }
  return out.str();
}

std::map<std::string, InterfaceAddresses> collect_addresses() {
  std::map<std::string, InterfaceAddresses> addresses;
  ifaddrs* interfaces = nullptr;

  if (getifaddrs(&interfaces) != 0) {
    return addresses;
  }

  for (ifaddrs* iface = interfaces; iface != nullptr; iface = iface->ifa_next) {
    if (iface->ifa_addr == nullptr) {
      continue;
    }

    int family = iface->ifa_addr->sa_family;
    char buffer[INET6_ADDRSTRLEN] = {};

    if (family == AF_INET) {
      auto* addr = reinterpret_cast<sockaddr_in*>(iface->ifa_addr);
      if (inet_ntop(AF_INET, &addr->sin_addr, buffer, sizeof(buffer)) != nullptr) {
        addresses[iface->ifa_name].ipv4.push_back(buffer);
      }
    } else if (family == AF_INET6) {
      auto* addr = reinterpret_cast<sockaddr_in6*>(iface->ifa_addr);
      if (inet_ntop(AF_INET6, &addr->sin6_addr, buffer, sizeof(buffer)) != nullptr) {
        addresses[iface->ifa_name].ipv6.push_back(buffer);
      }
    }
  }

  freeifaddrs(interfaces);
  return addresses;
}

BoolField read_bool_number(const std::string& path) {
  U64Field value = detail::read_u64_field(path);
  if (!value.valid()) {
    return BoolField::unavailable(value.status());
  }

  return BoolField::value(value.value() != 0);
}

U64Field read_speed_mbps(const std::string& path) {
  I64Field value = detail::read_i64_field(path);
  if (!value.valid()) {
    return U64Field::unavailable(value.status());
  }
  if (value.value() < 0) {
    return U64Field::unavailable(Status::not_found);
  }

  return U64Field::value(static_cast<std::uint64_t>(value.value()));
}

} // namespace

NetInterfaceList Collector::collect_net() const {
  NetInterfaceList list;
  std::string root = detail::sys_path(options_, "/class/net");
  std::map<std::string, InterfaceAddresses> addresses = collect_addresses();

  for (const std::string& name : detail::list_dir(root)) {
    std::string path = detail::join_path(root, name);
    if (!detail::path_is_dir(path)) {
      continue;
    }

    NetInterfaceInfo iface;
    iface.name = StringField::value(name);
    iface.mac_address = detail::read_string_field(detail::join_path(path, "address"));
    iface.mtu = detail::read_u64_field(detail::join_path(path, "mtu"));
    iface.speed_mbps = read_speed_mbps(detail::join_path(path, "speed"));
    iface.duplex = detail::read_string_field(detail::join_path(path, "duplex"));
    iface.carrier = read_bool_number(detail::join_path(path, "carrier"));
    iface.driver = detail::read_symlink_basename_field(detail::join_path(path, "device/driver"));

    auto found = addresses.find(name);
    if (found != addresses.end()) {
      iface.ipv4_addresses = StringField::value(join_values(found->second.ipv4));
      iface.ipv6_addresses = StringField::value(join_values(found->second.ipv6));
    } else {
      iface.ipv4_addresses = StringField::unavailable(Status::not_found);
      iface.ipv6_addresses = StringField::unavailable(Status::not_found);
    }

    list.entries.push_back(iface);
  }

  return list;
}

NetInterfaceList collect_net() { return Collector{}.collect_net(); }

} // namespace archspec
