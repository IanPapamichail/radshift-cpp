#pragma once

#include "mapping_resource.hpp"

#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>

namespace radshift
{

class MappedFile;
class CountView;

class MemoryView
{
public:
    [[nodiscard]]
    std::size_t size() const noexcept
    {
        return resource_
            ? resource_->memory_view().size()
            : 0;
    }

    [[nodiscard]]
    bool empty() const noexcept
    {
        return size() == 0;
    }

    [[nodiscard]]
    std::byte byte_at(std::size_t index) const
    {
        if (index >= size())
        {
            throw std::out_of_range(
                "Byte index outside memory view"
            );
        }

        return resource_->memory_view()[index];
    }

private:
    friend class MappedFile;
    friend class CountView;

    explicit MemoryView(
        std::shared_ptr<const details::mappingResource> resource
    ) noexcept
        : resource_(std::move(resource))
    {
    }

    std::shared_ptr<const details::mappingResource> resource_;
};

}
