/**
 * @file oscilloscope.h
 * @brief Oscilloscope panel for viewing analog waveforms
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QTimer>
#include <vector>
#include <deque>
#include <atomic>

#include "simulator/core/memory/memory_model.h"

QT_BEGIN_NAMESPACE
class QComboBox;
class QSpinBox;
class QCheckBox;
class QPushButton;
class QLabel;
QT_END_NAMESPACE

namespace esp32sim {

class SimulationEngine;

/**
 * @struct AnalogSample
 * @brief Single analog voltage sample
 */
struct AnalogSample {
    uint64_t timestamp;
    float voltage;  // 0-3.3V range

    AnalogSample(uint64_t ts, float v) : timestamp(ts), voltage(v) {}
};

/**
 * @struct AnalogChannel
 * @brief Configuration for one oscilloscope channel
 */
struct AnalogChannel {
    uint8_t gpio_pin = 0;
    std::string name;
    QColor color = Qt::red;
    bool enabled = false;
    bool coupling_dc = true;  // true = DC, false = AC
    float vertical_scale_v_per_div = 1.0f;  // V/div
    float vertical_offset_v = 1.65f;        // Center voltage (1.65V)
    float probe_attenuation = 1.0f;         // 1x, 10x, etc.
    std::deque<AnalogSample> buffer;
    static constexpr size_t MAX_BUFFER = 100000;  // ~1 second at 100ksps
};

/**
 * @class Oscilloscope
 * @brief Oscilloscope view for analog signals
 *
 * Features:
 * - Up to 4 simultaneous channels
 * - Timebase: 1 µs/div to 1 s/div
 * - Voltage scale: 0.1 V/div to 3.3 V/div
 * - Automatic measurements: freq, period, duty, min/max, RMS
 * - PWM demodulation display
 * - Trigger support
 */
class Oscilloscope : public QWidget {
    Q_OBJECT

public:
    explicit Oscilloscope(SimulationEngine* engine, QWidget* parent = nullptr);
    ~Oscilloscope() override;

    /**
     * @brief Enable/disable a channel
     */
    void setChannelEnabled(int channel, bool enabled);

    /**
     * @brief Configure channel
     */
    void configureChannel(int channel, uint8_t gpio_pin, const QColor& color);

    /**
     * @brief Get channel by index
     */
    const AnalogChannel* getChannel(int index) const;

    /**
     * @brief Set timebase (seconds per division)
     */
    void setTimebase(double s_per_div);

    /**
     * @brief Set voltage scale for channel (volts per division)
     */
    void setVerticalScale(int channel, double v_per_div);

    /**
     * @brief Get current measurements for channel
     */
    struct Measurements {
        double frequency_hz = 0.0;
        double period_us = 0.0;
        double duty_cycle = 0.0;
        double min_v = 0.0;
        double max_v = 0.0;
        double rms_v = 0.0;
        double avg_v = 0.0;
        uint64_t edge_count = 0;
    };
    const Measurements& measurements(int channel) const { return measurements_[channel]; }

    /**
     * @brief Start continuous acquisition
     */
    void startAcquisition();

    /**
     * @brief Stop acquisition
     */
    void stopAcquisition();

    /**
     * @brief Single capture (one screen)
     */
    void singleCapture();

    /**
     * @brief Save waveform to CSV
     */
    bool exportToCSV(const std::string& filename, int channel) const;

    /**
     * @brief Clear all buffers
     */
    void clear();

public slots:
    /**
     * @brief Update display
     */
    void updateDisplay();

    /**
     * @brief Handle simulation state change
     */
    void onSimulationStateChanged(SimulationState state);

private slots:
    /**
     * @brief Refresh rate changed
     */
    void onRefreshRateChanged(int index);

    /**
     * @brief Trigger settings changed
     */
    void onTriggerSettingsChanged();

private:
    SimulationEngine* engine_ = nullptr;

    // Configuration (static)
    static constexpr int NUM_CHANNELS = 4;
    static constexpr double MIN_TIME_BASE = 1e-6;    // 1 µs/div
    static constexpr double MAX_TIME_BASE = 1.0;     // 1 s/div
    static constexpr double MIN_V_SCALE = 0.1;       // 0.1 V/div
    static constexpr double MAX_V_SCALE = 3.3;       // 3.3 V/div
    static constexpr float VOLTAGE_RANGE = 3.3f;      // 0-3.3V

    // Channels
    std::array<AnalogChannel, NUM_CHANNELS> channels_;
    int active_channel_ = 0;

    // Timebase
    double timebase_s_per_div_ = 0.001;  // 1ms/div default
    double sweep_speed_ = 0.0001;        // 100 µs update interval

    // Trigger
    struct Trigger {
        bool enabled = false;
        int channel = 0;
        double level_v = 1.65f;
        int edge = 1;  // 1 = rising, 0 = falling
        bool armed = false;
        bool triggered = false;
        uint64_t trigger_position_ns = 0;
    } trigger_;

    // Display
    QTimer* refresh_timer_ = nullptr;
    QPoint view_origin_ = QPoint(0, 0);
    bool show_grid_ = true;
    bool show_measurements_ = true;
    Measurements measurements_;

    // UI controls
    QComboBox* timebase_combo_ = nullptr;
    QComboBox* channel_selector_ = nullptr;
    QSpinBox* vscale_spin_ = nullptr;
    QCheckBox* trigger_check_ = nullptr;
    QComboBox* trigger_edge_combo_ = nullptr;

    // Internal methods
    void initializeChannels();
    void drawBackground(QPainter& painter);
    void drawGrid(QPainter& painter, const QRect& rect);
    void drawWaveforms(QPainter& painter);
    void drawChannelWaveform(QPainter& painter, const AnalogChannel& ch, int y_offset, int height);
    void drawMeasurements(QPainter& painter);
    void drawTriggerLine(QPainter& painter);
    void drawTimeMarkers(QPainter& painter);
    void drawCursor(QPainter& painter, int x, const QColor& color, const QString& label);

    void addSample(int channel, uint64_t timestamp, float voltage);
    void processTrigger();
    void computeMeasurements(int channel);
    QString formatVoltage(double v) const;
    QString formatTime(double s) const;
    QString formatFrequency(double hz) const;

    void handleMousePress(QMouseEvent* event);
    void handleMouseMove(QMouseEvent* event);
    void handleMouseRelease(QMouseEvent* event);

    // Coordinate conversion
    QPoint voltageToPoint(double v, int channel) const;
    double pointToVoltage(int y, int channel) const;
    uint64_t pointToTime(int x) const;
    int timeToPoint(uint64_t time) const;

    /**
     * @brief Voltage to Y coordinate
     */
    int voltageToY(float v, int channel) const {
        const AnalogChannel& ch = channels_[channel];
        float volts_per_pixel = (ch.vertical_scale_v_per_div * 8.0f) / height();
        float center_y = height() / 2.0f;
        return static_cast<int>(center_y - (v - ch.vertical_offset_v) / volts_per_pixel);
    }

    /**
     * @brief Y coordinate to voltage
     */
    float yToVoltage(int y, int channel) const {
        const AnalogChannel& ch = channels_[channel];
        float volts_per_pixel = (ch.vertical_scale_v_per_div * 8.0f) / height();
        float center_y = height() / 2.0f;
        return ch.vertical_offset_v + (center_y - y) * volts_per_pixel;
    }

    /**
     * @brief Time to X coordinate
     */
    int timeToX(uint64_t time_ns) const {
        double total_s = static_cast<double>(time_ns) / 1e9;
        double current_time = static_cast<double>(engine_->simulationTime()) / 1e9;
        double relative_s = total_s - current_time + (timebase_s_per_div_ * 10.0);
        return static_cast<int>(relative_s / timebase_s_per_div_ * width() / 10.0) + width() / 2;
    }
};

} // namespace esp32sim
