#pragma once

#include "detection_config.hpp"

#include <cstdint>
#include <optional>

namespace radshift
{

class PoissonCusum
{
public:
    PoissonCusum(
        const DetectionConfig& config,
        std::uint64_t bin_duration_ns
    );

    void update(std::uint32_t count);

    [[nodiscard]]
    double score() const noexcept;

    [[nodiscard]]
    std::optional<std::uint64_t>
    first_alarm_bin() const noexcept;

private:
    double log_rate_ratio_;
    double expected_difference_;
    double threshold_;

    double score_ = 0.0;
    std::uint64_t processed_bins_ = 0;

    std::optional<std::uint64_t> first_alarm_bin_;
};

}
