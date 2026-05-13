/**
 * @file pinout_panel.cpp
 * @brief Pinout panel implementation
 */

#include "gui/pinout_panel.h"
#include "simulation_engine.h"
#include "peripherals/gpio_controller.h"
#include "utils/logger.h"

#include <QPainter>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>
#include <QToolTip>
#include <QTimer>

namespace esp32sim {

PinoutPanel::PinoutPanel(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    LOG_DEBUG("Creating PinoutPanel");

    setMinimumSize(120, 400);
    setMouseTracking(true);

    initializePinPositions();

    // Update timer (60 Hz)
    update_timer_ = new QTimer(this);
    connect(update_timer_, &QTimer::timeout, this, &PinoutPanel::onUpdateTimer);
    update_timer_->start(16);  // ~60 Hz

    // Initial update
    updateFromEngine();
}

PinoutPanel::~PinoutPanel() = default;

void PinoutPanel::initializePinPositions() {
    pin_positions_.clear();

    // ESP32-WROOM-32 pin layout (approximate)
    // Two rows of 19 pins each (38 total exposed pins)
    // Coordinates are normalized (0-1) then scaled

    constexpr float left_margin = 20.0f;
    constexpr float right_margin = 20.0f;
    constexpr float top_margin = 20.0f;
    constexpr float bottom_margin = 20.0f;
    constexpr float pin_spacing_x = 10.0f;
    constexpr float pin_spacing_y = 15.0f;
    constexpr float total_height = 500.0f;  // Virtual height

    // Top row (left to right, GPIO numbers 0-18)
    for (int i = 0; i <= 18; i++) {
        PinInfo info;
        info.number = i;
        info.position = QPointF(left_margin + i * pin_spacing_x, top_margin);
        info.top_side = true;
        info.left_side = true;
        info.label = QString("GPIO%1").arg(i);
        info.function = getPinFunction(i);
        pin_positions_.push_back(info);
    }

    // Bottom row (right to left, GPIO numbers 19-37)
    for (int i = 19; i <= 37; i++) {
        PinInfo info;
        info.number = i;
        info.position = QPointF(
            right_margin + (18 - (i - 19)) * pin_spacing_x,
            top_margin + (pin_spacing_y * 18)
        );
        info.top_side = false;
        info.left_side = false;
        info.label = QString("GPIO%1").arg(i);
        info.function = getPinFunction(i);
        pin_positions_.push_back(info);
    }

    // Calculate required widget size
    float width = left_margin + 19 * pin_spacing_x + right_margin;
    float height = top_margin + pin_spacing_y * 18 + bottom_margin;
    setFixedSize(static_cast<int>(width), static_cast<int>(height));
}

QString PinoutPanel::getPinFunction(int pin) const {
    // Return typical function for this pin
    switch (pin) {
        case 0: return "GPIO0 / BOOT";
        case 1: return "GPIO1 / TX0";
        case 2: return "GPIO2 / RX0";
        case 3: return "GPIO3 / TX1";
        case 4: return "GPIO4";
        case 5: return "GPIO5";
        case 6: return "GPIO6 / SCK";
        case 7: return "GPIO7 / MISO";
        case 8: return "GPIO8 / MOSI";
        case 9: return "GPIO9 / HD";
        case 10: return "GPIO10 / WP";
        case 11: return "GPIO11 / SDA0";
        case 12: return "GPIO12 / SCLK";
        case 13: return "GPIO13 / MISO";
        case 14: return "GPIO14 / MOSI";
        case 15: return "GPIO15 / HD";
        case 16: return "GPIO16";
        case 17: return "GPIO17";
        case 18: return "GPIO18";
        case 19: return "GPIO19";
        case 20: return "GPIO20";
        case 21: return "GPIO21";
        case 22: return "GPIO22";
        case 23: return "GPIO23";
        case 24: return "GPIO24";
        case 25: return "GPIO25 / DAC1";
        case 26: return "GPIO26 / DAC2";
        case 27: return "GPIO27";
        case 28: return "GPIO28";
        case 29: return "GPIO29";
        case 30: return "GPIO30";
        case 31: return "GPIO31";
        case 32: return "GPIO32";
        case 33: return "GPIO33";
        case 34: return "GPIO34 / ADC1 (IN)";
        case 35: return "GPIO35 / ADC1 (IN)";
        case 36: return "GPIO36 / ADC1 (IN)";
        case 37: return "GPIO37 / ADC1 (IN)";
        default: return "GPIO";
    }
}

void PinoutPanel::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Background
    painter.fillRect(rect(), QColor(250, 250, 250));

    // Draw module outline
    QRect module_rect(10, 10, width() - 20, height() - 20);
    painter.setPen(QPen(Qt::black, 2));
    painter.setBrush(QColor(100, 150, 200, 100));
    painter.drawRoundedRect(module_rect, 5, 5);

    // Draw all pins
    for (const auto& pin : pin_positions_) {
        drawPin(&painter, pin);
    }

    // Draw hover highlight
    if (hovered_pin_ >= 0) {
        auto it = std::find_if(pin_positions_.begin(), pin_positions_.end(),
                               [this](const PinInfo& p) { return p.number == hovered_pin_; });
        if (it != pin_positions_.end()) {
            painter.setPen(QPen(Qt::yellow, 3));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(it->position, 8, 8);
        }
    }
}

void PinoutPanel::drawPin(QPainter* painter, const PinInfo& pin) {
    QColor pin_color = pinColor(pins_[pin.number]);
    bool input_only = isInputOnlyPin(pin.number);

    // Draw pin dot
    painter->setPen(QPen(input_only ? Qt::blue : Qt::black, input_only ? 2 : 1));
    painter->setBrush(pin_color);
    painter->drawEllipse(pin.position, 5, 5);

    // Draw pin number
    painter->setPen(Qt::black);
    QFont font = painter->font();
    font.setPointSize(7);
    painter->setFont(font);

    if (pin.top_side) {
        painter->drawText(QRectF(pin.position.x() - 15, pin.position.y() + 8, 30, 10),
                         Qt::AlignCenter, pin.label);
    } else {
        painter->drawText(QRectF(pin.position.x() - 15, pin.position.y() - 18, 30, 10),
                         Qt::AlignCenter, pin.label);
    }
}

QColor PinoutPanel::pinColor(const GPIOPinState& pin) const {
    switch (pin.mode) {
        case GPIOMode::OUTPUT:
            return pin.level == GPIOLevel::HIGH ? QColor(0, 200, 0) : QColor(80, 80, 80);
        case GPIOMode::ADC_INPUT:
            return QColor(0, 150, 255);
        case GPIOMode::PWM_OUTPUT:
            return QColor(255, 165, 0);
        case GPIOMode::INPUT_PULLUP:
        case GPIOMode::INPUT_PULLDOWN:
            return QColor(200, 200, 200);
        default:
            return QColor(255, 255, 255);
    }
}

void PinoutPanel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        int pin = pinAtPosition(event->pos());
        if (pin >= 0) {
            selected_pin_ = pin;
            update();
            Q_EMIT pinClicked(pin, event->globalPos());
        }
    }
    QWidget::mousePressEvent(event);
}

void PinoutPanel::mouseDoubleClickEvent(QMouseEvent* event) {
    int pin = pinAtPosition(event->pos());
    if (pin >= 0) {
        // Toggle pin state if it's an output
        // For now just show info
    }
    QWidget::mouseDoubleClickEvent(event);
}

void PinoutPanel::contextMenuEvent(QContextMenuEvent* event) {
    int pin = pinAtPosition(event->pos());
    if (pin >= 0) {
        Q_EMIT pinContextMenuRequested(pin, event->globalPos());
    }
}

void PinoutPanel::enterEvent(QEvent* /*event*/) {
    setCursor(Qt::PointingHandCursor);
}

void PinoutPanel::leaveEvent(QEvent* /*event*/) {
    hovered_pin_ = -1;
    update();
}

int PinoutPanel::pinAtPosition(const QPointF& pos) const {
    for (const auto& pin : pin_positions_) {
        QPointF p = pin.position;
        float dist = std::sqrt(std::pow(pos.x() - p.x(), 2) + std::pow(pos.y() - p.y(), 2));
        if (dist <= 8.0f) {
            return pin.number;
        }
    }
    return -1;
}

void PinoutPanel::showPinContextMenu(int pin, const QPoint& global_pos) {
    QMenu menu(this);

    QAction* drive_high = menu.addAction("Drive HIGH");
    QAction* drive_low = menu.addAction("Drive LOW");
    menu.addSeparator();
    QAction* connect_comp = menu.addAction("Connect Component...");
    QAction* view_waveform = menu.addAction("View Waveform");

    QAction* chosen = menu.exec(global_pos);
    if (chosen == drive_high) {
        drivePinManually(pin, true);
    } else if (chosen == drive_low) {
        drivePinManually(pin, false);
    } else if (chosen == view_waveform) {
        openWaveformForPin(pin);
    }
}

void PinoutPanel::drivePinManually(int pin, bool high) {
    if (isInputOnlyPin(pin)) {
        QToolTip::showText(mapToGlobal(QPoint(0, 0)),
                           QString("GPIO%1 is input-only").arg(pin));
        return;
    }

    // Send signal to engine to drive pin
    Q_EMIT pinStateChanged(pin, high);
}

void PinoutPanel::openWaveformForPin(int pin) {
    // Focus logic analyzer on this pin
    // TODO: Implementation
}

void PinoutPanel::updateFromEngine() {
    if (!engine_) return;

    // Get GPIO state
    auto gpio = engine_->gpio();
    if (!gpio) return;

    // Update pin colors cache
    auto all_pins = gpio->getAllPins();
    pins_ = all_pins;

    update();
}

void PinoutPanel::refresh() {
    updateFromEngine();
}

void PinoutPanel::onUpdateTimer() {
    updateFromEngine();
}

void PinoutPanel::highlightPin(int pin, bool highlight) {
    if (highlight) {
        hovered_pin_ = pin;
    } else {
        hovered_pin_ = -1;
    }
    update();
}

} // namespace esp32sim
