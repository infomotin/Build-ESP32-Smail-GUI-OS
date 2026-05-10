#include "simulation_control_widget.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QLabel>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QSpinBox>

SimulationControlWidget::SimulationControlWidget(QWidget *parent)
    : QWidget(parent), isRunning(false), isPaused(false)
{
    createLayout();
    updateButtonStates();
}

void SimulationControlWidget::createLayout() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Control buttons
    QGroupBox* controlGroup = new QGroupBox("Simulation Control");
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);
    
    // Button row 1
    QHBoxLayout* buttonLayout1 = new QHBoxLayout();
    
    startButton = new QPushButton("Start");
    startButton->setToolTip("Start simulation (F5)");
    startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    connect(startButton, &QPushButton::clicked, this, &SimulationControlWidget::onStartClicked);
    
    pauseButton = new QPushButton("Pause");
    pauseButton->setToolTip("Pause simulation (F6)");
    pauseButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
    pauseButton->setEnabled(false);
    connect(pauseButton, &QPushButton::clicked, this, &SimulationControlWidget::onPauseClicked);
    
    stopButton = new QPushButton("Stop");
    stopButton->setToolTip("Stop simulation (F7)");
    stopButton->setStyleSheet("QPushButton { background-color: #F44336; color: white; font-weight: bold; padding: 8px; }");
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &SimulationControlWidget::onStopClicked);
    
    buttonLayout1->addWidget(startButton);
    buttonLayout1->addWidget(pauseButton);
    buttonLayout1->addWidget(stopButton);
    
    // Button row 2
    QHBoxLayout* buttonLayout2 = new QHBoxLayout();
    
    resetButton = new QPushButton("Reset");
    resetButton->setToolTip("Reset simulation (F8)");
    resetButton->setStyleSheet("QPushButton { background-color: #9E9E9E; color: white; font-weight: bold; padding: 8px; }");
    connect(resetButton, &QPushButton::clicked, this, &SimulationControlWidget::onResetClicked);
    
    stepButton = new QPushButton("Step");
    stepButton->setToolTip("Single step (F10)");
    stepButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; font-weight: bold; padding: 8px; }");
    connect(stepButton, &QPushButton::clicked, this, &SimulationControlWidget::onStepClicked);
    
    buttonLayout2->addWidget(resetButton);
    buttonLayout2->addWidget(stepButton);
    buttonLayout2->addStretch();
    
    controlLayout->addLayout(buttonLayout1);
    controlLayout->addLayout(buttonLayout2);
    
    // Speed control
    QGroupBox* speedGroup = new QGroupBox("Simulation Speed");
    QVBoxLayout* speedLayout = new QVBoxLayout(speedGroup);
    
    speedLabel = new QLabel("Speed: 1.0x");
    speedLabel->setAlignment(Qt::AlignCenter);
    
    speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(1, 100); // 0.1x to 10x speed
    speedSlider->setValue(10); // 1.0x default
    speedSlider->setTickPosition(QSlider::TicksBelow);
    speedSlider->setTickInterval(10);
    connect(speedSlider, &QSlider::valueChanged, this, &SimulationControlWidget::onSpeedChanged);
    
    QHBoxLayout* speedBoxLayout = new QHBoxLayout();
    speedSpinBox = new QSpinBox();
    speedSpinBox->setRange(1, 100);
    speedSpinBox->setValue(10);
    speedSpinBox->setSuffix("%");
    connect(speedSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SimulationControlWidget::onSpeedChanged);
    
    speedBoxLayout->addWidget(new QLabel("0.1x"));
    speedBoxLayout->addWidget(speedSlider);
    speedBoxLayout->addWidget(new QLabel("10x"));
    speedBoxLayout->addWidget(speedSpinBox);
    
    speedLayout->addWidget(speedLabel);
    speedLayout->addLayout(speedBoxLayout);
    
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(speedGroup);
    mainLayout->addStretch();
}

void SimulationControlWidget::setRunning(bool running) {
    isRunning = running;
    updateButtonStates();
}

void SimulationControlWidget::setPaused(bool paused) {
    isPaused = paused;
    updateButtonStates();
}

void SimulationControlWidget::setSpeed(double speed) {
    int sliderValue = static_cast<int>(speed * 10);
    speedSlider->setValue(sliderValue);
    speedSpinBox->setValue(sliderValue);
    speedLabel->setText(QString("Speed: %1x").arg(speed, 0, 'f', 1));
}

void SimulationControlWidget::onStartClicked() {
    emit startRequested();
}

void SimulationControlWidget::onPauseClicked() {
    emit pauseRequested();
}

void SimulationControlWidget::onStopClicked() {
    emit stopRequested();
}

void SimulationControlWidget::onResetClicked() {
    emit resetRequested();
}

void SimulationControlWidget::onStepClicked() {
    emit stepRequested();
}

void SimulationControlWidget::onSpeedChanged(int value) {
    double speed = value / 10.0;
    speedSpinBox->setValue(value);
    speedLabel->setText(QString("Speed: %1x").arg(speed, 0, 'f', 1));
    emit speedChanged(speed);
}

void SimulationControlWidget::updateButtonStates() {
    startButton->setEnabled(!isRunning);
    pauseButton->setEnabled(isRunning && !isPaused);
    stopButton->setEnabled(isRunning);
    stepButton->setEnabled(!isRunning);
    
    if (isRunning && !isPaused) {
        pauseButton->setText("Pause");
        pauseButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
    } else if (isRunning && isPaused) {
        pauseButton->setText("Resume");
        pauseButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 8px; }");
    } else {
        pauseButton->setText("Pause");
        pauseButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 8px; }");
    }
}
