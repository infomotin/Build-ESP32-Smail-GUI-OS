/**
 * @file button_component.h
 * @brief Push button virtual component
 */

#pragma once

#include "component_base.h"

namespace esp32sim {

/**
 * @class ButtonComponent
 * @brief A push button with configurable active level
 */
class ButtonComponent : public VirtualComponent {
public:
    explicit ButtonComponent(QObject* parent = nullptr);
    ~ButtonComponent() override = default;

    ComponentType type() const override { return ComponentType::BUTTON; }

    QRectF boundingRect() const override;
    void paint(QPainter* painter) override;
    void mousePressEvent(QPointF pos);
    void mouseReleaseEvent(QPointF pos);
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
     * @brief Check if button is currently pressed
     */
    bool isPressed() const { return pressed_; }

    /**
     * @brief Get active level (true = active high)
     */
    bool activeHigh() const { return active_high_; }

    /**
     * @brief Set active level
     */
    void setActiveHigh(bool active_high) { active_high_ = active_high; emitChanged(); }

private:
    QPointF position_;
    bool pressed_ = false;
    bool active_high_ = true;  // Default: active high
    int connected_pin_ = -1;

    // Debounce simulation
    static constexpr int DEBOUNCE_TICKS = 5;
    int debounce_counter_ = 0;
    bool last_physical_state_ = false;

    static constexpr float BUTTON_WIDTH = 30.0f;
    static constexpr float BUTTON_HEIGHT = 20.0f;
};

/**
 * @class ToggleSwitchComponent
 * @brief A toggle switch (on/off)
 */
class ToggleSwitchComponent : public VirtualComponent {
public:
    explicit ToggleSwitchComponent(QObject* parent = nullptr);
    ~ToggleSwitchComponent() override = default;

    ComponentType type() const override { return ComponentType::TOGGLE_SWITCH; }

    QRectF boundingRect() const override;
    void paint(QPainter* painter) override;
    void mousePressEvent(QPointF pos);
    void mouseReleaseEvent(QPointF pos);
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
     * @brief Get switch position
     */
    bool isOn() const { return on_; }

    /**
     * @brief Set switch position
     */
    void setOn(bool on) { on_ = on; emitChanged(); }

    /**
     * @brief Get active level
     */
    bool activeHigh() const { return active_high_; }

    /**
     * @brief Set active level
     */
    void setActiveHigh(bool active_high) { active_high_ = active_high; emitChanged(); }

private:
    QPointF position_;
    bool on_ = false;
    bool active_high_ = true;
    int connected_pin_ = -1;

    static constexpr float SWITCH_WIDTH = 40.0f;
    static constexpr float SWITCH_HEIGHT = 20.0f;
};

} // namespace esp32sim
