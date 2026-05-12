/**
 * @file main.cpp
 * @brief Main entry point for ESP32 Virtual Simulator
 */

#include "application.h"
#include "gui/main_window.h"
#include "utils/logger.h"

#include <QApplication>
#include <QMessageBox>

#ifdef _WIN32
#include <windows.h>
#undef ERROR
#undef min
#undef max
#endif

using namespace esp32sim;

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("ESP32 Virtual Hardware Simulator");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("ESP32Sim");
    QApplication::setApplicationDisplayName("ESP32 Simulator");

    // Initialize logging
    Logger::instance().setLevel(LogLevel::DEBUG);
    Logger::instance().addConsoleSink();

    LOG_INFO("=== ESP32 Virtual Hardware Simulator v{} ===",
             QApplication::applicationVersion().toStdString());

    // Create application
    esp32sim::Application* app_obj = new esp32sim::Application(argc, argv);

    if (!app_obj->initialize()) {
        LOG_ERROR("Failed to initialize application");
        QMessageBox::critical(nullptr, "Initialization Error",
                              "Failed to initialize the simulator. Please check the logs.");
        delete app_obj;
        return 1;
    }

    // Show main window
    app_obj->mainWindow()->show();

    LOG_INFO("Application started successfully");

    int result = app.exec();

    LOG_INFO("Application exiting with code {}", result);
    delete app_obj;

    return result;
}
