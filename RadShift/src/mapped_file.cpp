#include "../include/mapped_file.hpp"
#include "../include/count_view.hpp"

#include <cerrno>
#include <cstdint>
#include <limits>
#include <memory>
#include <stdexcept>
#include <system_error>
#include <utility>
#include <cstring>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace radshift {

namespace
{

template<typename T>
T load_little_endian(const std::byte* source) noexcept
{
    static_assert(std::is_integral_v<T> && std::is_unsigned_v<T>);
    if constexpr (std::endian::native == std::endian::little)
    {
        T value{};
        std::memcpy(&value, source, sizeof(T));
        return value;
    }
    else
    {
        T value{};
        for (std::size_t i = 0; i < sizeof(T); ++i)
        {
            const auto byte = std::to_integer<unsigned char>(source[i]);
            value |= static_cast<T>(byte) << (8 * i);
        }
        return value;
    }
}

}

CountView CountView::parse(MemoryView memory)
{
    constexpr std::size_t header_size = 32;
    constexpr std::size_t count_size = 4;

    if (memory.size() < header_size)
    {
        throw std::runtime_error(
            "Truncated RADCOUNT header"
        );
    }

    if (!memory.resource_)
    {
        throw std::runtime_error(
            "Memory view has no mapping"
        );
    }

    const auto bytes =
        memory.resource_->memory_view();

    if (std::memcmp(bytes.data(), "RADCOUNT", 8) != 0)
    {
        throw std::runtime_error(
            "Invalid RADCOUNT magic"
        );
    }

    const auto version =
        load_little_endian<std::uint32_t>(
            bytes.data() + 8
        );

    if (version != 1)
    {
        throw std::runtime_error(
            "Unsupported RADCOUNT version"
        );
    }

    const auto stored_header_size =
        load_little_endian<std::uint32_t>(
            bytes.data() + 12
        );

    if (stored_header_size != header_size)
    {
        throw std::runtime_error(
            "Invalid RADCOUNT header size"
        );
    }

    const auto count =
        load_little_endian<std::uint64_t>(
            bytes.data() + 16
        );

    const auto duration =
        load_little_endian<std::uint64_t>(
            bytes.data() + 24
        );

    if (count == 0)
    {
        throw std::runtime_error(
            "Trace contains no bins"
        );
    }

    if (duration == 0)
    {
        throw std::runtime_error(
            "Bin duration must be positive"
        );
    }

    const auto payload_size =
        bytes.size() - header_size;

    if (
        payload_size % count_size != 0
        || count != payload_size / count_size
)
    {
        throw std::runtime_error(
            "Payload size does not match bin count"
        );
    }
    return CountView{
        std::move(memory),
        count,
        duration
    };
}
namespace
{

// Κατέχει τον περιγραφέα και τον κλείνει αυτόματα.
class FileDescriptor
{
public:
    explicit FileDescriptor(int value) noexcept
        : value_(value)
    {
    }

    ~FileDescriptor()
    {
        if (value_ >= 0)
        {
            ::close(value_);
        }
    }

    FileDescriptor(const FileDescriptor&) = delete;

    FileDescriptor& operator=(
        const FileDescriptor&
    ) = delete;

    [[nodiscard]]
    int get() const noexcept
    {
        return value_;
    }

private:
    int value_;
};

}

MappedFile::MappedFile(
    std::shared_ptr<details::mappingResource> resource
) noexcept
    : resource_(std::move(resource))
{
}

MappedFile MappedFile::open_readonly(
    const std::filesystem::path& path
)
{
    FileDescriptor fd{
        ::open(path.c_str(), O_RDONLY | O_CLOEXEC)
    };

    if (fd.get() < 0)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "open"
        );
    }

    struct stat info{};
    if (::fstat(fd.get(), &info) != 0)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "fstat"
        );
    }
    if (!S_ISREG(info.st_mode))
    {
        throw std::runtime_error(
            "Input must be a regular file"
        );
    }
    if (info.st_size <= 0)
    {
        throw std::runtime_error(
            "Input file must not be empty"
        );
    }
    if (
        static_cast<std::uintmax_t>(info.st_size)
        > std::numeric_limits<std::size_t>::max()
    )
    {
        throw std::runtime_error(
            "File exceeds addressable size"
        );
    }
    const auto size =
        static_cast<std::size_t>(info.st_size);
    void* address = ::mmap(
        nullptr,
        size,
        PROT_READ,
        MAP_PRIVATE,
        fd.get(),
        0
    );
    if (address == MAP_FAILED)
    {
        throw std::system_error(
            errno,
            std::generic_category(),
            "mmap"
        );
    }
    details::mappingResource mapping{address, size};
    auto resource =
        std::make_shared<details::mappingResource>(
            std::move(mapping)
        );

    return MappedFile{std::move(resource)};
}

MemoryView MappedFile::memory_view() const noexcept
{
    // Η όψη αποκτά κοινή ιδιοκτησία χωρίς αντιγραφή των bytes.
    return MemoryView{resource_};
}
std::uint32_t CountView::count_at(
    std::uint64_t index
) const
{
    if (!memory_.resource_)
    {
        throw std::logic_error(
            "CountView has no mapping"
        );
    }

    if (index >= count_)
    {
        throw std::out_of_range(
            "Count index outside trace"
        );
    }

    constexpr std::size_t header_size = 32;
    constexpr std::size_t count_size = 4;

    const auto offset =
        header_size
        + static_cast<std::size_t>(index) * count_size;

    const auto bytes =
        memory_.resource_->memory_view();

    return load_little_endian<std::uint32_t>(
        bytes.data() + offset
    );
}
}
