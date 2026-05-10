/**
 * @file application.h
 * @brief Main application class for ESP32 Virtual Simulator
 */

#pragma once

#include <QApplication>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QTimer>
#include <memory>
#include <string>

#include "simulation_engine.h"

namespace esp32sim {

class MainWindow;

/**
 * @class Application
 * @brief Main application class managing the simulator lifecycle
 */
class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application();

    /**
     * @brief Get the singleton instance
     */
    static Application* instance() { return static_cast<Application*>(QApplication::instance()); }

    /**
     * @brief Get the simulation engine
     */
    SimulationEngine* engine() const { return engine_.get(); }

    /**
     * @brief Get the main window
     */
    MainWindow* mainWindow() const { return main_window_; }

    /**
     * @brief Initialize the application
     */
    bool initialize();

    /**
     * @brief Shutdown the application
     */
    void shutdown();

    /**
     * @brief Load firmware from file
     * @param filename Path to ELF file or Arduino sketch
     * @return true on success
     */
    bool loadFirmware(const QString& filename);

    /**
     * @brief Get recent files list
     */
    const QStringList& recentFiles() const { return recent_files_; }

    /**
     * @brief Add to recent files
     */
    void addRecentFile(const QString& filename);

signals:
    /**
     * @brief Emitted when firmware is loaded
     */
    void firmwareLoaded(bool success, const QString& error_msg = "");

    /**
     * @brief Emitted when simulation state changes
     */
    void simulationStateChanged(SimulationState state);

private slots:
    /**
     * @brief Handle application about to quit
     */
    void onAboutToQuit();

    /**
     * @brief Update status bar periodically
     */
    void updateStatus();

private:
    std::unique_ptr<SimulationEngine> engine_;
    MainWindow* main_window_ = nullptr;
    QStringList recent_files_;
    QTimer* status_timer_ = nullptr;

    /**
     * @brief Load recent files configuration
     */
    void loadRecentFiles();

    /**
     * @brief Save recent files configuration
     */
    void saveRecentFiles();
};

} // namespace esp32sim
