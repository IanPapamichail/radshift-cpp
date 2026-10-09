#include "../include/count_view.hpp"
#include "../include/detection_config.hpp"
#include "../include/mapped_file.hpp"
#include "../include/poisson_cusum.hpp"

#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <stdexcept>

namespace
{

double parse_number(
    const char* argument,
    const char* parameter_name
)
{
    const std::string text{argument};

    std::size_t consumed = 0;
    double value = 0.0;

    try
    {
        value = std::stod(text, &consumed);
    }
    catch (const std::invalid_argument&)
    {
        throw std::invalid_argument(
            std::string{"Invalid number for "}
            + parameter_name
            + ": "
            + text
        );
    }
    catch (const std::out_of_range&)
    {
        throw std::invalid_argument(
            std::string{"Number outside supported range for "}
            + parameter_name
            + ": "
            + text
        );
    }

    if (consumed != text.size())
    {
        throw std::invalid_argument(
            std::string{"Unexpected characters in "}
            + parameter_name
            + ": "
            + text
        );
    }

    return value;
}

void print_usage(const char* program)
{
    std::cerr
        << "Usage: "
        << program
        << " <trace.rct>"
        << " <baseline_rate>"
        << " <alternative_rate>"
        << " <threshold>\n"
        << "Rates are expressed in counts/second.\n";
}

}

int main(int argc, char* argv[])
{
    if (argc != 5)
    {
        print_usage(argv[0]);
        return 2;
    }

    try
    {
        const radshift::DetectionConfig config{
            parse_number(argv[2], "baseline_rate"),
            parse_number(argv[3], "alternative_rate"),
            parse_number(argv[4], "threshold")
        };

        const auto trace = [path = argv[1]]
        {
            auto file =
                radshift::MappedFile::open_readonly(path);

            return radshift::CountView::parse(
                file.memory_view()
            );
        }();

        radshift::PoissonCusum detector{
            config,
            trace.bin_duration_ns()
        };

        for (std::uint64_t i = 0; i < trace.size(); ++i)
        {
            detector.update(trace.count_at(i));
        }

        std::cout << std::fixed << std::setprecision(6);

        std::cout
            << "Baseline rate: "
            << config.baseline_rate()
            << " counts/s\n";

        std::cout
            << "Alternative rate: "
            << config.alternative_rate()
            << " counts/s\n";

        std::cout
            << "Threshold: "
            << config.threshold()
            << '\n';

        std::cout
            << "Bins: "
            << trace.size()
            << '\n';

        std::cout
            << "Bin duration (ns): "
            << trace.bin_duration_ns()
            << '\n';

        std::cout
            << "Final CUSUM score: "
            << detector.score()
            << '\n';

        const auto alarm = detector.first_alarm_bin();

        if (alarm.has_value())
        {
            std::cout
                << "First alarm bin: "
                << alarm.value()
                << '\n';
        }
        else
        {
            std::cout << "No alarm\n";
        }
    }
    catch (const std::exception& error)
    {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}
