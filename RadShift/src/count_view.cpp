#include "../include/count_view.hpp"
#include <utility>
#include <cstddef>
#include <cstring>

namespace radshift
{
CountView::CountView(
    MemoryView memory,
    std::uint64_t count,
    std::uint64_t bin_duration_ns
) noexcept : memory_(std::move(memory)), count_(count), bin_duration_ns_(bin_duration_ns){}
std::uint64_t CountView::size() const noexcept{return count_;}
std::uint64_t CountView::bin_duration_ns() const noexcept{return bin_duration_ns_;}
}
