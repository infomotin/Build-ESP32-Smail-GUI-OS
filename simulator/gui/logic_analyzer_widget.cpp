#include "logic_analyzer_widget.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QScrollArea>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>

// WaveformDisplay implementation
WaveformDisplay::WaveformDisplay(QWidget *parent)
    : QWidget(parent), timeStart(0), timeEnd(1000000), cursor1Pos(0), cursor2Pos(0),
      channelHeight(30), headerHeight(20), leftMargin(60), rightMargin(20),
      draggingCursor(false), activeCursor(0)
{
    setMinimumHeight(200);
    setStyleSheet("background-color: #1e1e1e; border: 1px solid #333;");
    
    // Initialize with some default channels
    channels = {
        ChannelData(2, "GPIO 2", QColor(255, 0, 0)),
        ChannelData(4, "GPIO 4", QColor(0, 255, 0)),
        ChannelData(5, "GPIO 5", QColor(0, 0, 255)),
        ChannelData(12, "GPIO 12", QColor(255, 255, 0)),
        ChannelData(13, "GPIO 13", QColor(255, 0, 255)),
        ChannelData(14, "GPIO 14", QColor(0, 255, 255)),
        ChannelData(15, "GPIO 15", QColor(255, 128, 0)),
        ChannelData(16, "GPIO 16", QColor(128, 255, 0))
    };
}

void WaveformDisplay::setChannels(const std::vector<ChannelData>& newChannels) {
    channels = newChannels;
    update();
}

void WaveformDisplay::setTimeRange(uint64_t startTime, uint64_t endTime) {
    timeStart = startTime;
    timeEnd = endTime;
    update();
}

void WaveformDisplay::addDataPoint(uint8_t pin, uint64_t timestamp, bool level) {
    for (auto& channel : channels) {
        if (channel.pin == pin && channel.enabled) {
            channel.data.emplace_back(timestamp, level);
            
            // Limit data points to prevent memory issues
            if (channel.data.size() > 10000) {
                channel.data.erase(channel.data.begin(), channel.data.begin() + 1000);
            }
            break;
        }
    }
}

void WaveformDisplay::clearData() {
    for (auto& channel : channels) {
        channel.data.clear();
    }
    update();
}

void WaveformDisplay::setCursorPositions(uint64_t cursor1, uint64_t cursor2) {
    cursor1Pos = cursor1;
    cursor2Pos = cursor2;
    update();
}

void WaveformDisplay::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Fill background
    painter.fillRect(rect(), QColor(30, 30, 30));
    
    drawGrid(painter);
    drawWaveforms(painter);
    drawCursors(painter);
    drawChannelLabels(painter);
}

void WaveformDisplay::drawGrid(QPainter& painter) {
    painter.setPen(QColor(60, 60, 60));
    
    int width = rect().width() - leftMargin - rightMargin;
    int height = rect().height() - headerHeight;
    
    // Vertical time lines
    for (int i = 0; i <= 10; i++) {
        int x = leftMargin + (width * i / 10);
        painter.drawLine(x, headerHeight, x, height);
    }
    
    // Horizontal channel lines
    for (size_t i = 0; i <= channels.size(); i++) {
        int y = headerHeight + (height * i / channels.size());
        painter.drawLine(leftMargin, y, leftMargin + width, y);
    }
}

void WaveformDisplay::drawWaveforms(QPainter& painter) {
    int width = rect().width() - leftMargin - rightMargin;
    int height = rect().height() - headerHeight;
    
    for (size_t i = 0; i < channels.size(); i++) {
        const auto& channel = channels[i];
        if (!channel.enabled || channel.data.empty()) continue;
        
        painter.setPen(QPen(channel.color, 2));
        
        int y = headerHeight + (height * i / channels.size()) + channelHeight / 2;
        
        // Draw waveform
        for (size_t j = 1; j < channel.data.size(); j++) {
            const auto& prevPoint = channel.data[j - 1];
            const auto& currPoint = channel.data[j];
            
            // Convert timestamps to x positions
            int x1 = leftMargin + (width * (prevPoint.timestamp - timeStart) / (timeEnd - timeStart));
            int x2 = leftMargin + (width * (currPoint.timestamp - timeStart) / (timeEnd - timeStart));
            
            // Clamp to visible area
            x1 = std::max(leftMargin, std::min(x1, leftMargin + width));
            x2 = std::max(leftMargin, std::min(x2, leftMargin + width));
            
            if (prevPoint.level != currPoint.level) {
                // Draw vertical transition
                int yLow = y + channelHeight / 4;
                int yHigh = y - channelHeight / 4;
                painter.drawLine(x1, prevPoint.level ? yLow : yHigh, x1, currPoint.level ? yLow : yHigh);
            }
            
            // Draw horizontal line
            int yLevel = currPoint.level ? (y + channelHeight / 4) : (y - channelHeight / 4);
            painter.drawLine(x1, yLevel, x2, yLevel);
        }
    }
}

void WaveformDisplay::drawCursors(QPainter& painter) {
    int width = rect().width() - leftMargin - rightMargin;
    
    // Draw cursor 1
    if (cursor1Pos >= timeStart && cursor1Pos <= timeEnd) {
        int x = leftMargin + (width * (cursor1Pos - timeStart) / (timeEnd - timeStart));
        painter.setPen(QPen(QColor(255, 255, 0), 2, Qt::DashLine));
        painter.drawLine(x, headerHeight, x, rect().height());
    }
    
    // Draw cursor 2
    if (cursor2Pos >= timeStart && cursor2Pos <= timeEnd) {
        int x = leftMargin + (width * (cursor2Pos - timeStart) / (timeEnd - timeStart));
        painter.setPen(QPen(QColor(0, 255, 255), 2, Qt::DashLine));
        painter.drawLine(x, headerHeight, x, rect().height());
    }
}

void WaveformDisplay::drawChannelLabels(QPainter& painter) {
    painter.setPen(QColor(200, 200, 200));
    QFont font("Arial", 9);
    painter.setFont(font);
    
    int height = rect().height() - headerHeight;
    
    for (size_t i = 0; i < channels.size(); i++) {
        const auto& channel = channels[i];
        if (!channel.enabled) continue;
        
        int y = headerHeight + (height * i / channels.size()) + channelHeight / 2;
        painter.drawText(5, y + 5, channel.name);
    }
}

void WaveformDisplay::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        int width = rect().width() - leftMargin - rightMargin;
        int x = event->x();
        
        if (x >= leftMargin && x <= leftMargin + width) {
            uint64_t timestamp = timeStart + ((x - leftMargin) * (timeEnd - timeStart) / width);
            
            // Set cursor 1
            cursor1Pos = timestamp;
            draggingCursor = true;
            activeCursor = 1;
            update();
        }
    }
}

void WaveformDisplay::mouseMoveEvent(QMouseEvent *event) {
    if (draggingCursor) {
        int width = rect().width() - leftMargin - rightMargin;
        int x = event->x();
        
        if (x >= leftMargin && x <= leftMargin + width) {
            uint64_t timestamp = timeStart + ((x - leftMargin) * (timeEnd - timeStart) / width);
            
            if (activeCursor == 1) {
                cursor1Pos = timestamp;
            } else {
                cursor2Pos = timestamp;
            }
            update();
        }
    }
}

void WaveformDisplay::wheelEvent(QWheelEvent *event) {
    // Zoom in/out
    if (event->angleDelta().y() > 0) {
        // Zoom in
        uint64_t center = (timeStart + timeEnd) / 2;
        uint64_t range = (timeEnd - timeStart) / 2;
        timeStart = center - range / 2;
        timeEnd = center + range / 2;
    } else {
        // Zoom out
        uint64_t center = (timeStart + timeEnd) / 2;
        uint64_t range = (timeEnd - timeStart) * 2;
        timeStart = center - range / 2;
        timeEnd = center + range / 2;
    }
    update();
}

// LogicAnalyzerWidget implementation
LogicAnalyzerWidget::LogicAnalyzerWidget(GpioModel* gpioModel, QWidget *parent)
    : QWidget(parent), gpioModel(gpioModel), capturing(false), captureStartTime(0),
      maxCaptureTime(10000000), currentTimeScale(3) // Default 1ms
{
    createLayout();
    createControls();
    createWaveformDisplay();
    
    // Initialize time scales
    timeScales = {
        1000,      // 1μs
        10000,     // 10μs
        100000,    // 100μs
        1000000,   // 1ms
        10000000,  // 10ms
        100000000, // 100ms
        1000000000 // 1s
    };
}

void LogicAnalyzerWidget::createLayout() {
    mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel* titleLabel = new QLabel("Logic Analyzer");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont("Arial", 12, QFont::Bold);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Controls
    controlLayout = new QHBoxLayout();
    mainLayout->addLayout(controlLayout);
    
    // Channel controls
    channelLayout = new QHBoxLayout();
    mainLayout->addLayout(channelLayout);
}

void LogicAnalyzerWidget::createControls() {
    // Capture controls
    startButton = new QPushButton("Start Capture");
    startButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; padding: 5px; }");
    connect(startButton, &QPushButton::clicked, this, &LogicAnalyzerWidget::onStartCapture);
    controlLayout->addWidget(startButton);
    
    stopButton = new QPushButton("Stop Capture");
    stopButton->setStyleSheet("QPushButton { background-color: #F44336; color: white; font-weight: bold; padding: 5px; }");
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &LogicAnalyzerWidget::onStopCapture);
    controlLayout->addWidget(stopButton);
    
    clearButton = new QPushButton("Clear Data");
    clearButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; font-weight: bold; padding: 5px; }");
    connect(clearButton, &QPushButton::clicked, this, &LogicAnalyzerWidget::onClearData);
    controlLayout->addWidget(clearButton);
    
    controlLayout->addStretch();
    
    // Time scale
    controlLayout->addWidget(new QLabel("Time Scale:"));
    timeScaleCombo = new QComboBox();
    timeScaleCombo->addItem("1μs", 0);
    timeScaleCombo->addItem("10μs", 1);
    timeScaleCombo->addItem("100μs", 2);
    timeScaleCombo->addItem("1ms", 3);
    timeScaleCombo->addItem("10ms", 4);
    timeScaleCombo->addItem("100ms", 5);
    timeScaleCombo->addItem("1s", 6);
    timeScaleCombo->setCurrentIndex(3);
    connect(timeScaleCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &LogicAnalyzerWidget::onTimeScaleChanged);
    controlLayout->addWidget(timeScaleCombo);
    
    // Trigger
    controlLayout->addWidget(new QLabel("Trigger:"));
    triggerCombo = new QComboBox();
    triggerCombo->addItem("None");
    triggerCombo->addItem("Rising Edge");
    triggerCombo->addItem("Falling Edge");
    triggerCombo->addItem("Both Edges");
    connect(triggerCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), 
            this, &LogicAnalyzerWidget::onTriggerChanged);
    controlLayout->addWidget(triggerCombo);
}

void LogicAnalyzerWidget::createWaveformDisplay() {
    waveformDisplay = new WaveformDisplay();
    mainLayout->addWidget(waveformDisplay, 1);
    
    // Create channel checkboxes
    updateChannelList();
}

void LogicAnalyzerWidget::updateChannelList() {
    // Clear existing checkboxes
    for (auto* checkbox : channelCheckboxes) {
        delete checkbox;
    }
    channelCheckboxes.clear();
    
    // Add checkboxes for each channel
    for (size_t i = 0; i < waveformDisplay->getChannels().size(); i++) {
        const auto& channel = waveformDisplay->getChannels()[i];
        QCheckBox* checkbox = new QCheckBox(channel.name);
        checkbox->setChecked(channel.enabled);
        checkbox->setStyleSheet(QString("QCheckBox { color: %1; }").arg(channel.color.name()));
        connect(checkbox, QOverload<int>::of(&QCheckBox::stateChanged), 
                this, [this, i](int state) { onChannelToggled(i); });
        channelLayout->addWidget(checkbox);
        channelCheckboxes.push_back(checkbox);
    }
    
    channelLayout->addStretch();
}

void LogicAnalyzerWidget::addGpioEvent(const GpioEvent& event) {
    if (capturing) {
        waveformDisplay->addDataPoint(event.pin, event.timestamp, event.data != 0);
        waveformDisplay->update();
    }
}

void LogicAnalyzerWidget::updateDisplay() {
    waveformDisplay->update();
}

void LogicAnalyzerWidget::clearData() {
    waveformDisplay->clearData();
}

void LogicAnalyzerWidget::onStartCapture() {
    capturing = true;
    captureStartTime = 0; // Will be set when first event arrives
    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    
    // Clear previous data
    waveformDisplay->clearData();
    
    // Set time range based on current scale
    uint64_t timeScale = timeScales[currentTimeScale];
    uint64_t duration = timeScale * 100; // Show 100 divisions
    waveformDisplay->setTimeRange(0, duration);
}

void LogicAnalyzerWidget::onStopCapture() {
    capturing = false;
    startButton->setEnabled(true);
    stopButton->setEnabled(false);
}

void LogicAnalyzerWidget::onClearData() {
    waveformDisplay->clearData();
}

void LogicAnalyzerWidget::onChannelToggled(int index) {
    if (index >= 0 && index < channelCheckboxes.size()) {
        bool enabled = channelCheckboxes[index]->isChecked();
        // Update channel enabled state
        // Note: This would require modifying the WaveformDisplay to allow channel state changes
        waveformDisplay->update();
    }
}

void LogicAnalyzerWidget::onTimeScaleChanged(int index) {
    currentTimeScale = index;
    uint64_t timeScale = timeScales[currentTimeScale];
    uint64_t duration = timeScale * 100; // Show 100 divisions
    waveformDisplay->setTimeRange(0, duration);
}

void LogicAnalyzerWidget::onTriggerChanged() {
    // TODO: Implement trigger functionality
}
