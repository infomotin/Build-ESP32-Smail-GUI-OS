/**
 * @file button_component.cpp
 * @brief Button virtual component implementation
 */

#include "virtual_components/button_component.h"
#include <QPainter>
#include <QString>

namespace esp32sim {

std::vector<std::string> split(const std::string& s, char delim);

ButtonComponent::ButtonComponent(QObject* parent)
    : VirtualComponent("Button", parent) {
}

QRectF ButtonComponent::boundingRect() const {
    return QRectF(0, 0, 40, 30);
}

void ButtonComponent::paint(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    // Button background
    QColor bg_color = pressed_ ? QColor(100, 100, 200) : QColor(220, 220, 220);
    painter->fillRect(boundingRect(), bg_color);

    // Border
    painter->setPen(QPen(Qt::black, 2));
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(boundingRect());

    // Label
    painter->setPen(Qt::black);
    painter->drawText(boundingRect(), Qt::AlignCenter, "BTN");

    // Draw connection dot if connected
    if (connected_pin_ >= 0) {
        painter->setPen(Qt::black);
        painter->drawText(QRect(0, 28, 40, 10), Qt::AlignCenter,
                          QString("GPIO%1").arg(connected_pin_));
    }
}

void ButtonComponent::mousePressEvent(QPointF pos) {
    setPressed(true);
    // Drive the pin HIGH or LOW depending on active_high
    if (connected_pin_ >= 0) {
        // Emit GPIO change signal
        onGPIOChanged(connected_pin_, active_high_);
    }
    emitChanged();
}

void ButtonComponent::mouseReleaseEvent(QPointF pos) {
    setPressed(false);
    if (connected_pin_ >= 0) {
        onGPIOChanged(connected_pin_, !active_high_);
    }
    emitChanged();
}

void ButtonComponent::mouseDoubleClickEvent(QPointF pos) {
    setActiveHigh(!active_high_);
    emitChanged();
}

std::vector<QPointF> ButtonComponent::connectionPoints() const {
    return { QPointF(40, 15) };
}

int ButtonComponent::pinForConnectionPoint(int index) const {
    return 0;
}

void ButtonComponent::connectPin(int pin, int) {
    connected_pin_ = pin;
    emitConnectionChanged(pin, true);
    emitChanged();
}

void ButtonComponent::disconnectPin(int pin) {
    if (connected_pin_ == pin) {
        connected_pin_ = -1;
        emitConnectionChanged(pin, false);
        emitChanged();
    }
}

bool ButtonComponent::isPinConnected(int pin) const {
    return connected_pin_ == pin;
}

std::string ButtonComponent::serialize() const {
    return "BUTTON:" + name_.toStdString() + ":" + std::to_string(connected_pin_) +
           ":" + (active_high_ ? "1" : "0");
}

void ButtonComponent::deserialize(const std::string& data) {
    auto parts = split(data, ':');
    if (parts.size() >= 4) {
        setName(QString::fromStdString(parts[1]));
        connected_pin_ = std::stoi(parts[2]);
        setActiveHigh(parts[3] == "1");
    }
}

void ButtonComponent::onGPIOChanged(int pin, bool level) {
    // This would normally update the firmware's GPIO state via Simulator Engine
}

std::string ButtonComponent::toJSON() const {
    return "{"
        "\"type\":\"BUTTON\","
        "\"name\":\"" + name_.toStdString() + "\","
        "\"pin\":" + std::to_string(connected_pin_) + ","
        "\"activeHigh\":" + (active_high_ ? "true" : "false") +
        "}";
}

void ButtonComponent::fromJSON(const std::string&) {
    // TODO: Parse and restore
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = s.find(delim, start)) != std::string::npos) {
        parts.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(s.substr(start));
    return parts;
}

// ToggleSwitchComponent implementation
ToggleSwitchComponent::ToggleSwitchComponent(QObject* parent)
    : VirtualComponent("ToggleSwitch", parent), on_(false), active_high_(true), connected_pin_(-1) {
}

QRectF ToggleSwitchComponent::boundingRect() const {
    return QRectF(0, 0, 40, 20);
}

void ToggleSwitchComponent::paint(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);
    QRectF r = boundingRect();
    QColor bg = on_ ? QColor(100, 200, 100) : QColor(200, 200, 200);
    painter->fillRect(r, bg);
    painter->setPen(QPen(Qt::black, 1));
    painter->drawRect(r);
    painter->drawText(r, Qt::AlignCenter, on_ ? "ON" : "OFF");

    if (connected_pin_ >= 0) {
        painter->drawText(QRect(0, 18, 40, 10), Qt::AlignCenter, QString("GPIO%1").arg(connected_pin_));
    }
}

void ToggleSwitchComponent::mousePressEvent(QPointF pos) {
    setOn(!on_);
    if (connected_pin_ >= 0) {
        onGPIOChanged(connected_pin_, on_ ? active_high_ : !active_high_);
    }
}

void ToggleSwitchComponent::mouseReleaseEvent(QPointF pos) {}

void ToggleSwitchComponent::mouseDoubleClickEvent(QPointF pos) {
    setActiveHigh(!activeHigh());
}

std::vector<QPointF> ToggleSwitchComponent::connectionPoints() const {
    return { QPointF(40, 10) };
}

int ToggleSwitchComponent::pinForConnectionPoint(int index) const {
    return 0;
}

void ToggleSwitchComponent::connectPin(int pin, int) {
    connected_pin_ = pin;
    emitConnectionChanged(pin, true);
    emitChanged();
}

void ToggleSwitchComponent::disconnectPin(int pin) {
    if (connected_pin_ == pin) {
        connected_pin_ = -1;
        emitConnectionChanged(pin, false);
        emitChanged();
    }
}

bool ToggleSwitchComponent::isPinConnected(int pin) const {
    return connected_pin_ == pin;
}

void ToggleSwitchComponent::onGPIOChanged(int pin, bool level) {
    // Optional: Update internal state based on external driver
    // For now, no-op
}

std::string ToggleSwitchComponent::serialize() const {
    return "TOGGLE:" + name_.toStdString() + ":" + std::to_string(connected_pin_) + ":" + (on_ ? "1" : "0");
}

void ToggleSwitchComponent::deserialize(const std::string& data) {
    auto parts = split(data, ':');
    if (parts.size() >= 4) {
        setName(QString::fromStdString(parts[1]));
        connected_pin_ = std::stoi(parts[2]);
        setOn(parts[3] == "1");
    }
}

std::string ToggleSwitchComponent::toJSON() const {
    return "{"
           "\"type\":\"TOGGLE\","
           "\"name\":\"" + name_.toStdString() + "\","
           "\"pin\":" + std::to_string(connected_pin_) + ","
           "\"on\":" + (on_ ? "true" : "false") +
           "}";
}

void ToggleSwitchComponent::fromJSON(const std::string& /*json*/) {
    // TODO: Parse JSON
}

} // namespace esp32sim
