/**
 * @file button_component.cpp
 * @brief Button virtual component implementation
 */

#include "virtual_components/button_component.h"
#include <QPainter>

namespace esp32sim {

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
    return "BUTTON:" + name_ + ":" + std::to_string(connected_pin_) +
           ":" + (active_high_ ? "1" : "0");
}

void ButtonComponent::deserialize(const std::string& data) {
    auto parts = split(data, ':');
    if (parts.size() >= 4) {
        setName(parts[1]);
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
        "\"name\":\"" + name_ + "\","
        "\"pin\":" + std::to_string(connected_pin_) + ","
        "\"activeHigh\":" + (active_high_ ? "true" : "false") +
        "}";
}

void ButtonComponent::fromJSON(const std::string&) {
    // TODO: Parse and restore
}

void ButtonComponent::setPressed(bool pressed) {
    pressed_ = pressed;
}

std::vector<std::string> ButtonComponent::split(const std::string& s, char delim) {
    std::vector<std::string> parts;
    size_t start = 0, end = 0;
    while ((end = s.find(delim, start)) != std::string::npos) {
        parts.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    parts.push_back(s.substr(start));
    return parts;
}

} // namespace esp32sim
