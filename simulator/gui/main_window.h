#pragma once

#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QSplitter>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QWidget>
#include <QtWidgets/QLabel>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtCore/QTimer>
#include <QtCore/QThread>

#include "../core/iss/xtensa_iss.h"
#include "../core/memory/memory_model.h"
#include "../core/scheduler/event_scheduler.h"
#include "../core/elf_loader/elf_loader.h"
#include "../peripherals/gpio/gpio_model.h"

// Forward declarations
class PinoutPanel;
class WorkspaceWidget;
class LogicAnalyzerWidget;
class SimulationControlWidget;
class StatusWidget;

// Main application window
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu actions
    void loadFirmware();
    void loadArduinoSketch();
    void saveProject();
    void loadProject();
    void exitApplication();
    
    // Simulation menu actions
    void startSimulation();
    void pauseSimulation();
    void stopSimulation();
    void resetSimulation();
    void stepSimulation();
    
    // View menu actions
    void showPinoutPanel();
    void showWorkspace();
    void showLogicAnalyzer();
    void showStatusPanel();
    
    // Tools menu actions
    void openSettings();
    void aboutSimulator();
    
    // Simulation events
    void onSimulationUpdate();
    void onSimulationStateChanged();
    void onGpioEvent(const class GpioEvent& event);

private:
    // UI components
    void createMenus();
    void createToolbars();
    void createStatusBar();
    void createCentralWidget();
    void setupConnections();
    
    // Core simulation components
    std::unique_ptr<XtensaISS> iss;
    std::unique_ptr<MemoryModel> memory;
    std::unique_ptr<EventScheduler> scheduler;
    std::unique_ptr<ElfLoader> elfLoader;
    std::unique_ptr<GpioModel> gpioModel;
    
    // UI widgets
    QSplitter* mainSplitter;
    QSplitter* leftSplitter;
    PinoutPanel* pinoutPanel;
    WorkspaceWidget* workspace;
    LogicAnalyzerWidget* logicAnalyzer;
    SimulationControlWidget* simControl;
    StatusWidget* statusWidget;
    
    // Status bar items
    QLabel* statusLabel;
    QLabel* timeLabel;
    QLabel* speedLabel;
    QProgressBar* progressBar;
    
    // Simulation state
    QTimer* updateTimer;
    bool simulationRunning;
    bool simulationPaused;
    uint64_t simulationTime;
    double simulationSpeed;
    
    // Firmware information
    QString currentFirmwarePath;
    QString currentProjectPath;
    
    // Helper methods
    void updateSimulationDisplay();
    void updateStatusBar();
    void enableSimulationControls(bool running);
    bool loadFirmwareFile(const QString& filePath);
    void resetSimulationState();
    void connectGpioCallbacks();
};
