#include "../include/detection_config.hpp"
#include "../include/poisson_cusum.hpp"

#include <array>
#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace
{

void require(bool condition, const char* message)
{
    if (!condition)
    {
        throw std::runtime_error(message);
    }
}

void require_close(
    double actual,
    double expected,
    double tolerance,
    const char* message
)
{
    require(
        std::isfinite(actual)
        && std::abs(actual - expected) <= tolerance,
        message
    );
}

template<typename Function>
void require_invalid_argument(
    Function action,
    const char* message
)
{
    bool rejected = false;

    try
    {
        action();
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }

    require(rejected, message);
}

void test_known_trace()
{
    const radshift::DetectionConfig config{
        1.0, 3.0, 4.0
    };

    radshift::PoissonCusum detector{
        config, 1'000'000'000
    };

    constexpr std::array<std::uint32_t, 4> counts{
        0, 1, 2, 6
    };

    const std::array<double, 4> expected_scores{
        0.0,
        0.0,
        2.0 * std::log(3.0) - 2.0,
        8.0 * std::log(3.0) - 4.0
    };

    require(
        !detector.first_alarm_bin().has_value(),
        "Unexpected alarm before processing"
    );

    for (std::size_t i = 0; i < counts.size(); ++i)
    {
        detector.update(counts[i]);

        require_close(
            detector.score(),
            expected_scores[i],
            1e-12,
            "Incorrect CUSUM score"
        );

        if (i < 3)
        {
            require(
                !detector.first_alarm_bin().has_value(),
                "Alarm raised too early"
            );
        }
    }

    const auto alarm = detector.first_alarm_bin();

    require(
        alarm.has_value() && alarm.value() == 3,
        "First alarm must be at bin 3"
    );

    detector.update(10);

    require(
        detector.first_alarm_bin().value() == 3,
        "First alarm must remain unchanged"
    );
}

void test_zero_counts()
{
    const radshift::DetectionConfig config{
        1.0, 3.0, 4.0
    };

    radshift::PoissonCusum detector{
        config, 1'000'000'000
    };

    for (int i = 0; i < 100; ++i)
    {
        detector.update(0);

        require(
            detector.score() == 0.0,
            "Zero counts must keep the score at zero"
        );
    }

    require(
        !detector.first_alarm_bin().has_value(),
        "Zero-count trace must not raise an alarm"
    );
}

void test_alarm_at_first_bin()
{
    const radshift::DetectionConfig config{
        1.0, 3.0, 4.0
    };

    radshift::PoissonCusum detector{
        config, 1'000'000'000
    };

    detector.update(6);

    const auto alarm = detector.first_alarm_bin();

    require(
        alarm.has_value() && alarm.value() == 0,
        "Bin zero must be a valid alarm index"
    );
}

void test_invalid_parameters()
{
    require_invalid_argument(
        [] {
            const radshift::DetectionConfig config{
                0.0, 3.0, 4.0
            };
            (void)config;
        },
        "Zero baseline must be rejected"
    );

    require_invalid_argument(
        [] {
            const radshift::DetectionConfig config{
                1.0, 1.0, 4.0
            };
            (void)config;
        },
        "Equal rates must be rejected"
    );

    require_invalid_argument(
        [] {
            const radshift::DetectionConfig config{
                1.0, 3.0, -1.0
            };
            (void)config;
        },
        "Negative threshold must be rejected"
    );

    require_invalid_argument(
        [] {
            const radshift::DetectionConfig config{
                1.0,
                3.0,
                std::numeric_limits<double>::quiet_NaN()
            };
            (void)config;
        },
        "NaN threshold must be rejected"
    );

    require_invalid_argument(
        [] {
            const radshift::DetectionConfig config{
                1.0, 3.0, 4.0
            };

            radshift::PoissonCusum detector{
                config, 0
            };

            (void)detector;
        },
        "Zero bin duration must be rejected"
    );
}

}

int main()
{
    try
    {
        test_known_trace();
        test_zero_counts();
        test_alarm_at_first_bin();
        test_invalid_parameters();

        std::cout << "All CUSUM tests passed\n";
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
