/**
 * @file main_window.h
 * @brief Main application window
 */

#pragma once

#include <QMainWindow>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QDockWidget>
#include <QTimer>
#include <QCloseEvent>
#include <memory>

#include "simulation_engine.h"
#include "peripherals/gpio_controller.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace esp32sim {

class PinoutPanel;
class Workspace;
class ComponentLibrary;
class ConsolePanel;
class LogicAnalyzer;
class Oscilloscope;
class DebugPanel;
class PropertiesPanel;
class SimulationControlBar;
class VirtualComponent;

/**
 * @class MainWindow
 * @brief Main application window with all panels and controls
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /**
     * @brief Get the simulation engine
     */
    SimulationEngine* engine() const { return engine_; }

    /**
     * @brief Get the pinout panel
     */
    PinoutPanel* pinoutPanel() const { return pinout_panel_; }

    /**
     * @brief Get the workspace
     */
    Workspace* workspace() const { return workspace_; }

    /**
     * @brief Get the console panel
     */
    ConsolePanel* consolePanel() const { return console_panel_; }

    /**
     * @brief Get the logic analyzer
     */
    LogicAnalyzer* logicAnalyzer() const { return logic_analyzer_; }

    /**
     * @brief Get the oscilloscope
     */
    Oscilloscope* oscilloscope() const { return oscilloscope_; }

    /**
     * @brief Get the debug panel
     */
    DebugPanel* debugPanel() const { return debug_panel_; }

public slots:
    /**
     * @brief Create a new project
     */
    void newProject();

    /**
     * @brief Open an existing project
     */
    void openProject();

    /**
     * @brief Save current project
     */
    void saveProject();

    /**
     * @brief Save project as
     */
    void saveProjectAs();

    /**
     * @brief Load firmware
     */
    void loadFirmware();

    /**
     * @brief Start simulation
     */
    void startSimulation();

    /**
     * @brief Pause simulation
     */
    void pauseSimulation();

    /**
     * @brief Stop simulation
     */
    void stopSimulation();

    /**
     * @brief Step one instruction
     */
    void stepSimulation();

    /**
     * @brief Reset simulation
     */
    void resetSimulation();

    /**
     * @brief Toggle breakpoint at address
     */
    void toggleBreakpoint(uint32_t address);

    /**
     * @brief Show about dialog
     */
    void showAbout();

     /**
      * @brief Show settings dialog
      */
     void showSettings();

     /**
      * @brief Select all components
      */
     void selectAll();

     /**
      * @brief Update status bar
      */
     void updateStatus();

    /**
     * @brief Handle firmware loaded event
     */
    void onFirmwareLoaded(bool success, const QString& error_msg = "");

private slots:
    /**
     * @brief Update simulation state UI
     */
    void onSimulationStateChanged(SimulationState state);

    /**
     * @brief Handle component selection
     */
    void onComponentSelected(VirtualComponent* component);

    /**
     * @brief Handle pin state change from engine
     */
    void onPinStateChanged(uint8_t pin, GPIOLevel level);

    /**
     * @brief Pin clicked on pinout panel
     */
    void onPinSelection(int pin, QPoint global_pos);

    /**
     * @brief Context menu requested on pin
     */
    void onPinContextMenu(int pin, QPoint global_pos);

    /**
     * @brief Component added to workspace
     */
    void onComponentAdded(VirtualComponent* component);

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void createMenus();
    void createToolbars();
    void createStatusBar();
    void createDockWidgets();
    void setupConnections();

    void updateUIBasedOnState();
    void loadSettings();
    void saveSettings();
    void updateRecentFilesMenu();

    Ui::MainWindow* ui_;
    SimulationEngine* engine_;

    // Panels
    PinoutPanel* pinout_panel_ = nullptr;
    Workspace* workspace_ = nullptr;
    ComponentLibrary* component_library_ = nullptr;
    ConsolePanel* console_panel_ = nullptr;
    LogicAnalyzer* logic_analyzer_ = nullptr;
    Oscilloscope* oscilloscope_ = nullptr;
    DebugPanel* debug_panel_ = nullptr;
    PropertiesPanel* properties_panel_ = nullptr;
    SimulationControlBar* control_bar_ = nullptr;

    // Menus
    QMenu* file_menu_ = nullptr;
    QMenu* edit_menu_ = nullptr;
    QMenu* view_menu_ = nullptr;
    QMenu* simulation_menu_ = nullptr;
    QMenu* debug_menu_ = nullptr;
    QMenu* help_menu_ = nullptr;

    // Actions
    QAction* action_new_ = nullptr;
    QAction* action_open_ = nullptr;
    QAction* action_save_ = nullptr;
    QAction* action_load_firmware_ = nullptr;
    QAction* action_start_ = nullptr;
    QAction* action_pause_ = nullptr;
    QAction* action_stop_ = nullptr;
    QAction* action_step_ = nullptr;
    QAction* action_reset_ = nullptr;

    // Timer for status updates
    QTimer* status_timer_ = nullptr;

    // Current project
    QString current_project_file_;
    bool project_dirty_ = false;
};

} // namespace esp32sim
