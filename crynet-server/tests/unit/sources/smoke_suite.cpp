#include "tests/unit/includes/smoke_suite.h"

#include "startup/app/includes/startup_app.h"

#include <iostream>

namespace crynet::tests::unit {

int SmokeSuite::run() noexcept {
    crynet::startup::StartupApp startup_app;
    if (!startup_app.initialize()) {
        std::cerr << "[unit] startup initialization failed" << std::endl;
        return 1;
    }

    if (!startup_app.is_initialized()) {
        std::cerr << "[unit] startup initialized state mismatch" << std::endl;
        return 1;
    }

    startup_app.shutdown();
    std::cout << "[unit] smoke suite passed" << std::endl;
    return 0;
}

}  // namespace crynet::tests::unit

int main() {
    return crynet::tests::unit::SmokeSuite::run();
}
