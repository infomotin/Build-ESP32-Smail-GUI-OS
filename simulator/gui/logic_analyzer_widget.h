#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QScrollArea>
#include <QtGui/QPainter>
#include <QtGui/QPixmap>
#include <QtCore/QTimer>

#include "../peripherals/gpio/gpio_model.h"

// Waveform data point
struct WaveformDataPoint {
    uint64_t timestamp;
    bool level;
    
    WaveformDataPoint(uint64_t ts, bool lvl) : timestamp(ts), level(lvl) {}
};

// Channel data
struct ChannelData {
    uint8_t pin;
    QString name;
    QColor color;
    std::vector<WaveformDataPoint> data;
    bool enabled;
    
    ChannelData(uint8_t p, const QString& n, const QColor& c) 
        : pin(p), name(n), color(c), enabled(true) {}
};

// Waveform display widget
class WaveformDisplay : public QWidget {
    Q_OBJECT

public:
    explicit WaveformDisplay(QWidget *parent = nullptr);
    
    void setChannels(const std::vector<ChannelData>& channels);
    void setTimeRange(uint64_t startTime, uint64_t endTime);
    void addDataPoint(uint8_t pin, uint64_t timestamp, bool level);
    void clearData();
    void setCursorPositions(uint64_t cursor1, uint64_t cursor2);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

signals:
    void cursorChanged(uint64_t cursor1, uint64_t cursor2);

private:
    void drawWaveforms(QPainter& painter);
    void drawGrid(QPainter& painter);
    void drawCursors(QPainter& painter);
    void drawChannelLabels(QPainter& painter);
    
    std::vector<ChannelData> channels;
    uint64_t timeStart;
    uint64_t timeEnd;
    uint64_t cursor1Pos;
    uint64_t cursor2Pos;
    
    int channelHeight;
    int headerHeight;
    int leftMargin;
    int rightMargin;
    
    bool draggingCursor;
    int activeCursor;
};

// Logic analyzer widget
class LogicAnalyzerWidget : public QWidget {
    Q_OBJECT

public:
    explicit LogicAnalyzerWidget(GpioModel* gpioModel, QWidget *parent = nullptr);
    
    void addGpioEvent(const class GpioEvent& event);
    void updateDisplay();
    void clearData();

signals:
    void pinSelected(uint8_t pin);

private slots:
    void onStartCapture();
    void onStopCapture();
    void onClearData();
    void onChannelToggled(int index);
    void onTimeScaleChanged(int index);
    void onTriggerChanged();

private:
    void createLayout();
    void createControls();
    void createWaveformDisplay();
    void updateChannelList();
    void captureEvents();
    
    GpioModel* gpioModel;
    
    // UI components
    QVBoxLayout* mainLayout;
    QHBoxLayout* controlLayout;
    QHBoxLayout* channelLayout;
    
    QPushButton* startButton;
    QPushButton* stopButton;
    QPushButton* clearButton;
    QComboBox* timeScaleCombo;
    QComboBox* triggerCombo;
    
    WaveformDisplay* waveformDisplay;
    
    // Channel management
    std::vector<ChannelData> channels;
    std::vector<QCheckBox*> channelCheckboxes;
    
    // Capture state
    bool capturing;
    uint64_t captureStartTime;
    uint64_t maxCaptureTime;
    
    // Time scale options (in microseconds)
    std::vector<uint64_t> timeScales;
    int currentTimeScale;
};
