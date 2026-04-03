#include "startup/app/includes/startup_app.h"

#include "core/base/includes/build_config.h"
#include "core/bootstrap/includes/bootstrap_config.h"
#include "core/bootstrap/includes/bootstrapper.h"
#include "core/logger/includes/logger.h"
#include "core/monitor/includes/monitor_runner.h"
#include "core/metrics/includes/metrics_system.h"
#include "core/scheduler/includes/worker_monitor.h"
#include "core/scheduler/includes/worker_runner.h"

#include <cstdlib>
#include <iostream>
#include <sstream>

namespace crynet::startup {

bool StartupApp::initialize() noexcept {
    auto& logger = crynet::core::Logger::default_logger();
    m_b_bootstrap_ready = false;
    m_b_initialized = true;

    std::optional<crynet::bootstrap::BootstrapConfig> config;
    if (const auto* config_path = std::getenv("CRYNET_BOOTSTRAP_CONFIG"); config_path != nullptr && config_path[0] != '\0') {
        config = crynet::bootstrap::BootstrapConfig::from_file(config_path);
    } else {
        config = crynet::bootstrap::BootstrapConfig::from_text(default_bootstrap_config());
    }

    if (!config.has_value()) {
        logger.log(crynet::core::LogLevel::Error, "startup", "bootstrap config parse failed");
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
        logger.log(crynet::core::LogLevel::Error, "startup", "bootstrap service registration failed");
        m_b_initialized = false;
        return false;
    }

    std::size_t dispatched_messages = 0;
    if (config->monitor().enabled) {
        crynet::core::WorkerMonitor worker_monitor{1};
        worker_monitor.set_logger_alert_enabled(config->monitor().logger_alert_enabled);

        crynet::core::WorkerRunner worker_runner{bootstrapper.system(), 0};
        worker_runner.add_plugin(worker_monitor);
        dispatched_messages = worker_runner.run_until_idle();

        crynet::monitor::MonitorRunner monitor_runner{worker_monitor};
        static_cast<void>(monitor_runner.run_checks(config->monitor().check_cycles));
    } else {
        dispatched_messages = bootstrapper.dispatch_until_idle();
    }

    m_b_bootstrap_ready = dispatched_messages > 0;
    const auto metrics = bootstrapper.system().metrics();
    if (m_b_bootstrap_ready) {
        std::ostringstream stream;
        stream << "bootstrap flow is ready services=" << metrics.registered_services
               << " queued=" << metrics.queued_messages
               << " dispatched=" << metrics.dispatched_messages;
        logger.log(crynet::core::LogLevel::Info, "startup", stream.str());
    } else {
        logger.log(crynet::core::LogLevel::Warn, "startup", "bootstrap flow completed without work");
    }

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
monitor.enabled false
)";
}

int StartupApp::run() noexcept {
    auto& logger = crynet::core::Logger::default_logger();
    if (!initialize()) {
        logger.log(crynet::core::LogLevel::Error, "startup", "startup initialization failed");
        return 1;
    }

    logger.log(crynet::core::LogLevel::Info, "startup", banner());
    logger.log(crynet::core::LogLevel::Info, "startup", "startup flow completed");

    shutdown();
    return 0;
}

}  // namespace crynet::startup
