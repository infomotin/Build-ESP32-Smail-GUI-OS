#include "status_widget.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>

StatusWidget::StatusWidget(QWidget *parent)
    : QWidget(parent), isRunning(false), isPaused(false), simulationTime(0)
{
    createLayout();
}

void StatusWidget::createLayout() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Simulation status
    QGroupBox* statusGroup = new QGroupBox("Simulation Status");
    QVBoxLayout* statusLayout = new QVBoxLayout(statusGroup);
    
    QHBoxLayout* statusRow = new QHBoxLayout();
    statusLabel = new QLabel("Status: Stopped");
    statusLabel->setStyleSheet("font-weight: bold; color: #F44336;");
    statusRow->addWidget(statusLabel);
    statusRow->addStretch();
    
    QHBoxLayout* timeRow = new QHBoxLayout();
    timeLabel = new QLabel("Time: 0.000s");
    timeRow->addWidget(timeLabel);
    timeRow->addStretch();
    
    QHBoxLayout* speedRow = new QHBoxLayout();
    speedLabel = new QLabel("Speed: 1.0x");
    speedRow->addWidget(speedLabel);
    speedRow->addStretch();
    
    statusLayout->addLayout(statusRow);
    statusLayout->addLayout(timeRow);
    statusLayout->addLayout(speedRow);
    
    // Memory usage
    QHBoxLayout* memoryRow = new QHBoxLayout();
    memoryRow->addWidget(new QLabel("Memory:"));
    memoryBar = new QProgressBar();
    memoryBar->setRange(0, 100);
    memoryBar->setValue(30);
    memoryBar->setFormat("%v% used");
    memoryRow->addWidget(memoryBar);
    statusLayout->addLayout(memoryRow);
    
    mainLayout->addWidget(statusGroup);
    
    // System information
    QGroupBox* systemGroup = new QGroupBox("System Information");
    QVBoxLayout* systemLayout = new QVBoxLayout(systemGroup);
    
    cpuUsageLabel = new QLabel("CPU Usage: 0%");
    memoryUsageLabel = new QLabel("Memory Usage: 0KB");
    instructionsLabel = new QLabel("Instructions: 0");
    
    systemLayout->addWidget(cpuUsageLabel);
    systemLayout->addWidget(memoryUsageLabel);
    systemLayout->addWidget(instructionsLabel);
    
    mainLayout->addWidget(systemGroup);
    
    // Log output
    QGroupBox* logGroup = new QGroupBox("Log Output");
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    
    logOutput = new QTextEdit();
    logOutput->setMaximumHeight(200);
    logOutput->setReadOnly(true);
    logOutput->setStyleSheet("QTextEdit { font-family: 'Courier New'; font-size: 9pt; }");
    
    logLayout->addWidget(logOutput);
    
    mainLayout->addWidget(logGroup);
    
    // Add initial log message
    addLogMessage("ESP32 Virtual Hardware Simulation Environment initialized");
    addLogMessage("Ready to load firmware...");
}

void StatusWidget::updateSimulationState(bool running, bool paused, uint64_t time) {
    isRunning = running;
    isPaused = paused;
    simulationTime = time;
    
    updateDisplay();
}

void StatusWidget::addLogMessage(const QString& message) {
    QString timestamp = QString("[%1.%2] ").arg(simulationTime / 1000000).arg((simulationTime % 1000000) / 1000, 3, 10, QChar('0'));
    logOutput->append(timestamp + message);
    
    // Auto-scroll to bottom
    QTextCursor cursor = logOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    logOutput->setTextCursor(cursor);
}

void StatusWidget::clearLog() {
    logOutput->clear();
}

void StatusWidget::updateDisplay() {
    // Update status
    if (isRunning && !isPaused) {
        statusLabel->setText("Status: Running");
        statusLabel->setStyleSheet("font-weight: bold; color: #4CAF50;");
        addLogMessage("Simulation started");
    } else if (isRunning && isPaused) {
        statusLabel->setText("Status: Paused");
        statusLabel->setStyleSheet("font-weight: bold; color: #FF9800;");
        addLogMessage("Simulation paused");
    } else {
        statusLabel->setText("Status: Stopped");
        statusLabel->setStyleSheet("font-weight: bold; color: #F44336;");
        addLogMessage("Simulation stopped");
    }
    
    // Update time
    double timeSeconds = simulationTime / 1000000.0;
    timeLabel->setText(QString("Time: %1s").arg(timeSeconds, 0, 'f', 3));
    
    // Update system info (simulated values)
    cpuUsageLabel->setText(QString("CPU Usage: %1%").arg(isRunning ? 45 : 0));
    memoryUsageLabel->setText(QString("Memory Usage: %1KB").arg(isRunning ? 128 : 64));
    instructionsLabel->setText(QString("Instructions: %1").arg(simulationTime / 1000)); // Rough estimate
}
