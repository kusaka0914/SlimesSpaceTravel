#include "actor/Actor.h"
#include "actor/MathUtils.h"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace {

int ParseCount(const char* text)
{
    const std::string value(text);
    int count = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), count);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || count < 1 || count > 1000000) {
        throw std::runtime_error("iteration/sample count must be in 1..1000000");
    }
    return count;
}

void Measure(const char* name, int iterations, int samples, const std::function<std::uint64_t()>& operation)
{
    std::uint64_t checksum = 0;
    for (int warmup = 0; warmup < 3; ++warmup) {
        checksum += operation();
    }
    std::vector<double> measurements;
    for (int sample = 0; sample < samples; ++sample) {
        const auto start = std::chrono::steady_clock::now();
        for (int iteration = 0; iteration < iterations; ++iteration) {
            checksum += operation();
        }
        const auto elapsed = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start);
        measurements.push_back(elapsed.count() / iterations);
    }
    std::sort(measurements.begin(), measurements.end());
    const auto p95 = static_cast<std::size_t>(std::ceil(measurements.size() * 0.95)) - 1;
    std::cout << name << ',' << iterations << ',' << samples << ',' << std::setprecision(12)
              << measurements[measurements.size() / 2] << ',' << measurements[p95] << ',' << checksum << '\n';
}

} // namespace

int main(int argc, char* argv[])
{
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: game_benchmarks <stage.yaml> [iterations=30] [samples=11]\n";
        return 2;
    }
    try {
        const int iterations = argc > 2 ? ParseCount(argv[2]) : 30;
        const int samples = argc > 3 ? ParseCount(argv[3]) : 11;
        std::ifstream input(argv[1], std::ios::binary);
        if (!input) {
            throw std::runtime_error("cannot open stage YAML");
        }
        const std::string text{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
        const YAML::Node stage = YAML::Load(text);
        if (!stage.IsMap()) {
            throw std::runtime_error("stage YAML must be a mapping");
        }
        std::cout << "benchmark,iterations,samples,median_us,p95_us,checksum\n";
        Measure("stage_yaml_parse", iterations, samples, [&]() { return YAML::Load(text).size(); });
        Measure("stage_yaml_clone", iterations, samples, [&]() { return YAML::Clone(stage).size(); });
        Measure("stage_yaml_serialize", iterations, samples, [&]() { return YAML::Dump(stage).size(); });
        // 短い処理は反復を増やし、時計の分解能の影響を抑える。
        Measure("actor_construct_destroy", iterations * 1000, samples, []() {
            const auto actor = std::make_unique<Actor>(nullptr);
            return static_cast<std::uint64_t>(actor->GetRadius());
        });
        const MathUtils math;
        Measure("billboard_regular", iterations * 1000, samples, [&]() {
            const auto matrix = math.CreateBillBoard(glm::mat4(1), {2, 3, -5}, {0, 1, 0}, 2, 3);
            return static_cast<std::uint64_t>(std::abs(matrix[0].x) * 100000);
        });
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
