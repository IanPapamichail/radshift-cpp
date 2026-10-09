#include "../include/mapping_resource.hpp"
#include <utility>
#include <sys/mman.h>

using namespace std;

namespace radshift::details {

mappingResource::mappingResource(void* addr, size_t sz) noexcept: address(addr), size(sz) {}
mappingResource::~mappingResource() {release(); }

void mappingResource::release() noexcept {
  if (size != 0) {::munmap(address, size); }
  address = nullptr;
  size = 0;
}

mappingResource::mappingResource(mappingResource&& other) noexcept: address(exchange(other.address, nullptr)), size(exchange(other.size, 0)) {}
mappingResource &mappingResource::operator=(mappingResource &&other) noexcept {
  if(this!= &other){release(); address = exchange(other.address, nullptr); size = exchange(other.size, 0);} return *this;}

span<const byte> mappingResource::memory_view() const & noexcept {
  return{static_cast<const byte*>(address), size};
}}
