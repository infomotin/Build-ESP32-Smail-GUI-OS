/**
 * @file logic_analyzer.cpp
 * @brief Logic analyzer panel implementation
 */

#include "gui/logic_analyzer.h"
#include "simulation_engine.h"
#include "peripherals/gpio_controller.h"
#include "utils/logger.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMenu>
#include <QFileDialog>
#include <fstream>

namespace esp32sim {

LogicAnalyzer::LogicAnalyzer(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    if (!engine_) {
        LOG_ERROR("LogicAnalyzer: null engine");
        return;
    }

    gpio_ = engine_->gpio();
    if (!gpio_) {
        LOG_ERROR("LogicAnalyzer: null GPIO");
        return;
    }

    // Default to all GPIO pins
    for (uint8_t i = 0; i < 40; i++) {
        channels_.push_back(i);
    }
    settings_.num_channels = channels_.size();

    initializeUI();

    // Update timer at 60 Hz
    refresh_timer_ = new QTimer(this);
    connect(refresh_timer_, &QTimer::timeout, this, &LogicAnalyzer::updateFromEngine);
    refresh_timer_->start(16);

    LOG_INFO("LogicAnalyzer initialized with {} channels", settings_.num_channels);
}

LogicAnalyzer::~LogicAnalyzer() = default;

void LogicAnalyzer::initializeUI() {
    // Create controls
    QVBoxLayout* layout = new QVBoxLayout(this);
    setLayout(layout);

    // Control bar
    QHBoxLayout* controls = new QHBoxLayout();

    QPushButton* capture_btn = new QPushButton("Capture", this);
    connect(capture_btn, &QPushButton::clicked, this, &LogicAnalyzer::startCapture);

    QPushButton* stop_btn = new QPushButton("Stop", this);
    connect(stop_btn, &QPushButton::clicked, this, &LogicAnalyzer::stopCapture);

    QPushButton* export_btn = new QPushButton("Export VCD", this);
    connect(export_btn, &QPushButton::clicked, this, []() {
        // TODO: Export
    });

    controls->addWidget(capture_btn);
    controls->addWidget(stop_btn);
    controls->addWidget(export_btn);

    // Timebase
    QLabel* time_lbl = new QLabel("Timebase:");
    timebase_combo_ = new QComboBox(this);
    timebase_combo_->addItems({"100 ns/div", "200 ns/div", "500 ns/div", "1 µs/div",
                               "2 µs/div", "5 µs/div", "10 µs/div", "20 µs/div",
                               "50 µs/div", "100 µs/div", "200 µs/div", "500 µs/div",
                               "1 ms/div", "2 ms/div", "5 ms/div", "10 ms/div"});
    timebase_combo_->setCurrentIndex(5);  // 10 µs default
    connect(timebase_combo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LogicAnalyzer::onTimebaseChanged);

    controls->addWidget(time_lbl);
    controls->addWidget(timebase_combo_);

    layout->addLayout(controls);
}

void LogicAnalyzer::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(rect(), QColor(30, 30, 30));

    drawBackground(&painter);
    drawWaveforms(&painter);
    drawGrid(&painter);
    drawCursors(&painter);
    drawChannelLabels(&painter);
    drawAnnotations(&painter);

    QWidget::paintEvent(event);
}

void LogicAnalyzer::drawBackground(QPainter* painter) {
    // Dark background
    painter->fillRect(rect(), QColor(40, 40, 40));
}

void LogicAnalyzer::drawGrid(QPainter* painter) {
    painter->setPen(QPen(QColor(80, 80, 80), 0.5));

    // Vertical grid (time)
    int x_step = static_cast<int>(horizontal_scale_ * 10 * 1000);  // 10 divisions
    for (int x = 0; x < width(); x += x_step) {
        painter->drawLine(x, 0, x, height());
    }

    // Horizontal grid (channels)
    for (int ch = 0; ch <= settings_.num_channels; ch++) {
        int y = ch * vertical_scale_;
        painter->drawLine(0, y, width(), y);
    }
}

void LogicAnalyzer::drawWaveforms(QPainter* painter) {
    if (waveform_buffer_.empty()) return;

    for (size_t ch = 0; ch < channels_.size(); ch++) {
        QColor color = QColor::fromHsv(static_cast<int>(ch * 30) % 360, 255, 200);
        painter->setPen(QPen(color, 2));

        int base_y = static_cast<int>(ch * vertical_scale_ + vertical_scale_ / 2);
        bool last_level = false;
        int last_x = -1;

        for (const auto& sample : waveform_buffer_) {
            if (ch >= sample.levels.size()) break;

            bool level = sample.levels[ch];
            int x = timeToX(sample.timestamp);

            if (last_x >= 0) {
                // Draw line segment
                int y1 = base_y + (last_level ? -5 : 5);
                int y2 = base_y + (level ? -5 : 5);
                painter->drawLine(last_x, y1, x, y2);
            }

            last_level = level;
            last_x = x;
        }
    }
}

void LogicAnalyzer::drawChannelLabels(QPainter* painter) {
    painter->setPen(Qt::white);
    QFont font = painter->font();
    font.setPointSize(8);
    painter->setFont(font);

    for (size_t i = 0; i < channels_.size(); i++) {
        int y = static_cast<int>(i * vertical_scale_);
        painter->drawText(5, y + 15, QString("GPIO%1").arg(channels_[i]));
    }
}

void LogicAnalyzer::drawCursors(QPainter* painter) {
    if (!show_cursors_) return;

    painter->setPen(QPen(Qt::yellow, 1, Qt::DashLine));

    // Cursor 1
    painter->drawLine(cursor1_pos_.x(), 0, cursor1_pos_.x(), height());
    painter->drawText(cursor1_pos_.x() + 2, 20, "C1");

    // Cursor 2
    painter->drawLine(cursor2_pos_.x(), 0, cursor2_pos_.x(), height());
    painter->drawText(cursor2_pos_.x() + 2, 40, "C2");

    // Measurement display
    int dx = cursor2_pos_.x() - cursor1_pos_.x();
    if (dx != 0) {
        double time_s = dx / (horizontal_scale_ * 1000.0);  // ns
        QString time_str = QString("Δt = %1 ns").arg(static_cast<uint64_t>(dx));
        painter->drawText(10, height() - 30, time_str);
    }
}

void LogicAnalyzer::drawAnnotations(QPainter* painter) {
    for (const auto& ann : annotations_) {
        int x = timeToX(ann.timestamp);
        painter->setPen(QPen(ann.color, 1, Qt::DashLine));
        painter->drawLine(x, 0, x, height());
        painter->drawText(x + 2, 20, QString::fromStdString(ann.label));
    }
}

void LogicAnalyzer::updateFromEngine() {
    if (!engine_ || !engine_->isRunning()) return;

    // Capture current GPIO states
    uint64_t current_time = engine_->simulationTime();
    WaveformSample sample(current_time, channels_.size());

    for (size_t i = 0; i < channels_.size(); i++) {
        uint8_t pin = channels_[i];
        GPIOLevel level = gpio_->getLevel(pin);
        sample.levels[i] = (level == GPIOLevel::HIGH);
    }

    waveform_buffer_.push_back(sample);

    // Trim buffer to max size
    while (waveform_buffer_.size() > settings_.buffer_size) {
        waveform_buffer_.pop_front();
    }

    update();
}

void LogicAnalyzer::startCapture() {
    capturing_ = true;
    trigger_armed_ = true;
    waveform_buffer_.clear();
    LOG_INFO("LogicAnalyzer capture started");
}

void LogicAnalyzer::stopCapture() {
    capturing_ = false;
    LOG_INFO("LogicAnalyzer capture stopped");
}

bool LogicAnalyzer::exportToVCD(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        LOG_ERROR("Failed to open VCD file: {}", filename);
        return false;
    }

    file << "$date\n    " << __DATE__ << "\n$end\n";
    file << "$version\n    ESP32 Simulator VCD Export\n$end\n";
    file << "$timescale 1ns $end\n";

    // Define signals
    file << "$scope module logic_analyzer $end\n";
    for (uint8_t pin : channels_) {
        file << "$var wire 1 S" << pin << " GPIO" << pin << " $end\n";
    }
    file << "$upscope $end\n";
    file << "$enddefinitions $end\n";

    // Dump values
    uint64_t last_time = 0;
    for (const auto& sample : waveform_buffer_) {
        uint64_t delta = sample.timestamp - last_time;
        file << "#" << delta << "\n";
        for (size_t i = 0; i < channels_.size(); i++) {
            file << (sample.levels[i] ? "1" : "0") << "S" << channels_[i] << "\n";
        }
        last_time = sample.timestamp;
    }

    file.close();
    LOG_INFO("Exported VCD to {}", filename);
    return true;
}

void LogicAnalyzer::onAutoScrollToggled(bool checked) {
    auto_scroll_ = checked;
}

void LogicAnalyzer::onTimebaseChanged(int index) {
    double scale_values[] = {100, 200, 500, 1000, 2000, 5000, 10000,
                             20000, 50000, 100000, 200000, 500000,
                             1000000, 2000000, 5000000, 10000000};
    horizontal_scale_ = scale_values[index] / 1000.0;  // ns to µs
    update();
}

void LogicAnalyzer::setSettings(const CaptureSettings& settings) {
    settings_ = settings;
    channels_.resize(settings.num_channels);
}

} // namespace esp32sim
