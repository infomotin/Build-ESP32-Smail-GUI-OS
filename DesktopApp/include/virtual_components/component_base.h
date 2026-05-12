/**
 * @file component_base.h
 * @brief Base class for all virtual components in the workspace
 */

#ifndef VIRTUAL_COMPONENTS_COMPONENT_BASE_H
#define VIRTUAL_COMPONENTS_COMPONENT_BASE_H

#include <QObject>
#include <QPainter>
#include <QRectF>
#include <string>
#include <vector>
#include <QPointF>

namespace esp32sim {

/**
 * @enum ComponentType
 * @brief Types of virtual components available
 */
enum class ComponentType {
    UNKNOWN = 0,
    LED_SINGLE,
    LED_RGB,
    BUTTON,
    TOGGLE_SWITCH,
    LCD_CHAR,
    OLED_GRAPHIC,
    NTC_THERMISTOR,
    LDR_SENSOR,
    HC_SR04,
    DHT22
};

/**
 * @class VirtualComponent
 * @brief Base class for all draggable GUI components in the workspace
 */
class VirtualComponent : public QObject {
    Q_OBJECT

public:
    explicit VirtualComponent(const QString& name, QObject* parent = nullptr);
    ~VirtualComponent() override = default;

    // Component identification
    virtual ComponentType type() const = 0;
    QString name() const { return name_; }
    void setName(const QString& name) { name_ = name; }

    // Geometry
    virtual QRectF boundingRect() const = 0;
    virtual void paint(QPainter* painter) = 0;

    // Position (stored in derived class, but accessed via virtual getter/setter)
    virtual QPointF position() const = 0;
    virtual void setPosition(const QPointF& pos) = 0;

    // Connection points (pins)
    virtual std::vector<QPointF> connectionPoints() const = 0;
    virtual int pinForConnectionPoint(int index) const = 0;
    virtual void connectPin(int pin, int connection_point) = 0;
    virtual void disconnectPin(int pin) = 0;
    virtual bool isPinConnected(int pin) const = 0;

    // GPIO event handling
    virtual void onGPIOChanged(int pin, bool level) = 0;

    // Serialization
    virtual std::string serialize() const = 0;
    virtual void deserialize(const std::string& data) = 0;

    // JSON (for saving/loading workspace)
    virtual std::string toJSON() const = 0;
    virtual void fromJSON(const std::string& json) = 0;

signals:
    void changed();  // Emitted when component property changes
    void connectionChanged(int pin, bool connected);  // Emitted when pin connection changes

protected:
    void emitChanged() { emit changed(); }
    void emitConnectionChanged(int pin, bool connected) { emit connectionChanged(pin, connected); }

    QString name_;
    std::vector<int> pin_connections_;
};

} // namespace esp32sim

#endif // VIRTUAL_COMPONENTS_COMPONENT_BASE_H

