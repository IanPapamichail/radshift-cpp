#include "../include/poisson_cusum.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace radshift
{

PoissonCusum::PoissonCusum(
    const DetectionConfig& config,
    std::uint64_t bin_duration_ns
)
    : threshold_(config.threshold())
{
    if (bin_duration_ns == 0)
    {
        throw std::invalid_argument(
            "Bin duration must be positive"
        );
    }

    const double duration_seconds =
        static_cast<double>(bin_duration_ns)
        / 1'000'000'000.0;

    const double rate_difference =
        config.alternative_rate()
        - config.baseline_rate();

    const double relative_increase =
        rate_difference / config.baseline_rate();

    if (std::isfinite(relative_increase))
    {
        log_rate_ratio_ = std::log1p(relative_increase);
    }
    else
    {
        log_rate_ratio_ =
            std::log(config.alternative_rate())
            - std::log(config.baseline_rate());
    }

    expected_difference_ =
        rate_difference * duration_seconds;

    if (
        !std::isfinite(log_rate_ratio_)
        || !std::isfinite(expected_difference_)
        || log_rate_ratio_ <= 0.0
        || expected_difference_ <= 0.0
    )
    {
        throw std::invalid_argument(
            "Parameters exceed supported numerical range"
        );
    }
}

void PoissonCusum::update(std::uint32_t count)
{
    if (
        processed_bins_
        == std::numeric_limits<std::uint64_t>::max()
    )
    {
        throw std::overflow_error(
            "Processed bin counter overflow"
        );
    }

    const double candidate =
        score_
        + static_cast<double>(count) * log_rate_ratio_
        - expected_difference_;

    if (!std::isfinite(candidate))
    {
        throw std::overflow_error(
            "CUSUM score exceeds numerical range"
        );
    }

    score_ = std::max(0.0, candidate);

    if (
        !first_alarm_bin_.has_value()
        && score_ >= threshold_
    )
    {
        first_alarm_bin_ = processed_bins_;
    }

    ++processed_bins_;
}

double PoissonCusum::score() const noexcept
{
    return score_;
}

std::optional<std::uint64_t>
PoissonCusum::first_alarm_bin() const noexcept
{
    return first_alarm_bin_;
}

}
