/**
 * @file led_component.cpp
 * @brief LED virtual component implementation
 */

#include "virtual_components/led_component.h"
#include <QPainter>
#include <QPainterPath>

namespace esp32sim {

std::vector<std::string> split(const std::string& s, char delim);

LEDComponent::LEDComponent(QObject* parent)
    : VirtualComponent("LED", parent) {
}

QRectF LEDComponent::boundingRect() const {
    return QRectF(0, 0, 40, 30);
}

void LEDComponent::paint(QPainter* painter) {
    painter->setRenderHint(QPainter::Antialiasing);

    // Draw background
    painter->fillRect(boundingRect(), QColor(240, 240, 240));

    // Calculate LED center
    QPointF center(20, 15);

    // LED body
    QPainterPath led_shape;
    led_shape.addEllipse(center, 12, 12);
    painter->setPen(Qt::NoPen);

    // LED color based on state
    if (is_lit_) {
        QColor lit_color = color_;
        lit_color.setAlpha(255);
        painter->setBrush(lit_color);

        // Glow effect
        QColor glow = color_;
        glow.setAlpha(100);
        painter->setBrush(glow);
        painter->drawEllipse(center, 18, 18);

        painter->setBrush(lit_color);
        painter->drawEllipse(center, 12, 12);

        // Highlight
        painter->setBrush(QColor(255, 255, 255, 150));
        painter->drawEllipse(QPointF(center.x() - 4, center.y() - 4), 4, 4);
    } else {
        // Off state - dark gray
        painter->setBrush(QColor(80, 80, 80));
        painter->drawEllipse(center, 12, 12);

        // Subtle outline
        painter->setPen(QColor(120, 120, 120));
        painter->setBrush(Qt::NoBrush);
        painter->drawEllipse(center, 12, 12);
    }

    // Pin connections
    if (connected_pin_ >= 0) {
        painter->setPen(QPen(Qt::black, 1));
        painter->drawLine(32, 15, 40, 15);
        painter->drawText(35, 12, QString("GPIO%1").arg(connected_pin_));
    }

    // Label
    painter->setPen(Qt::black);
    painter->drawText(QRectF(0, 28, 40, 10), Qt::AlignCenter, name_);
}

void LEDComponent::mouseDoubleClickEvent(QPointF pos) {
    // Open properties dialog - for now toggle state
    setLit(!is_lit_);
    emitChanged();
}

std::vector<QPointF> LEDComponent::connectionPoints() const {
    return { QPointF(40, 15) };  // Right side connection
}

int LEDComponent::pinForConnectionPoint(int index) const {
    return 0;  // Single pin
}

void LEDComponent::connectPin(int pin, int /*connection_point*/) {
    connected_pin_ = pin;
    emitConnectionChanged(pin, true);
    emitChanged();
}

void LEDComponent::disconnectPin(int pin) {
    if (connected_pin_ == pin) {
        connected_pin_ = -1;
        emitConnectionChanged(pin, false);
        emitChanged();
    }
}

bool LEDComponent::isPinConnected(int pin) const {
    return connected_pin_ == pin;
}

std::string LEDComponent::serialize() const {
    // Simple serialization
    return "LED:" + name_.toStdString() + ":" + std::to_string(connected_pin_) + ":" +
           (is_lit_ ? "1" : "0") + ":" + color_.name().toStdString();
}

void LEDComponent::deserialize(const std::string& data) {
    // Parse serialized data
    // Format: LED:name:pin:state:color
    auto parts = split(data, ':');
    if (parts.size() >= 5) {
        setName(QString::fromStdString(parts[1]));
        connected_pin_ = std::stoi(parts[2]);
        setLit(parts[3] == "1");
        setColor(QColor(QString::fromStdString(parts[4])));
    }
}

void LEDComponent::onGPIOChanged(int pin, bool level) {
    if (pin == connected_pin_) {
        setLit(level);
        emitChanged();
    }
}

std::string LEDComponent::toJSON() const {
    // Return JSON representation
    return "{"
        "\"type\":\"LED\","
        "\"name\":\"" + name_.toStdString() + "\","
        "\"pin\":" + std::to_string(connected_pin_) + ","
        "\"lit\":" + (is_lit_ ? "true" : "false") + ","
        "\"color\":\"" + color_.name().toStdString() + "\""
        "}";
}

void LEDComponent::fromJSON(const std::string& json) {
    // Parse and restore
    // Simplified
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> result;
    size_t start = 0, end = 0;
    while ((end = s.find(delim, start)) != std::string::npos) {
        result.push_back(s.substr(start, end - start));
        start = end + 1;
    }
    result.push_back(s.substr(start));
    return result;
}

} // namespace esp32sim
