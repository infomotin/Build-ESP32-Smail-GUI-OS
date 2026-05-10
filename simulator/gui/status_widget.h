#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QProgressBar>
#include <QtWidgets/QTextEdit>

class StatusWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatusWidget(QWidget *parent = nullptr);
    
    void updateSimulationState(bool running, bool paused, uint64_t time);
    void addLogMessage(const QString& message);
    void clearLog();

private:
    void createLayout();
    void updateDisplay();
    
    // Simulation status
    QLabel* statusLabel;
    QLabel* timeLabel;
    QLabel* speedLabel;
    QProgressBar* memoryBar;
    
    // System information
    QLabel* cpuUsageLabel;
    QLabel* memoryUsageLabel;
    QLabel* instructionsLabel;
    
    // Log output
    QTextEdit* logOutput;
    
    // Current state
    bool isRunning;
    bool isPaused;
    uint64_t simulationTime;
};
