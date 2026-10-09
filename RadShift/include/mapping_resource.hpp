#pragma once

#include <cstddef>
#include <span>

using namespace std;
namespace radshift {
class MappedFile;

namespace details {

class mappingResource {
public:
  mappingResource(mappingResource&& other) noexcept;
  ~mappingResource();

  mappingResource(const mappingResource &) = delete;

  mappingResource &operator=(mappingResource &&other) noexcept;

  [[nodiscard]]
  span<const byte> memory_view() const & noexcept;
  span<const byte> memory_view() const && = delete;

private:
  friend class radshift::MappedFile;

  mappingResource(void *address, size_t size) noexcept;

  void release() noexcept;

  void* address = nullptr;
  size_t size = 0;

};
}

}
