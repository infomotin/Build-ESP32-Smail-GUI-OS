#include "main_window.h"
#include "pinout_panel.h"
#include "workspace_widget.h"
#include "logic_analyzer_widget.h"
#include "simulation_control_widget.h"
#include "status_widget.h"

#include <QApplication>
#include <QAction>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QDir>
#include <QStandardPaths>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , mainSplitter(nullptr)
    , leftSplitter(nullptr)
    , pinoutPanel(nullptr)
    , workspace(nullptr)
    , logicAnalyzer(nullptr)
    , simControl(nullptr)
    , statusWidget(nullptr)
    , statusLabel(nullptr)
    , timeLabel(nullptr)
    , speedLabel(nullptr)
    , progressBar(nullptr)
    , updateTimer(new QTimer(this))
    , simulationRunning(false)
    , simulationPaused(false)
    , simulationTime(0)
    , simulationSpeed(1.0)
{
    setWindowTitle("ESP32 Virtual Hardware Simulation Environment");
    setMinimumSize(1200, 800);
    
    // Initialize core components
    iss = std::make_unique<XtensaISS>(nullptr, nullptr);
    memory = std::make_unique<MemoryModel>();
    scheduler = std::make_unique<EventScheduler>();
    elfLoader = std::make_unique<ElfLoader>();
    gpioModel = std::make_unique<GpioModel>();
    
    // Set up ISS with memory and scheduler
    iss = std::make_unique<XtensaISS>(memory.get(), scheduler.get());
    
    // Create UI
    createMenus();
    createToolbars();
    createStatusBar();
    createCentralWidget();
    setupConnections();
    
    // Set up simulation timer
    updateTimer->setInterval(50); // 20 FPS update rate
    connect(updateTimer, &QTimer::timeout, this, &MainWindow::onSimulationUpdate);
    
    // Connect GPIO callbacks
    connectGpioCallbacks();
    
    // Reset simulation state
    resetSimulationState();
    
    // Enable/disable controls based on initial state
    enableSimulationControls(false);
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event) {
    // Stop simulation if running
    if (simulationRunning) {
        stopSimulation();
    }
    
    // Save any project state
    // TODO: Implement project saving
    
    event->accept();
}

void MainWindow::createMenus() {
    // File menu
    QMenu* fileMenu = menuBar()->addMenu("&File");
    
    QAction* loadFirmwareAction = fileMenu->addAction("&Load Firmware...");
    loadFirmwareAction->setShortcut(QKeySequence::Open);
    loadFirmwareAction->setStatusTip("Load ESP32 ELF firmware binary");
    connect(loadFirmwareAction, &QAction::triggered, this, &MainWindow::loadFirmware);
    
    QAction* loadSketchAction = fileMenu->addAction("Load &Arduino Sketch...");
    loadSketchAction->setStatusTip("Load and compile Arduino sketch");
    connect(loadSketchAction, &QAction::triggered, this, &MainWindow::loadArduinoSketch);
    
    fileMenu->addSeparator();
    
    QAction* saveProjectAction = fileMenu->addAction("&Save Project");
    saveProjectAction->setShortcut(QKeySequence::Save);
    saveProjectAction->setStatusTip("Save current simulation project");
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);
    
    QAction* loadProjectAction = fileMenu->addAction("&Load Project...");
    loadProjectAction->setShortcut(QKeySequence::Open);
    loadProjectAction->setStatusTip("Load simulation project");
    connect(loadProjectAction, &QAction::triggered, this, &MainWindow::loadProject);
    
    fileMenu->addSeparator();
    
    QAction* exitAction = fileMenu->addAction("E&xit");
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Exit the simulator");
    connect(exitAction, &QAction::triggered, this, &MainWindow::exitApplication);
    
    // Simulation menu
    QMenu* simMenu = menuBar()->addMenu("&Simulation");
    
    QAction* startAction = simMenu->addAction("&Start");
    startAction->setShortcut(QKeySequence("F5"));
    startAction->setStatusTip("Start simulation");
    connect(startAction, &QAction::triggered, this, &MainWindow::startSimulation);
    
    QAction* pauseAction = simMenu->addAction("&Pause");
    pauseAction->setShortcut(QKeySequence("F6"));
    pauseAction->setStatusTip("Pause simulation");
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pauseSimulation);
    
    QAction* stopAction = simMenu->addAction("&Stop");
    stopAction->setShortcut(QKeySequence("F7"));
    stopAction->setStatusTip("Stop simulation");
    connect(stopAction, &QAction::triggered, this, &MainWindow::stopSimulation);
    
    QAction* resetAction = simMenu->addAction("&Reset");
    resetAction->setShortcut(QKeySequence("F8"));
    resetAction->setStatusTip("Reset simulation");
    connect(resetAction, &QAction::triggered, this, &MainWindow::resetSimulation);
    
    QAction* stepAction = simMenu->addAction("Step &Over");
    stepAction->setShortcut(QKeySequence("F10"));
    stepAction->setStatusTip("Single step simulation");
    connect(stepAction, &QAction::triggered, this, &MainWindow::stepSimulation);
    
    // View menu
    QMenu* viewMenu = menuBar()->addMenu("&View");
    
    QAction* showPinoutAction = viewMenu->addAction("&Pinout Panel");
    showPinoutAction->setCheckable(true);
    showPinoutAction->setChecked(true);
    showPinoutAction->setStatusTip("Show/hide ESP32 pinout panel");
    connect(showPinoutAction, &QAction::triggered, this, &MainWindow::showPinoutPanel);
    
    QAction* showWorkspaceAction = viewMenu->addAction("&Workspace");
    showWorkspaceAction->setCheckable(true);
    showWorkspaceAction->setChecked(true);
    showWorkspaceAction->setStatusTip("Show/hide component workspace");
    connect(showWorkspaceAction, &QAction::triggered, this, &MainWindow::showWorkspace);
    
    QAction* showLogicAnalyzerAction = viewMenu->addAction("&Logic Analyzer");
    showLogicAnalyzerAction->setCheckable(true);
    showLogicAnalyzerAction->setChecked(true);
    showLogicAnalyzerAction->setStatusTip("Show/hide logic analyzer");
    connect(showLogicAnalyzerAction, &QAction::triggered, this, &MainWindow::showLogicAnalyzer);
    
    QAction* showStatusAction = viewMenu->addAction("&Status Panel");
    showStatusAction->setCheckable(true);
    showStatusAction->setChecked(true);
    showStatusAction->setStatusTip("Show/hide status panel");
    connect(showStatusAction, &QAction::triggered, this, &MainWindow::showStatusPanel);
    
    // Tools menu
    QMenu* toolsMenu = menuBar()->addMenu("&Tools");
    
    QAction* settingsAction = toolsMenu->addAction("&Settings...");
    settingsAction->setStatusTip("Open simulator settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::openSettings);
    
    // Help menu
    QMenu* helpMenu = menuBar()->addMenu("&Help");
    
    QAction* aboutAction = helpMenu->addAction("&About");
    aboutAction->setStatusTip("About ESP32 Simulator");
    connect(aboutAction, &QAction::triggered, this, &MainWindow::aboutSimulator);
}

void MainWindow::createToolbars() {
    QToolBar* mainToolBar = addToolBar("Main");
    mainToolBar->setMovable(false);
    
    // Simulation controls
    QAction* startAction = mainToolBar->addAction("Start");
    startAction->setStatusTip("Start simulation");
    connect(startAction, &QAction::triggered, this, &MainWindow::startSimulation);
    
    QAction* pauseAction = mainToolBar->addAction("Pause");
    pauseAction->setStatusTip("Pause simulation");
    connect(pauseAction, &QAction::triggered, this, &MainWindow::pauseSimulation);
    
    QAction* stopAction = mainToolBar->addAction("Stop");
    stopAction->setStatusTip("Stop simulation");
    connect(stopAction, &QAction::triggered, this, &MainWindow::stopSimulation);
    
    QAction* resetAction = mainToolBar->addAction("Reset");
    resetAction->setStatusTip("Reset simulation");
    connect(resetAction, &QAction::triggered, this, &MainWindow::resetSimulation);
    
    mainToolBar->addSeparator();
    
    // File operations
    QAction* loadAction = mainToolBar->addAction("Load Firmware");
    loadAction->setStatusTip("Load ESP32 firmware");
    connect(loadAction, &QAction::triggered, this, &MainWindow::loadFirmware);
}

void MainWindow::createStatusBar() {
    statusLabel = new QLabel("Ready");
    timeLabel = new QLabel("Time: 0.000s");
    speedLabel = new QLabel("Speed: 1.0x");
    progressBar = new QProgressBar();
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setVisible(false);
    
    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(timeLabel);
    statusBar()->addPermanentWidget(speedLabel);
    statusBar()->addPermanentWidget(progressBar);
}

void MainWindow::createCentralWidget() {
    // Create main splitter
    mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(mainSplitter);
    
    // Create left splitter (pinout + workspace)
    leftSplitter = new QSplitter(Qt::Vertical);
    
    // Create panels
    pinoutPanel = new PinoutPanel(gpioModel.get(), this);
    workspace = new WorkspaceWidget(gpioModel.get(), this);
    logicAnalyzer = new LogicAnalyzerWidget(gpioModel.get(), this);
    simControl = new SimulationControlWidget(this);
    statusWidget = new StatusWidget(this);
    
    // Add widgets to left splitter
    leftSplitter->addWidget(pinoutPanel);
    leftSplitter->addWidget(workspace);
    leftSplitter->setSizes({300, 400});
    
    // Create right container
    QWidget* rightContainer = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightContainer);
    
    rightLayout->addWidget(simControl);
    rightLayout->addWidget(logicAnalyzer);
    rightLayout->addWidget(statusWidget);
    rightLayout->setStretch(1, 1); // Logic analyzer gets most space
    
    // Add to main splitter
    mainSplitter->addWidget(leftSplitter);
    mainSplitter->addWidget(rightContainer);
    mainSplitter->setSizes({700, 500});
}

void MainWindow::setupConnections() {
    // Connect simulation control signals
    connect(simControl, &SimulationControlWidget::startRequested, this, &MainWindow::startSimulation);
    connect(simControl, &SimulationControlWidget::pauseRequested, this, &MainWindow::pauseSimulation);
    connect(simControl, &SimulationControlWidget::stopRequested, this, &MainWindow::stopSimulation);
    connect(simControl, &SimulationControlWidget::resetRequested, this, &MainWindow::resetSimulation);
    connect(simControl, &SimulationControlWidget::stepRequested, this, &MainWindow::stepSimulation);
    connect(simControl, &SimulationControlWidget::speedChanged, this, [this](double speed) {
        simulationSpeed = speed;
        scheduler->set_time_scale(speed);
        updateStatusBar();
    });
}

void MainWindow::loadFirmware() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load ESP32 Firmware",
        "",
        "ELF Files (*.elf);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        loadFirmwareFile(filePath);
    }
}

void MainWindow::loadArduinoSketch() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Arduino Sketch",
        "",
        "Arduino Sketch (*.ino);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        // TODO: Implement Arduino sketch compilation
        QMessageBox::information(this, "Arduino Sketch", 
            "Arduino sketch compilation not yet implemented.\n"
            "Please compile the sketch to ELF format and load the firmware directly.");
    }
}

void MainWindow::saveProject() {
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Save Project",
        "",
        "JSON Files (*.json);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        currentProjectPath = filePath;
        // TODO: Implement project saving
        statusBar()->showMessage("Project saving not yet implemented", 3000);
    }
}

void MainWindow::loadProject() {
    QString filePath = QFileDialog::getOpenFileName(
        this,
        "Load Project",
        "",
        "JSON Files (*.json);;All Files (*)"
    );
    
    if (!filePath.isEmpty()) {
        currentProjectPath = filePath;
        // TODO: Implement project loading
        statusBar()->showMessage("Project loading not yet implemented", 3000);
    }
}

void MainWindow::exitApplication() {
    close();
}

void MainWindow::startSimulation() {
    if (currentFirmwarePath.isEmpty()) {
        QMessageBox::warning(this, "No Firmware", "Please load firmware before starting simulation.");
        return;
    }
    
    if (!simulationRunning) {
        simulationRunning = true;
        simulationPaused = false;
        
        // Start the simulation timer
        updateTimer->start();
        
        // Enable simulation controls
        enableSimulationControls(true);
        
        // Update status
        statusLabel->setText("Simulation running");
        updateStatusBar();
        
        // Notify components
        if (simControl) simControl->setRunning(true);
    }
}

void MainWindow::pauseSimulation() {
    if (simulationRunning && !simulationPaused) {
        simulationPaused = true;
        updateTimer->stop();
        
        statusLabel->setText("Simulation paused");
        updateStatusBar();
        
        if (simControl) simControl->setRunning(false);
    }
}

void MainWindow::stopSimulation() {
    if (simulationRunning) {
        simulationRunning = false;
        simulationPaused = false;
        updateTimer->stop();
        
        // Reset simulation
        resetSimulationState();
        
        statusLabel->setText("Simulation stopped");
        updateStatusBar();
        
        if (simControl) simControl->setRunning(false);
    }
}

void MainWindow::resetSimulation() {
    stopSimulation();
    
    // Reset all components
    if (iss) iss->reset();
    if (memory) memory->reset();
    if (scheduler) scheduler->reset();
    if (gpioModel) gpioModel->reset();
    
    // Reset time
    simulationTime = 0;
    
    statusLabel->setText("Simulation reset");
    updateStatusBar();
}

void MainWindow::stepSimulation() {
    if (!currentFirmwarePath.isEmpty()) {
        // Execute one instruction
        if (iss) {
            iss->step();
            simulationTime++;
            updateSimulationDisplay();
        }
    }
}

void MainWindow::showPinoutPanel() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && pinoutPanel) {
        pinoutPanel->setVisible(action->isChecked());
    }
}

void MainWindow::showWorkspace() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && workspace) {
        workspace->setVisible(action->isChecked());
    }
}

void MainWindow::showLogicAnalyzer() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && logicAnalyzer) {
        logicAnalyzer->setVisible(action->isChecked());
    }
}

void MainWindow::showStatusPanel() {
    QAction* action = qobject_cast<QAction*>(sender());
    if (action && statusWidget) {
        statusWidget->setVisible(action->isChecked());
    }
}

void MainWindow::openSettings() {
    QMessageBox::information(this, "Settings", "Settings dialog not yet implemented.");
}

void MainWindow::aboutSimulator() {
    QMessageBox::about(this, "ESP32 Virtual Hardware Simulation Environment",
        "<h2>ESP32 Virtual Hardware Simulation Environment</h2>"
        "<p>Version 1.0.0</p>"
        "<p>A high-fidelity desktop simulator for ESP32 firmware development.</p>"
        "<p>Features:</p>"
        "<ul>"
        "<li>Complete Xtensa LX6 instruction set simulation</li>"
        "<li>Full ESP32 peripheral modeling</li>"
        "<li>Interactive virtual components</li>"
        "<li>Real-time debugging and analysis</li>"
        "</ul>"
        "<p>&copy; 2024 ESP32 Small OS Project</p>");
}

void MainWindow::onSimulationUpdate() {
    if (simulationRunning && !simulationPaused) {
        // Update simulation time
        simulationTime += 50 * simulationSpeed; // 50ms timer interval
        
        // Update scheduler
        if (scheduler) {
            scheduler->set_time(simulationTime);
            scheduler->process_events();
        }
        
        // Update GPIO model
        if (gpioModel) {
            gpioModel->update(simulationTime);
        }
        
        // Update display
        updateSimulationDisplay();
    }
}

void MainWindow::onSimulationStateChanged() {
    // This slot can be called by other components when simulation state changes
    updateStatusBar();
}

void MainWindow::onGpioEvent(const GpioEvent& event) {
    // Handle GPIO events
    if (logicAnalyzer) {
        logicAnalyzer->addGpioEvent(event);
    }
    
    if (workspace) {
        workspace->handleGpioEvent(event);
    }
}

void MainWindow::updateSimulationDisplay() {
    // Update time display
    double timeSeconds = simulationTime / 1000000.0;
    timeLabel->setText(QString("Time: %1s").arg(timeSeconds, 0, 'f', 3));
    
    // Update status widget
    if (statusWidget) {
        statusWidget->updateSimulationState(simulationRunning, simulationPaused, simulationTime);
    }
    
    // Update pinout panel
    if (pinoutPanel) {
        pinoutPanel->updateDisplay();
    }
    
    // Update logic analyzer
    if (logicAnalyzer) {
        logicAnalyzer->updateDisplay();
    }
}

void MainWindow::updateStatusBar() {
    speedLabel->setText(QString("Speed: %1x").arg(simulationSpeed, 0, 'f', 1));
    
    if (simulationRunning) {
        progressBar->setVisible(true);
        progressBar->setValue(50); // Simulated progress
    } else {
        progressBar->setVisible(false);
    }
}

void MainWindow::enableSimulationControls(bool running) {
    // Update menu and toolbar actions
    QList<QAction*> actions = findChildren<QAction*>();
    for (QAction* action : actions) {
        QString text = action->text().remove("&");
        if (text == "Start") {
            action->setEnabled(!running);
        } else if (text == "Pause" || text == "Stop" || text == "Reset") {
            action->setEnabled(running);
        }
    }
}

bool MainWindow::loadFirmwareFile(const QString& filePath) {
    if (!elfLoader->load_file(filePath.toStdString())) {
        QMessageBox::critical(this, "Load Error", 
            QString("Failed to load firmware file:\n%1").arg(filePath));
        return false;
    }
    
    // Validate ELF
    if (!elfLoader->is_xtensa_elf()) {
        QMessageBox::critical(this, "Load Error", 
            "The selected file is not a valid ESP32 Xtensa ELF binary.");
        return false;
    }
    
    // Load segments into memory
    elfLoader->load_segments_to_memory([this](uint32_t address, const std::vector<uint8_t>& data) {
        if (memory) {
            for (size_t i = 0; i < data.size(); i++) {
                memory->write_byte(address + i, data[i]);
            }
        }
    });
    
    // Set entry point
    if (iss) {
        iss->set_pc(elfLoader->get_entry_point());
    }
    
    currentFirmwarePath = filePath;
    statusLabel->setText(QString("Firmware loaded: %1").arg(QFileInfo(filePath).fileName()));
    
    return true;
}

void MainWindow::resetSimulationState() {
    simulationRunning = false;
    simulationPaused = false;
    simulationTime = 0;
    simulationSpeed = 1.0;
    
    if (scheduler) {
        scheduler->set_time_scale(1.0);
    }
    
    updateTimer->stop();
    enableSimulationControls(false);
    updateStatusBar();
}

void MainWindow::connectGpioCallbacks() {
    if (gpioModel) {
        gpioModel->register_callback([this](const GpioEvent& event) {
            onGpioEvent(event);
        });
    }
}
