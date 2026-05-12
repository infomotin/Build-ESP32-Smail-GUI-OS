/**
 * @file main_window.cpp
 * @brief Main window implementation
 */

#include "gui/main_window.h"

#include "application.h"
#include "simulation_engine.h"
#include "gui/pinout_panel.h"
#include "gui/workspace.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QLabel>
#include <QSlider>
#include <QSettings>

#include "gui/console_panel.h"
#include "gui/logic_analyzer.h"
#include "gui/oscilloscope.h"
#include "gui/debug_panel.h"
#include "gui/properties_panel.h"
#include "gui/component_library.h"
#include "virtual_components/component_base.h"

namespace esp32sim {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui_(nullptr), engine_(nullptr) {
    LOG_INFO("Creating MainWindow");

    // Get the already initialized engine from Application
    engine_ = Application::instance()->engine();

    setupUI();
    createMenus();
    createToolbars();
    createStatusBar();
    createDockWidgets();
    setupConnections();

    // Set initial state
    updateUIBasedOnState();

    // Load settings
    loadSettings();

    // Set window properties
    setWindowTitle("ESP32 Virtual Hardware Simulator v1.0.0");
    resize(1400, 900);
    setMinimumSize(1000, 700);

    LOG_INFO("MainWindow created");
}

MainWindow::~MainWindow() {
    saveSettings();
    // Children will be auto-deleted
}

void MainWindow::setupUI() {
    // Central widget: splitter with workspace and pinout
    QSplitter* central_splitter = new QSplitter(Qt::Horizontal, this);

    // Workspace (left side)
    workspace_ = new Workspace(engine_, this);
    central_splitter->addWidget(workspace_);

    // Pinout panel (right side)
    pinout_panel_ = new PinoutPanel(engine_, this);
    central_splitter->addWidget(pinout_panel_);
    central_splitter->setStretchFactor(0, 3);
    central_splitter->setStretchFactor(1, 1);

    setCentralWidget(central_splitter);
}

void MainWindow::createMenus() {
    // File menu
    file_menu_ = menuBar()->addMenu("&File");

    action_new_ = file_menu_->addAction("&New Project", this, &MainWindow::newProject, QKeySequence::New);
    action_open_ = file_menu_->addAction("&Open Project...", this, &MainWindow::openProject, QKeySequence::Open);
    action_save_ = file_menu_->addAction("&Save Project", this, &MainWindow::saveProject, QKeySequence::Save);
    file_menu_->addSeparator();
    action_load_firmware_ = file_menu_->addAction("Load &Firmware...", this, &MainWindow::loadFirmware);
    file_menu_->addSeparator();
    file_menu_->addAction("E&xit", this, &QWidget::close, QKeySequence::Quit);

    // Edit menu
    edit_menu_ = menuBar()->addMenu("&Edit");
    edit_menu_->addAction("&Undo", this, nullptr, QKeySequence::Undo);
    edit_menu_->addAction("&Redo", this, nullptr, QKeySequence::Redo);
    edit_menu_->addSeparator();
    edit_menu_->addAction("Cu&t", this, nullptr, QKeySequence::Cut);
    edit_menu_->addAction("&Copy", this, nullptr, QKeySequence::Copy);
    edit_menu_->addAction("&Paste", this, nullptr, QKeySequence::Paste);
    edit_menu_->addSeparator();
    edit_menu_->addAction("Select &All", this, &MainWindow::selectAll, QKeySequence::SelectAll);

    // View menu
    view_menu_ = menuBar()->addMenu("&View");
    view_menu_->addAction("Component &Library", this, [](){ /* toggle */ });
    view_menu_->addAction("&Console", this, [](){ /* toggle */ });
    view_menu_->addAction("&Logic Analyzer", this, [](){ /* toggle */ });
    view_menu_->addAction("&Oscilloscope", this, [](){ /* toggle */ });
    view_menu_->addAction("&Debug Panel", this, [](){ /* toggle */ });
    view_menu_->addSeparator();
    view_menu_->addAction("&Full Screen", this, [](){ /* toggle */ });

    // Simulation menu
    simulation_menu_ = menuBar()->addMenu("&Simulation");
    action_start_ = simulation_menu_->addAction("&Start", this, &MainWindow::startSimulation, QKeySequence(Qt::Key_F5));
    action_pause_ = simulation_menu_->addAction("&Pause", this, &MainWindow::pauseSimulation, QKeySequence(Qt::Key_F6));
    action_stop_ = simulation_menu_->addAction("&Stop", this, &MainWindow::stopSimulation, QKeySequence(Qt::Key_F7));
    action_step_ = simulation_menu_->addAction("&Step", this, &MainWindow::stepSimulation, QKeySequence(Qt::Key_F8));
    action_reset_ = simulation_menu_->addAction("&Reset", this, &MainWindow::resetSimulation, QKeySequence(Qt::Key_F9));
    simulation_menu_->addSeparator();
    simulation_menu_->addAction("&Settings...", this, &MainWindow::showSettings);

    // Debug menu
    debug_menu_ = menuBar()->addMenu("&Debug");
    debug_menu_->addAction("Toggle &Breakpoint", this, [](){ /* */ }, QKeySequence(Qt::Key_F9));
    debug_menu_->addAction("&Run to Cursor", this, [](){ /* */ });

    // Help menu
    help_menu_ = menuBar()->addMenu("&Help");
    help_menu_->addAction("&About", this, &MainWindow::showAbout);
    help_menu_->addAction("&Documentation", this, [](){ /* open browser */ });
}

void MainWindow::createToolbars() {
    // Simulation toolbar
    QToolBar* sim_toolbar = addToolBar("Simulation");
    sim_toolbar->addAction(action_start_);
    sim_toolbar->addAction(action_pause_);
    sim_toolbar->addAction(action_stop_);
    sim_toolbar->addAction(action_step_);
    sim_toolbar->addAction(action_reset_);
    sim_toolbar->addSeparator();

    // Speed control
    QLabel* speed_label = new QLabel("Speed: ");
    sim_toolbar->addWidget(speed_label);
    QSlider* speed_slider = new QSlider(Qt::Horizontal);
    speed_slider->setRange(10, 1000);  // 0.1x to 10x
    speed_slider->setValue(100);
    sim_toolbar->addWidget(speed_slider);
}

void MainWindow::createStatusBar() {
    statusBar()->setSizeGripEnabled(true);

    // Permanent widgets
    statusBar()->addPermanentWidget(new QLabel("Ready"));

    // Update timer
    QTimer* status_timer = new QTimer(this);
    connect(status_timer, &QTimer::timeout, this, &MainWindow::updateStatus);
    status_timer->start(100);  // 10 Hz
}

void MainWindow::createDockWidgets() {
    // Console dock (bottom)
    QDockWidget* console_dock = new QDockWidget("Console", this);
    console_panel_ = new ConsolePanel(engine_, this);
    console_dock->setWidget(console_panel_);
    addDockWidget(Qt::BottomDockWidgetArea, console_dock);

    // Logic Analyzer dock (right)
    QDockWidget* la_dock = new QDockWidget("Logic Analyzer", this);
    logic_analyzer_ = new LogicAnalyzer(engine_, this);
    la_dock->setWidget(logic_analyzer_);
    addDockWidget(Qt::RightDockWidgetArea, la_dock);

    // Oscilloscope dock
    QDockWidget* scope_dock = new QDockWidget("Oscilloscope", this);
    oscilloscope_ = new Oscilloscope(engine_, this);
    scope_dock->setWidget(oscilloscope_);
    tabifyDockWidget(la_dock, scope_dock);

    // Debug Panel dock
    QDockWidget* debug_dock = new QDockWidget("Debug", this);
    debug_panel_ = new DebugPanel(engine_, this);
    debug_dock->setWidget(debug_panel_);
    addDockWidget(Qt::RightDockWidgetArea, debug_dock);

    // Properties panel (right side, tabbed)
    QDockWidget* props_dock = new QDockWidget("Properties", this);
    properties_panel_ = new PropertiesPanel(engine_, this);
    props_dock->setWidget(properties_panel_);
    addDockWidget(Qt::RightDockWidgetArea, props_dock);
    tabifyDockWidget(debug_dock, props_dock);

    // Component library (left)
    QDockWidget* lib_dock = new QDockWidget("Components", this);
    component_library_ = new ComponentLibrary(this);
    lib_dock->setWidget(component_library_);
    addDockWidget(Qt::LeftDockWidgetArea, lib_dock);
}

void MainWindow::setupConnections() {
    // Engine signals
    connect(engine_, &SimulationEngine::stateChanged,
            this, &MainWindow::onSimulationStateChanged);
    connect(engine_, &SimulationEngine::firmwareLoaded,
            this, [this](const FirmwareInfo& /*info*/, bool success) {
                onFirmwareLoaded(success, success ? QString() : QString("Firmware load failed"));
            });
    connect(engine_, &SimulationEngine::statisticsUpdated,
            this, [](const SimulationStatistics& stats) {
                // Update status bar with stats
            });

    // Pinout panel
    connect(pinout_panel_, &PinoutPanel::pinClicked,
            this, &MainWindow::onPinSelection);
    connect(pinout_panel_, &PinoutPanel::pinContextMenuRequested,
            this, &MainWindow::onPinContextMenu);

    // Workspace
    connect(workspace_, &Workspace::componentAdded,
            this, &MainWindow::onComponentAdded);
    connect(workspace_, &Workspace::selectionChanged,
            this, &MainWindow::onComponentSelected);

    // Console
    connect(console_panel_, &ConsolePanel::clearConsole,
            this, [](){ /* clear log */ });
}

void MainWindow::newProject() {
    // TODO: Implement new project
    QMessageBox::information(this, "New Project", "New project functionality not yet implemented");
}

void MainWindow::openProject() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Open Project", "", "ESP32 Simulator Projects (*.esp32sim);;All Files (*)");

    if (!filename.isEmpty()) {
        // TODO: Load project
    }
}

void MainWindow::saveProject() {
    if (current_project_file_.isEmpty()) {
        saveProjectAs();
    } else {
        // Save to file
    }
}

void MainWindow::saveProjectAs() {
    QString filename = QFileDialog::getSaveFileName(
        this, "Save Project As", "", "ESP32 Simulator Projects (*.esp32sim);;All Files (*)");

    if (!filename.isEmpty()) {
        current_project_file_ = filename;
        saveProject();
    }
}

void MainWindow::loadFirmware() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Load Firmware", "", "ELF Files (*.elf);;All Files (*)");

    if (!filename.isEmpty()) {
        engine_->loadFirmware(filename.toStdString());
    }
}

void MainWindow::startSimulation() {
    engine_->start();
}

void MainWindow::pauseSimulation() {
    engine_->pause();
}

void MainWindow::stopSimulation() {
    engine_->stop();
}

void MainWindow::stepSimulation() {
    engine_->step();
}

void MainWindow::resetSimulation() {
    engine_->reset();
}

void MainWindow::showAbout() {
    QMessageBox::about(this, "About ESP32 Simulator",
                       "<h3>ESP32 Virtual Hardware Simulator</h3>"
                       "<p>Version 1.0.0</p>"
                       "<p>A comprehensive simulation environment for ESP32 firmware development.</p>"
                       "<p>&copy; 2025 ESP32 Simulator Project</p>");
}

void MainWindow::showSettings() {
    // Show settings dialog
    QMessageBox::information(this, "Settings", "Settings dialog not yet implemented");
}

void MainWindow::updateStatus() {
    if (!engine_) return;
    QString state_str;
    switch (engine_->state()) {
        case SimulationState::STOPPED: state_str = "Stopped"; break;
        case SimulationState::RUNNING: state_str = "Running"; break;
        case SimulationState::PAUSED: state_str = "Paused"; break;
        case SimulationState::STEPPING: state_str = "Stepping"; break;
        case SimulationState::ERROR: state_str = "Error"; break;
        default: state_str = "Unknown";
    }
    QString msg = QString("State: %1 | Time: %2 us")
                      .arg(state_str)
                      .arg(engine_->simulationTime());
    statusBar()->showMessage(msg);
}

void MainWindow::onFirmwareLoaded(bool success, const QString& error_msg) {
    if (success) {
        const auto& info = engine_->firmwareInfo();
        statusBar()->showMessage(
            QString("Loaded: %1 (Entry: 0x%2, %3 symbols)")
                .arg(QString::fromStdString(info.filename))
                .arg(info.entry_point, 8, 16, QChar('0'))
                .arg(info.symbols.size()));
    } else {
        QMessageBox::critical(this, "Load Error", error_msg);
    }
}

void MainWindow::onSimulationStateChanged(SimulationState state) {
    updateUIBasedOnState();

    QString state_text;
    switch (state) {
        case SimulationState::STOPPED: state_text = "Stopped"; break;
        case SimulationState::RUNNING: state_text = "Running"; break;
        case SimulationState::PAUSED:  state_text = "Paused"; break;
        case SimulationState::STEPPING: state_text = "Stepping"; break;
        case SimulationState::ERROR:   state_text = "Error"; break;
    }
    statusBar()->showMessage(state_text, 2000);
}

void MainWindow::updateUIBasedOnState() {
    bool running = engine_->isRunning();

    action_start_->setEnabled(!running);
    action_pause_->setEnabled(running);
    action_stop_->setEnabled(running);
    action_step_->setEnabled(!running);
    action_reset_->setEnabled(!running);
}

void MainWindow::onPinSelection(int pin, QPoint /*global_pos*/) {
    // Select pin in workspace or show info
    QString msg = QString("GPIO%1 selected").arg(pin);
    statusBar()->showMessage(msg);
}

void MainWindow::onPinContextMenu(int pin, QPoint global_pos) {
    pinout_panel_->showPinContextMenu(pin, global_pos);
}

void MainWindow::onComponentAdded(VirtualComponent* component) {
    LOG_DEBUG("Component added: {}", component->name());
}

void MainWindow::onComponentSelected(VirtualComponent* component) {
    if (component) {
        properties_panel_->setComponent(component);
    }
}

void MainWindow::loadSettings() {
    // Load window geometry, recent files, etc.
    QSettings settings("ESP32Sim", "VirtualHardwareSimulator");
    restoreGeometry(settings.value("mainwindow/geometry").toByteArray());
    restoreState(settings.value("mainwindow/state").toByteArray());
}

void MainWindow::saveSettings() {
    QSettings settings("ESP32Sim", "VirtualHardwareSimulator");
    settings.setValue("mainwindow/geometry", saveGeometry());
    settings.setValue("mainwindow/state", saveState());
}

void MainWindow::selectAll() {
    workspace_->selectAll();
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Confirm if simulation running
    if (engine_->isRunning()) {
        auto reply = QMessageBox::question(this, "Confirm Exit",
                                           "Simulation is running. Exit anyway?",
                                           QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::No) {
            event->ignore();
            return;
        }
    }

    QMainWindow::closeEvent(event);
}

} // namespace esp32sim
