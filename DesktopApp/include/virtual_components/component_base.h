/**
 * @file component_base.h
 * @brief Base class for all virtual components
 */

#pragma once

#include <QObject>
#include <QRectF>
#include <QPainter>
#include <QPointF>
#include <string>
#include <vector>
#include <functional>

namespace esp32sim {

/**
 * @class VirtualComponent
 * @brief Base class for all drag-and-drop virtual components
 */
class VirtualComponent : public QObject {
    Q_OBJECT

public:
    explicit VirtualComponent(const std::string& name, QObject* parent = nullptr);
    virtual ~VirtualComponent();

    /**
     * @brief Component type identifier
     */
    enum ComponentType {
        LED_SINGLE,
        LED_RGB,
        LCD_16x2,
        OLED_128x64,
        BUTTON,
        TOGGLE_SWITCH,
        NTC_THERMISTOR,
        LDR_SENSOR,
        HC_SR04,
        DHT22,
        UNKNOWN
    };

    virtual ComponentType type() const = 0;

    /**
     * @brief Get component name
     */
    const std::string& name() const { return name_; }

    /**
     * @brief Set component name
     */
    void setName(const std::string& name) { name_ = name; }

    /**
     * @brief Get component position on workspace
     */
    QPointF position() const { return position_; }

    /**
     * @brief Set component position
     */
    void setPosition(const QPointF& pos) { position_ = pos; }

    /**
     * @brief Get component bounding rectangle
     */
    virtual QRectF boundingRect() const = 0;

    /**
     * @brief Paint the component
     */
    virtual void paint(QPainter* painter) = 0;

    /**
     * @brief Handle mouse press
     */
    virtual void mousePressEvent(QPointF pos) = 0;

    /**
     * @brief Handle mouse release
     */
    virtual void mouseReleaseEvent(QPointF pos) = 0;

    /**
     * @brief Handle mouse move
     */
    virtual void mouseMoveEvent(QPointF pos) = 0;

    /**
     * @brief Double click for properties
     */
    virtual void mouseDoubleClickEvent(QPointF pos) = 0;

    /**
     * @brief Get connection points (pins)
     */
    virtual std::vector<QPointF> connectionPoints() const = 0;

    /**
     * @brief Get pin number for a connection point index
     */
    virtual int pinForConnectionPoint(int index) const = 0;

    /**
     * @brief Connect a GPIO pin to this component
     */
    virtual void connectPin(int pin, int connection_point) = 0;

    /**
     * @brief Disconnect a GPIO pin
     */
    virtual void disconnectPin(int pin) = 0;

    /**
     * @brief Check if a pin is connected
     */
    virtual bool isPinConnected(int pin) const = 0;

    /**
     * @brief Get component state for serialization
     */
    virtual std::string serialize() const = 0;

    /**
     * @brief Restore component state from serialization
     */
    virtual void deserialize(const std::string& data) = 0;

    /**
     * @brief Called when connected GPIO changes
     */
    virtual void onGPIOChanged(int pin, bool level) = 0;

    /**
     * @brief Get JSON configuration
     */
    virtual std::string toJSON() const = 0;

    /**
     * @brief Load from JSON
     */
    virtual void fromJSON(const std::string& json) = 0;

signals:
    /**
     * @brief Component state changed (for UI updates)
     */
    void componentChanged();

    /**
     * @brief Connection changed
     */
    void connectionChanged(int pin, bool connected);

protected:
    std::string name_;
    QPointF position_;
    std::map<int, int> pin_connections_;  // gpio_pin -> connection_point

    /**
     * @brief Notify the workspace that this component changed
     */
    void emitChanged();

    /**
     * @brief Notify connection change
     */
    void emitConnectionChanged(int pin, bool connected);
};

/**
 * @class ComponentLibrary
 * @brief Factory for creating virtual components
 */
class ComponentLibrary : public QObject {
    Q_OBJECT

public:
    static ComponentLibrary* instance();

    /**
     * @brief Create a component by type
     */
    static VirtualComponent* createComponent(VirtualComponent::ComponentType type);

    /**
     * @brief Get list of available component types
     */
    static std::vector<std::pair<VirtualComponent::ComponentType, std::string>> availableComponents();

    /**
     * @brief Get component display name
     */
    static std::string componentDisplayName(VirtualComponent::ComponentType type);
};

} // namespace esp32sim
