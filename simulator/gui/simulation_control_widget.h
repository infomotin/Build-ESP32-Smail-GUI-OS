#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSpinBox>

class SimulationControlWidget : public QWidget {
    Q_OBJECT

public:
    explicit SimulationControlWidget(QWidget *parent = nullptr);
    
    void setRunning(bool running);
    void setPaused(bool paused);
    void setSpeed(double speed);

signals:
    void startRequested();
    void pauseRequested();
    void stopRequested();
    void resetRequested();
    void stepRequested();
    void speedChanged(double speed);

private slots:
    void onStartClicked();
    void onPauseClicked();
    void onStopClicked();
    void onResetClicked();
    void onStepClicked();
    void onSpeedChanged(int value);

private:
    void createLayout();
    void updateButtonStates();
    
    QPushButton* startButton;
    QPushButton* pauseButton;
    QPushButton* stopButton;
    QPushButton* resetButton;
    QPushButton* stepButton;
    
    QSlider* speedSlider;
    QSpinBox* speedSpinBox;
    QLabel* speedLabel;
    
    bool isRunning;
    bool isPaused;
};
