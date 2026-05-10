/**
 * @file oscilloscope.cpp
 * @brief Oscilloscope panel implementation
 */

#include "gui/oscilloscope.h"
#include "simulation_engine.h"
#include "utils/logger.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMenu>

namespace esp32sim {

Oscilloscope::Oscilloscope(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    initializeChannels();

    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Timer for updates
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, &QTimer::timeout, this, &Oscilloscope::updateDisplay);
    refresh_timer_->start(50);  // 20 Hz update

    LOG_INFO("Oscilloscope initialized");
}

Oscilloscope::~Oscilloscope() = default;

void Oscilloscope::initializeChannels() {
    for (int i = 0; i < NUM_CHANNELS; i++) {
        AnalogChannel& ch = channels_[i];
        ch.gpio_pin = 0;
        ch.name = QString("CH%1").arg(i + 1).toStdString();
        ch.enabled = (i == 0);  // Only CH1 enabled by default
        ch.color = QColor::fromHsl(i * 60, 255, 200);
        ch.vertical_scale_v_per_div = 1.0f;
        ch.vertical_offset_v = 1.65f;
    }
}

void Oscilloscope::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    drawBackground(painter);
    drawGrid(painter, rect());
    drawWaveforms(painter);
    drawMeasurements(painter);
    drawTriggerLine(painter);

    QWidget::paintEvent(event);
}

void Oscilloscope::drawBackground(QPainter& painter) {
    painter.fillRect(rect(), QColor(30, 30, 30));
}

void Oscilloscope::drawGrid(QPainter& painter, const QRect& rect) {
    painter.setPen(QPen(QColor(60, 60, 60), 0.5));

    // Vertical lines (time divisions)
    int x_step = width() / 10;
    for (int x = 0; x < width(); x += x_step) {
        painter.drawLine(x, 0, x, height());
    }

    // Horizontal lines (voltage divisions)
    int y_step = height() / 8;
    for (int y = 0; y < height(); y += y_step) {
        painter.drawLine(0, y, width(), y);
    }

    // Center line
    painter.setPen(QPen(QColor(100, 100, 100), 1));
    painter.drawLine(0, height() / 2, width(), height() / 2);
}

void Oscilloscope::drawWaveforms(QPainter& painter) {
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        const auto& channel = channels_[ch];
        if (!channel.enabled || channel.buffer.empty()) continue;

        drawChannelWaveform(painter, channel, 0, height());
    }
}

void Oscilloscope::drawChannelWaveform(QPainter& painter, const AnalogChannel& ch,
                                      int y_offset, int height) {
    QColor color = ch.color;
    painter.setPen(QPen(color, 2));

    int mid_y = height() / 2;
    float v_per_px = (ch.vertical_scale_v_per_div * 8.0f) / height();

    // Draw waveform
    QPolygon poly;
    for (size_t i = 0; i < ch.buffer.size(); i++) {
        int x = timeToX(ch.buffer[i].timestamp);
        int y = voltageToY(ch.buffer[i].voltage, ch - &channels_[0]);
        poly << QPoint(x, y);
    }

    painter.drawPolyline(poly);
}

void Oscilloscope::drawMeasurements(QPainter& painter) {
    if (!show_measurements_) return;

    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);

    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        const auto& m = measurements_[ch];
        int y = ch * 30 + 20;
        QString text = QString("CH%1: %2V %3Hz").arg(ch + 1).arg(yToVoltage(0, ch), 0, 'f', 2)
                        .arg(m.frequency_hz > 0 ? QString::number(m.frequency_hz, 'f', 1) : "---");
        painter.drawText(10, y, text);
    }
}

void Oscilloscope::drawTriggerLine(QPainter& painter) {
    if (!trigger_.enabled) return;

    int x = timeToX(trigger_.trigger_position_ns);
    painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
    painter.drawLine(x, 0, x, height());
}

void Oscilloscope::updateDisplay() {
    if (!engine_ || engine_->state() != SimulationState::RUNNING) return;

    // Sample active channels
    for (int ch = 0; ch < NUM_CHANNELS; ch++) {
        if (!channels_[ch].enabled) continue;

        // Get voltage from connected GPIO
        uint8_t pin = channels_[ch].gpio_pin;
        if (pin > 0) {
            GPIOController* gpio = engine_->gpio();
            if (gpio) {
                float voltage = gpio->getVoltage(pin);
                uint64_t now = engine_->simulationTime();
                addSample(ch, now, voltage);
            }
        }
    }

    // Check trigger
    processTrigger();

    update();
}

void Oscilloscope::addSample(int channel, uint64_t timestamp, float voltage) {
    auto& ch = channels_[channel];
    ch.buffer.emplace_back(timestamp, voltage);

    // Trim buffer
    const size_t max_size = AnalogChannel::MAX_BUFFER;
    while (ch.buffer.size() > max_size) {
        ch.buffer.pop_front();
    }

    // Trigger detection
    if (trigger_.enabled && channel == trigger_.channel) {
        if (!trigger_.armed &&
            ((trigger_.edge == 1 && voltage > trigger_.level_v && ch.buffer.size() >= 2 &&
              ch.buffer[ch.buffer.size() - 2].voltage < trigger_.level_v) ||
             (trigger_.edge == 0 && voltage < trigger_.level_v && ch.buffer.size() >= 2 &&
              ch.buffer[ch.buffer.size() - 2].voltage >= trigger_.level_v))) {
            trigger_.armed = false;
            trigger_.triggered = true;
            trigger_.trigger_position_ns = timestamp;
        }
    }
}

void Oscilloscope::processTrigger() {
    // Trigger logic handled in addSample
}

void Oscilloscope::computeMeasurements(int channel) {
    const auto& ch = channels_[channel];
    if (ch.buffer.empty()) return;

    auto& m = measurements_[channel];
    m.min_v = ch.buffer.front().voltage;
    m.max_v = m.min_v;
    m.rms_v = 0.0;

    size_t crossings = 0;
    bool last_above = (ch.buffer[0].voltage > 1.65f);

    for (const auto& sample : ch.buffer) {
        m.min_v = std::min(m.min_v, sample.voltage);
        m.max_v = std::max(m.max_v, sample.voltage);
        m.rms_v += sample.voltage * sample.voltage;

        bool above = (sample.voltage > 1.65f);
        if (above != last_above) crossings++;
        last_above = above;
    }

    m.rms_v = std::sqrt(m.rms_v / ch.buffer.size());

    if (ch.buffer.size() > 1) {
        uint64_t duration = ch.buffer.back().timestamp - ch.buffer.front().timestamp;
        if (duration > 0 && crossings > 0) {
            m.frequency_hz = (crossings / 2.0) / (duration / 1e9);
            m.period_us = 1e6 / m.frequency_hz;
        }
    }
}

void Oscilloscope::setTimebase(double s_per_div) {
    timebase_s_per_div_ = s_per_div;
    update();
}

void Oscilloscope::setVerticalScale(int channel, double v_per_div) {
    if (channel >= 0 && channel < NUM_CHANNELS) {
        channels_[channel].vertical_scale_v_per_div = v_per_div;
        update();
    }
}

void Oscilloscope::startAcquisition() {
    refresh_timer_->start(50);
    LOG_INFO("Oscilloscope acquisition started");
}

void Oscilloscope::stopAcquisition() {
    refresh_timer_->stop();
    LOG_INFO("Oscilloscope acquisition stopped");
}

bool Oscilloscope::exportToCSV(const std::string& filename, int channel) const {
    std::ofstream file(filename);
    if (!file.is_open()) return false;

    file << "Time (s),Voltage (V)\n";
    const auto& ch = channels_[channel];
    for (const auto& sample : ch.buffer) {
        file << (sample.timestamp / 1e9) << "," << sample.voltage << "\n";
    }

    file.close();
    LOG_INFO("Exported CSV for channel {} to {}", channel, filename);
    return true;
}

} // namespace esp32sim
