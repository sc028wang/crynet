#include "startup/app/includes/startup_app.h"

#include "core/base/includes/build_config.h"

#include <iostream>
#include <sstream>

namespace crynet::startup {

bool StartupApp::initialize() noexcept {
    m_b_initialized = true;
    return m_b_initialized;
}

void StartupApp::shutdown() noexcept {
    m_b_initialized = false;
}

bool StartupApp::is_initialized() const noexcept {
    return m_b_initialized;
}

std::string StartupApp::banner() const {
    std::ostringstream stream;
    stream << crynet::base::BuildConfig::project_name() << " v"
           << crynet::base::BuildConfig::version() << " ("
           << crynet::base::BuildConfig::platform() << ")";
    return stream.str();
}

int StartupApp::run() noexcept {
    if (!initialize()) {
        std::cerr << "[crynet] startup initialization failed" << std::endl;
        return 1;
    }

    std::cout << "[crynet] " << banner() << std::endl;
    std::cout << "[crynet] T0 startup scaffold is ready." << std::endl;

    shutdown();
    return 0;
}

}  // namespace crynet::startup
