/**
 * @file logic_analyzer.h
 * @brief Logic analyzer panel for capturing digital waveforms
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <vector>
#include <deque>
#include <atomic>
#include <memory>

#include "simulator/core/memory/memory_model.h"
#include "peripherals/gpio_controller.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
QT_END_NAMESPACE

namespace esp32sim {

/**
 * @struct WaveformSample
 * @brief Single sample in the waveform buffer
 */
struct WaveformSample {
    uint64_t timestamp;  // Simulation time in nanoseconds
    std::vector<bool> levels;  // One bool per channel

    WaveformSample(uint64_t ts, int num_channels)
        : timestamp(ts), levels(num_channels, false) {}
};

/**
 * @struct CaptureSettings
 * @brief Logic analyzer configuration
 */
struct CaptureSettings {
    uint64_t sample_rate_hz = 10000000;  // 10 MHz default
    uint32_t num_channels = 0;
    uint64_t buffer_size = 1000000;      // 1M samples per channel
    uint64_t pre_trigger_samples = 10000;
    bool trigger_enabled = false;
    uint32_t trigger_channel = 0;
    int trigger_edge = 1;  // 1=rising, 0=falling, -1=any
    uint32_t trigger_value = 0;  // For pattern triggering
    uint64_t timeout_us = 1000000; // 1 second capture timeout
};

/**
 * @class LogicAnalyzer
 * @brief Logic analyzer panel for digital signal capture
 *
 * Features:
 * - Up to 34 channels (all GPIOs)
 * - Up to 10 MS/s sampling rate
 * - Trigger on edge or pattern
 * - Cursors for time/frequency measurement
 * - Protocol overlay (I2C, SPI, UART)
 * - VCD export
 */
class LogicAnalyzer : public QWidget {
    Q_OBJECT

public:
    explicit LogicAnalyzer(SimulationEngine* engine, QWidget* parent = nullptr);
    ~LogicAnalyzer() override;

    /**
     * @brief Set enabled channels
     */
    void setChannels(const std::vector<uint8_t>& channels);

    /**
     * @brief Get capture settings
     */
    const CaptureSettings& settings() const { return settings_; }

    /**
     * @brief Set capture settings
     */
    void setSettings(const CaptureSettings& settings);

    /**
     * @brief Start a capture
     */
    void startCapture();

    /**
     * @brief Stop current capture
     */
    void stopCapture();

    /**
     * @brief Check if capturing
     */
    bool isCapturing() const { return capturing_; }

    /**
     * @brief Get captured waveform data
     */
    const std::deque<WaveformSample>& waveform() const { return waveform_buffer_; }

    /**
     * @brief Export to VCD file
     */
    bool exportToVCD(const std::string& filename);

    /**
     * @brief Add protocol decoder annotation
     */
    void addProtocolAnnotation(uint64_t timestamp, uint32_t channel_mask,
                               const std::string& label, const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    /**
     * @brief Update from GPIO samples
     */
    void updateFromEngine();

    /**
     * @brief Auto-scroll toggled
     */
    void onAutoScrollToggled(bool checked);

    /**
     * @brief Timebase changed
     */
    void onTimebaseChanged(int index);

private:
    SimulationEngine* engine_ = nullptr;
    GPIOController* gpio_ = nullptr;

    // Configuration
    CaptureSettings settings_;
    std::vector<uint8_t> channels_;  // Sorted GPIO numbers

    // Capture state
    std::atomic<bool> capturing_{false};
    std::atomic<bool> trigger_armed_{false};
    uint64_t capture_start_time_ = 0;
    uint64_t trigger_time_ = 0;
    std::deque<WaveformSample> waveform_buffer_;

    // Display
    QTimer* refresh_timer_ = nullptr;
    float vertical_scale_ = 10.0f;  // Pixels per channel
    float horizontal_scale_ = 1.0f; // Pixels per nanosecond
    QPoint view_offset_ = QPoint(0, 0);
    QPoint cursor1_pos_;
    QPoint cursor2_pos_;
    bool show_cursors_ = false;
    bool auto_scroll_ = true;

    // Cursor measurements
    struct Measurement {
        uint64_t time_diff_ns = 0;
        double frequency_hz = 0.0;
        double duty_cycle = 0.0;
    } measurement_;

    // Protocol annotations
    struct Annotation {
        uint64_t timestamp;
        uint32_t channels;
        std::string label;
        QColor color;
    };
    std::vector<Annotation> annotations_;

    // Context menu
    QMenu* context_menu_ = nullptr;

    // UI controls
    QComboBox* timebase_combo_ = nullptr;
    QSpinBox* sample_rate_spin_ = nullptr;
    QCheckBox* trigger_enabled_check_ = nullptr;
    QComboBox* trigger_channel_combo_ = nullptr;
    QComboBox* trigger_edge_combo_ = nullptr;
    QPushButton* capture_button_ = nullptr;

    // Internal methods
    void initializeUI();
    void drawBackground(QPainter* painter);
    void drawWaveforms(QPainter* painter);
    void drawGrid(QPainter* painter);
    void drawCursors(QPainter* painter);
    void drawAnnotations(QPainter* painter);
    void drawChannelLabels(QPainter* painter);

    void handleMouseWheel(QWheelEvent* event);
    void handleMousePress(QMouseEvent* event);
    void handleMouseMove(QMouseEvent* event);
    void handleMouseRelease(QMouseEvent* event);

    void processGPIOChange(uint64_t timestamp, uint8_t pin, bool level);
    bool checkTriggerCondition();
    void applyTrigger();
    void updateDisplay();
    QString formatTime(uint64_t ns) const;
    QString formatFrequency(double hz) const;

    /**
     * @brief Convert timestamp to X coordinate
     */
    int timeToX(uint64_t timestamp) const {
        int64_t diff = (int64_t)timestamp - (int64_t)trigger_time_;
        return view_offset_.x() + static_cast<int>(diff * horizontal_scale_ / 1000.0);
    }

    /**
     * @brief Convert X coordinate to timestamp
     */
    uint64_t xToTime(int x) const {
        return trigger_time_ + static_cast<uint64_t>((x - view_offset_.x()) * 1000.0 / horizontal_scale_);
    }
};

} // namespace esp32sim
