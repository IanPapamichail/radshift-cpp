#include "../include/detection_config.hpp"

#include <cmath>
#include <stdexcept>

namespace radshift
{

DetectionConfig::DetectionConfig(
    double baseline_rate,
    double alternative_rate,
    double threshold
)
    : baseline_rate_(baseline_rate),
      alternative_rate_(alternative_rate),
      threshold_(threshold)
{
    if (
        !std::isfinite(baseline_rate_)
        || !std::isfinite(alternative_rate_)
        || !std::isfinite(threshold_)
    )
    {
        throw std::invalid_argument(
            "Detection parameters must be finite"
        );
    }

    if (baseline_rate_ <= 0.0)
    {
        throw std::invalid_argument(
            "Baseline rate must be positive"
        );
    }

    if (alternative_rate_ <= baseline_rate_)
    {
        throw std::invalid_argument(
            "Alternative rate must exceed baseline rate"
        );
    }

    if (threshold_ <= 0.0)
    {
        throw std::invalid_argument(
            "Threshold must be positive"
        );
    }
}

double DetectionConfig::baseline_rate() const noexcept
{
    return baseline_rate_;
}

double DetectionConfig::alternative_rate() const noexcept
{
    return alternative_rate_;
}

double DetectionConfig::threshold() const noexcept
{
    return threshold_;
}

}
