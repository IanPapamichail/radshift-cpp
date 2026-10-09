# RadShift

### Radiation Count-Rate Change Detection in Modern C++20

![C++20](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Build](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)
![Platforms](https://img.shields.io/badge/platforms-macOS%20%7C%20Linux-lightgrey)
![Method](https://img.shields.io/badge/detection-Poisson%20CUSUM-purple)
![Input](https://img.shields.io/badge/input-memory--mapped-orange)

RadShift is a C++20 command-line tool for detecting increases in radiation count rates using a Poisson cumulative sum detector (CUSUM).

It reads binary files containing event counts collected over fixed-duration intervals, validates their structure, and identifies the first interval where the accumulated evidence reaches a configured threshold.

The included demonstration uses synthetic counts. No radiation detector, special hardware, or external service is required.

## Motivation

Radiation detectors record discrete events. Even when the background rate remains constant, the number of events observed during each measurement interval fluctuates.

A single unusually high count does not necessarily indicate a change. RadShift accumulates statistical evidence across successive intervals to detect an increase above a known background rate.

The project combines a practical scientific-computing problem with explicit resource ownership, defensive binary parsing, and a small, testable statistical core.

## Features

- Poisson CUSUM detection for increases above a known background rate.
- Configurable background rate, alternative rate, and alarm threshold.
- Read-only memory-mapped input using POSIX `mmap`.
- Shared ownership that keeps mapped data alive while views use it.
- Binary-format validation before accessing count records.
- Explicit little-endian decoding.
- Bounds-checked count access.
- First-alarm reporting with zero-based interval indexing.
- Automated tests for storage, parsing, and detection.
- CMake build with CTest integration.

## Architecture

The design separates file ownership, data interpretation, configuration, and statistical detection.

| Component | Responsibility |
|---|---|
| `detail::MappingResource` | Owns the mapping and releases it through RAII. |
| `MappedFile` | Opens and maps a regular file for read-only access. |
| `MemoryView` | Retains shared ownership and exposes controlled byte access. |
| `CountView` | Validates the binary format and provides decoded counts. |
| `DetectionConfig` | Validates the rates and detection threshold. |
| `PoissonCusum` | Updates the detection score and records the first alarm. |
| CLI | Connects file input, configuration, processing, and reporting. |

`MappedFile`, `MemoryView`, and `CountView` retain shared ownership of the underlying mapping. A `CountView` therefore remains usable after the original `MappedFile` object is destroyed.

`PoissonCusum` receives one count at a time and has no dependency on files or memory mapping. This separation allows a future live-data adapter to reuse the detector.

### Ownership and safety

- File descriptors are closed through RAII.
- Mappings are released when their final owner is destroyed.
- Invalid headers and inconsistent payload sizes are rejected.
- Declared record counts are validated without overflowing an untrusted size multiplication.
- Counts are decoded without assuming pointer alignment.
- Invalid numerical configuration is rejected.
- Non-finite detector updates are rejected.

Mapped input files must remain unchanged during processing. Shared ownership protects the mapping's lifetime; it does not protect against another process modifying or truncating the file.

## Detection Method

For each interval, RadShift compares two Poisson models:

- **Background model:** rate `λ₀`.
- **Alternative model:** higher rate `λ₁`, with `λ₁ > λ₀`.

Let:

- `xₜ` be the observed count in interval `t`.
- `Δt` be the interval duration in seconds.
- `h` be the positive alarm threshold.

The log-likelihood increment is:

```text
Lₜ = xₜ · log(λ₁ / λ₀) − (λ₁ − λ₀) · Δt
```

The accumulated score is:

```text
Sₜ = max(0, Sₜ₋₁ + Lₜ)
```

An alarm is recorded when:

```text
Sₜ ≥ h
```

Evidence favoring the higher rate increases the score. Evidence favoring the background reduces it, with a lower bound of zero.

The detector records the first alarm and continues processing the remaining intervals. The reported final score is the score after the complete file has been processed.

## Requirements

- macOS or Linux.
- A compiler supporting C++20.
- CMake 3.20 or newer.
- Python 3 to generate the demonstration fixture.

The C++ implementation uses the standard library and POSIX APIs. It requires no third-party C++ libraries.

## Build

From the repository root:

```bash
cmake -S . -B build-cmake \
  -DCMAKE_BUILD_TYPE=Release \
  -DBUILD_TESTING=ON

cmake --build build-cmake --parallel
```

## Run Tests

```bash
ctest --test-dir build-cmake --output-on-failure
```

The test suite covers:

- Known CUSUM scores and alarm timing.
- Zero-count sequences.
- An alarm in the first interval.
- Retention of the first alarm during subsequent updates.
- Invalid detection parameters.
- Valid binary files and count access.
- Mapping lifetime after the original file owner is destroyed.
- Little-endian decoding.
- Out-of-range count access.
- Truncated headers and payloads.
- Invalid magic, version, and header size.
- Zero interval duration.
- Invalid or inconsistent declared record counts.

## Reproducible Demo

Generate the fixture:

```bash
python3 scripts/create_fixture.py
```

This creates:

```text
examples/four_bins.rct
```

The fixture contains four one-second intervals:

```text
Counts: [0, 1, 2, 6]
```

Run the detector:

```bash
./build-cmake/radshift examples/four_bins.rct 1 3 4
```

Arguments:

```text
radshift <trace.rct> <baseline_rate> <alternative_rate> <threshold>
```

For this example:

| Parameter | Value |
|---|---:|
| Background rate | 1 count/second |
| Alternative rate | 3 counts/second |
| Alarm threshold | 4 |
| Interval duration | 1 second |

Expected numerical results:

| Interval index | Count | Score after update |
|---:|---:|---:|
| 0 | 0 | 0.000000 |
| 1 | 1 | 0.000000 |
| 2 | 2 | 0.197225 |
| 3 | 6 | 4.788898 |

The first alarm occurs at interval index `3`, corresponding to the fourth measurement interval.

Increase the threshold:

```bash
./build-cmake/radshift examples/four_bins.rct 1 3 10
```

The final score remains approximately `4.788898`, but no alarm is recorded because the score never reaches `10`.

### Input validation example

```bash
./build-cmake/radshift examples/four_bins.rct 1 3abc 4
```

The CLI rejects the partially parsed numerical argument and returns a nonzero exit code.

### Exit codes

| Code | Meaning |
|---:|---|
| `0` | Processing completed successfully, with or without an alarm. |
| `1` | Input, configuration, or processing error. |
| `2` | Incorrect number of command-line arguments. |

## Binary Format: RADCOUNT v1

All integer fields use little-endian encoding.

| Byte offset | Size | Type | Meaning |
|---:|---:|---|---|
| 0 | 8 bytes | ASCII | Magic value: `RADCOUNT` |
| 8 | 4 bytes | `uint32` | Format version: `1` |
| 12 | 4 bytes | `uint32` | Header size: `32` |
| 16 | 8 bytes | `uint64` | Number of count intervals |
| 24 | 8 bytes | `uint64` | Interval duration in nanoseconds |
| 32 | `4 × N` bytes | `uint32[N]` | Counts for successive intervals |

A valid file must contain:

- A complete 32-byte header.
- The expected magic, version, and header size.
- A positive number of intervals.
- A positive interval duration.
- Exactly one four-byte count per declared interval.

Individual counts may be zero.

The demonstration file occupies 48 bytes: a 32-byte header and four four-byte counts.

## Repository Layout

```text
include/radshift/
    mapping_resource.hpp
    mapped_file.hpp
    memory_view.hpp
    count_view.hpp
    detection_config.hpp
    poisson_cusum.hpp

src/
    mapping_resource.cpp
    mapped_file.cpp
    count_view.cpp
    detection_config.cpp
    poisson_cusum.cpp
    main.cpp

tests/
    test_storage.cpp
    test_poisson_cusum.cpp

scripts/
    create_fixture.py

examples/
    four_bins.rct

CMakeLists.txt
README.md
.gitignore
```

## Performance Design

RadShift maps the input file instead of first copying its complete payload into an application-owned buffer.

Count records are decoded on demand, and the detector keeps constant-size state. The processing loop does not allocate memory or print output for individual intervals.

For `N` intervals:

- Detection takes `O(N)` time.
- Detector state occupies `O(1)` space.
- The virtual mapping spans the input file; resident memory depends on operating-system paging and file access.

Memory mapping does not eliminate disk I/O or page faults. Its performance relative to buffered reading depends on the workload and operating system.

No throughput or speedup claims are made yet. A reproducible comparison with buffered input is planned.

## Scope and Assumptions

The current implementation assumes:

- Counts are modeled as independent Poisson observations.
- Measurement intervals have a fixed duration.
- The background and alternative rates are supplied by the user.
- The change of interest is an increase in count rate.
- Input files are complete and remain unchanged during processing.

The threshold is user-configured. It has not been calibrated to a specific false-alarm probability or average run length.

The demonstration is a deterministic synthetic fixture. It verifies the implementation's behavior but does not establish detection performance on real measurements.

The current version does not estimate radiation dose, identify isotopes, or model detector dead time. It is a software demonstration, not a certified radiation monitoring instrument.

## Roadmap

- Seeded Poisson trace generation with configurable change points.
- Monte Carlo evaluation of false alarms and detection delay.
- Threshold calibration under explicitly documented assumptions.
- Buffered-input comparison and reproducible benchmarks.
- CSV export of scores and alarm results.
- Automated CLI integration tests.
- Live measurement input through a separate adapter.

## Author

Nickolas Adrian Papamichail| `CountView` | Validates the file and decodes counts |
| `DetectionConfig` | Validates rates and alarm threshold |
| `PoissonCusum` | Processes counts and records the first alarm |

`CountView` remains usable after the original `MappedFile` is destroyed
because its `MemoryView` retains shared ownership of the mapping.

Shared ownership protects mapping lifetime. It does not protect against
external modification or truncation of the input file.

## Statistical model

For a bin duration Δt and count rate r:

    K_t ~ Poisson(r × Δt)

The detector compares:

- Baseline rate: r₀
- Alternative rate: r₁ > r₀

The per-bin log-likelihood contribution is:

    ℓ_t = K_t × log(r₁ / r₀) − (r₁ − r₀) × Δt

The CUSUM update is:

    S_t = max(0, S_(t−1) + ℓ_t)

The first bin satisfying `S_t >= threshold` is recorded as the alarm.

Rates are expressed in counts/second. Bin duration is stored in the file
as integer nanoseconds.

## Requirements

- C++20 compiler
- CMake 3.20 or newer
- macOS or Linux
- Python 3 for generating the example fixture

The C++ implementation requires no third-party libraries.

## Build

Run from the repository root:

```bash
cmake -S . -B build-cmake \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_TESTING=ON

cmake --build build-cmake --parallel
```

## Run the tests

```bash
ctest --test-dir build-cmake --output-on-failure
```

The tests cover:

- Known CUSUM scores and first-alarm position.
- Zero-count observations.
- Alarm at bin zero.
- Invalid detection parameters.
- Binary-format validation.
- Little-endian decoding.
- Out-of-range count access.
- Mapping lifetime after destruction of the original file owner.

## Reproducible example

Generate a four-bin trace:

```bash
python3 scripts/create_fixture.py
```

The trace contains:

```text
Counts:       [0, 1, 2, 6]
Bin duration: 1 second
File size:    48 bytes
```

Run detection:

```bash
./build-cmake/radshift examples/four_bins.rct 1 3 4
```

Argument order:

```text
radshift <trace.rct> <baseline_rate> <alternative_rate> <threshold>
```

Expected output:

```text
Baseline rate: 1.000000 counts/s
Alternative rate: 3.000000 counts/s
Threshold: 4.000000
Bins: 4
Bin duration (ns): 1000000000
Final CUSUM score: 4.788898
First alarm bin: 3
```

Increasing the threshold prevents an alarm for this short trace:

```bash
./build-cmake/radshift examples/four_bins.rct 1 3 10
```

Expected final line:

```text
No alarm
```

## Binary format: RADCOUNT v1

All integer fields use little-endian encoding.

| Offset | Size | Field |
|---:|---:|---|
| 0 | 8 bytes | ASCII magic: `RADCOUNT` |
| 8 | 4 bytes | Version: `1` |
| 12 | 4 bytes | Header size: `32` |
| 16 | 8 bytes | Number of bins |
| 24 | 8 bytes | Bin duration in nanoseconds |
| 32 | 4 bytes per bin | Unsigned 32-bit counts |

The parser rejects unsupported headers, zero duration, zero bins, and
payload sizes inconsistent with the declared count.

## Performance design

- No complete application-owned copy of the mapped input.
- Constant detector-state memory.
- Precomputed likelihood coefficients.
- No allocation or text parsing inside the detector update.
- Summary output after processing.

Memory mapping is not assumed to outperform buffered reading.
Comparative benchmarks are planned; no throughput claims are made yet.

A whole-file mapping uses virtual address space proportional to file size.
Resident memory depends on accessed pages and operating-system behavior.

## Limitations

- Upward changes only.
- Known, fixed baseline and alternative rates.
- Fixed-duration bins.
- Poisson observation model.
- Thresholds are user-specified and have not been statistically calibrated.
- First-alarm detection; processing continues after the alarm.
- No live acquisition or growing-file support.
- No medical, dosimetry, or safety certification.

## Roadmap

- Synthetic Poisson trace generator.
- False-alarm and detection-delay evaluation.
- Buffered binary reader.
- Mapped-versus-buffered benchmarks.
- Automated command-line tests.
- Threshold calibration.
- CSV result export.

## Author

Nickolas Adrian Papamichail
