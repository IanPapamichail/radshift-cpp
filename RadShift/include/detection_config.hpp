#pragma once

namespace radshift
{

class DetectionConfig
{
public:
    DetectionConfig(
        double baseline_rate,
        double alternative_rate,
        double threshold
    );

    [[nodiscard]]
    double baseline_rate() const noexcept;

    [[nodiscard]]
    double alternative_rate() const noexcept;

    [[nodiscard]]
    double threshold() const noexcept;

private:
    double baseline_rate_;
    double alternative_rate_;
    double threshold_;
};

}
