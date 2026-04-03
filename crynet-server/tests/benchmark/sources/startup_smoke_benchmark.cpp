#include "tests/benchmark/includes/startup_smoke_benchmark.h"

#include "startup/app/includes/startup_app.h"

#include <chrono>
#include <cstdint>
#include <iostream>

namespace crynet::tests::benchmark {

int StartupSmokeBenchmark::run() noexcept {
    constexpr std::uint32_t k_iterations = 200000;

    auto begin = std::chrono::steady_clock::now();
    for (std::uint32_t index = 0; index < k_iterations; ++index) {
        crynet::startup::StartupApp startup_app;
        static_cast<void>(startup_app.initialize());
        startup_app.shutdown();
    }
    auto end = std::chrono::steady_clock::now();

    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    std::cout << "[benchmark] iterations=" << k_iterations << ", elapsed_us=" << elapsed << std::endl;
    return 0;
}

}  // namespace crynet::tests::benchmark

int main() {
    return crynet::tests::benchmark::StartupSmokeBenchmark::run();
}
