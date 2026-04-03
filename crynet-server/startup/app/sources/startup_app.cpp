#include "startup/app/includes/startup_app.h"

#include "core/base/includes/build_config.h"
#include "core/bootstrap/includes/bootstrap_config.h"
#include "core/bootstrap/includes/bootstrapper.h"

#include <iostream>
#include <sstream>

namespace crynet::startup {

bool StartupApp::initialize() noexcept {
    m_b_bootstrap_ready = false;
    m_b_initialized = true;

    const auto config = crynet::bootstrap::BootstrapConfig::from_text(default_bootstrap_config());
    if (!config.has_value()) {
        m_b_initialized = false;
        return false;
    }

    crynet::bootstrap::Bootstrapper bootstrapper;
    bootstrapper.bind_handler(
        "bootstrap",
        [](crynet::core::ServiceContext&, const crynet::core::Message&) {
            // The startup shell only needs proof that the bootstrap path can execute.
        }
    );

    if (!bootstrapper.start(config.value())) {
        m_b_initialized = false;
        return false;
    }

    m_b_bootstrap_ready = bootstrapper.dispatch_until_idle() > 0;
    return m_b_initialized;
}

void StartupApp::shutdown() noexcept {
    m_b_bootstrap_ready = false;
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

bool StartupApp::bootstrap_ready() const noexcept {
    return m_b_bootstrap_ready;
}

std::string_view StartupApp::default_bootstrap_config() noexcept {
    return R"(# minimal startup config
service bootstrap ready
timeout_ms 10
)";
}

int StartupApp::run() noexcept {
    if (!initialize()) {
        std::cerr << "[crynet] startup initialization failed" << std::endl;
        return 1;
    }

    std::cout << "[crynet] " << banner() << std::endl;
    std::cout << "[crynet] bootstrap flow is ready." << std::endl;

    shutdown();
    return 0;
}

}  // namespace crynet::startup
