/**
 * @file led_component.h
 * @brief Single-color LED virtual component
 */

#pragma once

#include "component_base.h"
#include <QColor>

namespace esp32sim {

/**
 * @class LEDComponent
 * @brief A simple LED component with configurable color
 */
class LEDComponent : public VirtualComponent {
public:
    explicit LEDComponent(QObject* parent = nullptr);
    ~LEDComponent() override = default;

    ComponentType type() const override { return ComponentType::LED_SINGLE; }

    QRectF boundingRect() const override;
    void paint(QPainter* painter) override;
    void mousePressEvent(QPointF pos) {}
    void mouseReleaseEvent(QPointF pos) {}
    void mouseMoveEvent(QPointF pos) {}
    void mouseDoubleClickEvent(QPointF pos);

    std::vector<QPointF> connectionPoints() const override;
    int pinForConnectionPoint(int index) const override;
    void connectPin(int pin, int connection_point) override;
    void disconnectPin(int pin) override;
    bool isPinConnected(int pin) const override;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    void onGPIOChanged(int pin, bool level) override;

    std::string toJSON() const override;
    void fromJSON(const std::string& json) override;

    QPointF position() const override { return position_; }
    void setPosition(const QPointF& pos) override { position_ = pos; emitChanged(); }

    /**
     * @brief Get LED color
     */
    QColor color() const { return color_; }

    /**
     * @brief Set LED color
     */
    void setColor(const QColor& color) { color_ = color; emitChanged(); }

    /**
     * @brief Get whether LED is lit
     */
    bool isLit() const { return is_lit_; }

    /**
     * @brief Set LED state (for manual control)
     */
    void setLit(bool lit) { is_lit_ = lit; emitChanged(); }

private:
    QPointF position_;
    QColor color_ = QColor(Qt::green);
    bool is_lit_ = false;
    int connected_pin_ = -1;

    // Visual properties
    static constexpr float LED_SIZE = 20.0f;
    static constexpr float PIN_SPACING = 10.0f;
};

/**
 * @class RGBLEDComponent
 * @brief RGB LED with three color channels
 */
class RGBLEDComponent : public VirtualComponent {
public:
    explicit RGBLEDComponent(QObject* parent = nullptr);
    ~RGBLEDComponent() override = default;

    ComponentType type() const override { return ComponentType::LED_RGB; }

    QRectF boundingRect() const override;
    void paint(QPainter* painter) override;
    void mousePressEvent(QPointF pos) {}
    void mouseReleaseEvent(QPointF pos) {}
    void mouseMoveEvent(QPointF pos) {}
    void mouseDoubleClickEvent(QPointF pos);

    std::vector<QPointF> connectionPoints() const override;
    int pinForConnectionPoint(int index) const override;
    void connectPin(int pin, int connection_point) override;
    void disconnectPin(int pin) override;
    bool isPinConnected(int pin) const override;

    std::string serialize() const override;
    void deserialize(const std::string& data) override;

    void onGPIOChanged(int pin, bool level) override;

    std::string toJSON() const override;
    void fromJSON(const std::string& json) override;

    QPointF position() const override { return position_; }
    void setPosition(const QPointF& pos) override { position_ = pos; emitChanged(); }

    /**
     * @brief Get color components
     */
    QColor color() const { return color_; }
    void setColor(const QColor& color) { color_ = color; emitChanged(); }

    /**
     * @brief Get red component state
     */
    bool isRedOn() const { return red_on_; }
    void setRed(bool on) { red_on_ = on; emitChanged(); }

    /**
     * @brief Get green component state
     */
    bool isGreenOn() const { return green_on_; }
    void setGreen(bool on) { green_on_ = on; emitChanged(); }

    /**
     * @brief Get blue component state
     */
    bool isBlueOn() const { return blue_on_; }
    void setBlue(bool on) { blue_on_ = on; emitChanged(); }

private:
    QPointF position_;
    QColor color_ = QColor(Qt::white);
    bool red_on_ = false;
    bool green_on_ = false;
    bool blue_on_ = false;

    // Pin connections: 0=R, 1=G, 2=B, 3=Common (optional)
    int red_pin_ = -1;
    int green_pin_ = -1;
    int blue_pin_ = -1;
    int common_pin_ = -1;

    static constexpr float LED_SIZE = 20.0f;
};

} // namespace esp32sim
