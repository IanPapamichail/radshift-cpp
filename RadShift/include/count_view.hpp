#pragma once

#include "../include/memory_view.hpp"

#include <cstdint>

namespace radshift
{

class CountView
{
public:
    [[nodiscard]]
    static CountView parse(MemoryView memory);

    [[nodiscard]]
    std::uint64_t size() const noexcept;

    [[nodiscard]]
    std::uint64_t bin_duration_ns() const noexcept;

    [[nodiscard]]
    std::uint32_t count_at(std::uint64_t index) const;

private:
    CountView(
        MemoryView memory,
        std::uint64_t count,
        std::uint64_t bin_duration_ns
    ) noexcept;

    MemoryView memory_;

    std::uint64_t count_;
    std::uint64_t bin_duration_ns_;
};

}
