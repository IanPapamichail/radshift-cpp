#include "../include/count_view.hpp"
#include "../include/mapped_file.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <unistd.h>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <system_error>

namespace
{

using Bytes = std::array<std::byte, 48>;

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

// Προσωρινός φάκελος με αυτόματο καθαρισμό.
class TemporaryDirectory
{
public:
    TemporaryDirectory()
    {
        char pattern[] = "/tmp/radshift-tests-XXXXXX";

        const char* created = ::mkdtemp(pattern);

        if (created == nullptr)
        {
            throw std::runtime_error(
                "Cannot create temporary directory"
            );
        }

        path_ = created;
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path_, error);
    }

    TemporaryDirectory(const TemporaryDirectory&) = delete;

    TemporaryDirectory& operator=(
        const TemporaryDirectory&
    ) = delete;

    const std::filesystem::path& path() const noexcept
    {
        return path_;
    }

private:
    std::filesystem::path path_;
};

void store_integer(
    Bytes& bytes,
    std::size_t offset,
    std::uint64_t value,
    std::size_t width
)
{
    for (std::size_t i = 0; i < width; ++i)
    {
        bytes.at(offset + i) = static_cast<std::byte>(
            (value >> (8 * i)) & 0xff
        );
    }
}

Bytes make_fixture()
{
    Bytes bytes{};

    constexpr char magic[] = "RADCOUNT";

    for (std::size_t i = 0; i < 8; ++i)
    {
        bytes[i] = static_cast<std::byte>(magic[i]);
    }

    store_integer(bytes, 8, 1, 4);
    store_integer(bytes, 12, 32, 4);
    store_integer(bytes, 16, 4, 8);
    store_integer(bytes, 24, 1'000'000'000, 8);

    store_integer(bytes, 32, 0, 4);
    store_integer(bytes, 36, 1, 4);
    store_integer(bytes, 40, 2, 4);
    store_integer(bytes, 44, 6, 4);

    return bytes;
}

void write_file(
    const std::filesystem::path& path,
    const Bytes& bytes,
    std::size_t length
)
{
    require(
        length <= bytes.size(),
        "Invalid fixture length"
    );

    std::ofstream output{
        path,
        std::ios::binary | std::ios::trunc
    };

    output.exceptions(
        std::ios::failbit | std::ios::badbit
    );

    output.write(
        reinterpret_cast<const char*>(bytes.data()),
        static_cast<std::streamsize>(length)
    );

    output.close();
}

radshift::CountView read_trace(
    const std::filesystem::path& path
)
{
    auto file =
        radshift::MappedFile::open_readonly(path);

    return radshift::CountView::parse(
        file.memory_view()
    );
}

void require_rejected(
    const std::filesystem::path& path,
    const Bytes& bytes,
    std::size_t length
)
{
    write_file(path, bytes, length);

    // Χαρτογραφούμε έξω από το try ώστε ένα σφάλμα
    // ανοίγματος να μην θεωρηθεί επιτυχία του parser.
    auto file =
        radshift::MappedFile::open_readonly(path);

    bool rejected = false;

    try
    {
        (void)radshift::CountView::parse(
            file.memory_view()
        );
    }
    catch (const std::runtime_error&)
    {
        rejected = true;
    }

    require(rejected, "Malformed trace was accepted");
}

void test_valid_trace(
    const std::filesystem::path& directory
)
{
    const auto path = directory / "valid.rct";
    const auto bytes = make_fixture();

    write_file(path, bytes, bytes.size());

    // Το MappedFile μέσα στη read_trace καταστρέφεται
    // πριν χρησιμοποιήσουμε εδώ το CountView.
    const auto trace = read_trace(path);

    require(trace.size() == 4, "Incorrect bin count");

    require(
        trace.bin_duration_ns() == 1'000'000'000,
        "Incorrect bin duration"
    );

    constexpr std::array<std::uint32_t, 4> expected{
        0, 1, 2, 6
    };

    for (std::size_t i = 0; i < expected.size(); ++i)
    {
        require(
            trace.count_at(i) == expected[i],
            "Incorrect decoded count"
        );
    }

    bool rejected = false;

    try
    {
        (void)trace.count_at(4);
    }
    catch (const std::out_of_range&)
    {
        rejected = true;
    }

    require(rejected, "Out-of-range index was accepted");
}

void test_byte_order(
    const std::filesystem::path& directory
)
{
    auto bytes = make_fixture();

    store_integer(bytes, 32, 0x12345678, 4);

    const auto path = directory / "byte_order.rct";

    write_file(path, bytes, bytes.size());

    const auto trace = read_trace(path);

    require(
        trace.count_at(0) == 0x12345678,
        "Incorrect little-endian decoding"
    );
}

void test_invalid_files(
    const std::filesystem::path& directory
)
{
    const auto valid = make_fixture();

    require_rejected(
        directory / "short_header.rct",
        valid,
        12
    );

    require_rejected(
        directory / "short_payload.rct",
        valid,
        47
    );

    auto bytes = valid;
    bytes[0] = std::byte{0};

    require_rejected(
        directory / "wrong_magic.rct",
        bytes,
        bytes.size()
    );

    bytes = valid;
    store_integer(bytes, 8, 2, 4);

    require_rejected(
        directory / "wrong_version.rct",
        bytes,
        bytes.size()
    );

    bytes = valid;
    store_integer(bytes, 12, 64, 4);

    require_rejected(
        directory / "wrong_header_size.rct",
        bytes,
        bytes.size()
    );

    bytes = valid;
    store_integer(bytes, 24, 0, 8);

    require_rejected(
        directory / "zero_duration.rct",
        bytes,
        bytes.size()
    );

    bytes = valid;
    store_integer(bytes, 16, 0, 8);

    require_rejected(
        directory / "zero_count.rct",
        bytes,
        bytes.size()
    );

    bytes = valid;

    store_integer(
        bytes,
        16,
        std::numeric_limits<std::uint64_t>::max(),
        8
    );

    require_rejected(
        directory / "excessive_count.rct",
        bytes,
        bytes.size()
    );
}

}

int main()
{
    try
    {
        const TemporaryDirectory directory;

        test_valid_trace(directory.path());
        test_byte_order(directory.path());
        test_invalid_files(directory.path());

        std::cout << "All storage tests passed\n";
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "Test failed: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}
