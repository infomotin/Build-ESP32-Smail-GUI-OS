/**
 * @file application.cpp
 * @brief Application class implementation
 */

#include "application.h"
#include "gui/main_window.h"
#include "utils/logger.h"

#include <QSettings>

namespace esp32sim {

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv) {
}

Application::~Application() {
    shutdown();
}

bool Application::initialize() {
    LOG_INFO("Initializing Application");

    // Create simulation engine
    engine_ = std::make_unique<SimulationEngine>();
    if (!engine_->initialize()) {
        LOG_ERROR("Failed to initialize simulation engine");
        return false;
    }

    // Create main window
    main_window_ = new MainWindow();
    if (!main_window_) {
        LOG_ERROR("Failed to create main window");
        return false;
    }

    // Connect engine signals
    connect(engine_.get(), &SimulationEngine::firmwareLoaded,
            this, &Application::firmwareLoaded);
    connect(engine_.get(), &SimulationEngine::stateChanged,
            this, &Application::simulationStateChanged);

    // Load settings
    loadRecentFiles();

    LOG_INFO("Application initialized");
    return true;
}

void Application::shutdown() {
    LOG_INFO("Shutting down Application");

    if (main_window_) {
        main_window_->close();
        delete main_window_;
        main_window_ = nullptr;
    }

    if (engine_) {
        engine_->shutdown();
        engine_.reset();
    }

    LOG_INFO("Application shutdown complete");
}

bool Application::loadFirmware(const QString& filename) {
    if (!engine_) return false;

    std::string std_filename = filename.toStdString();
    engine_->loadFirmware(std_filename);

    if (engine_->isFirmwareLoaded()) {
        addRecentFile(filename);
        return true;
    }

    return false;
}

void Application::addRecentFile(const QString& filename) {
    // Remove if already exists
    recent_files_.removeAll(filename);

    // Add to front
    recent_files_.push_front(filename);

    // Limit to 10
    while (recent_files_.size() > 10) {
        recent_files_.pop_back();
    }

    saveRecentFiles();
}

void Application::loadRecentFiles() {
    QSettings settings("ESP32Sim", "VirtualHardwareSimulator");
    recent_files_ = settings.value("recentFiles").toStringList();
}

void Application::saveRecentFiles() {
    QSettings settings("ESP32Sim", "VirtualHardwareSimulator");
    settings.setValue("recentFiles", recent_files_);
}

void Application::onAboutToQuit() {
    LOG_INFO("Application about to quit");
    shutdown();
}

void Application::updateStatus() {
    // No-op for now
}

} // namespace esp32sim
