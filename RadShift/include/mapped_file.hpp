#pragma once

#include "../include/mapping_resource.hpp"
#include "../include/memory_view.hpp"
#include <filesystem>
#include <memory>

namespace radshift
{

class MappedFile
{
public:
    [[nodiscard]]
    static MappedFile open_readonly(
        const std::filesystem::path& path
    );

    MappedFile(const MappedFile&) = delete;

    MappedFile& operator=(
        const MappedFile&
    ) = delete;

    MappedFile(MappedFile&&) noexcept = default;

    MappedFile& operator=(
        MappedFile&&
    ) noexcept = default;

    ~MappedFile() = default;

    [[nodiscard]]
    MemoryView memory_view() const noexcept;

private:
    explicit MappedFile(
        std::shared_ptr<details::mappingResource> resource
    ) noexcept;

    std::shared_ptr<details::mappingResource> resource_;
};

}
