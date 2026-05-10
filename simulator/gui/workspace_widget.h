#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QFrame>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>

#include "../peripherals/gpio/gpio_model.h"

// Virtual component base class
class VirtualComponent : public QFrame {
    Q_OBJECT

public:
    VirtualComponent(uint8_t pin, const QString& name, GpioModel* gpioModel, QWidget* parent = nullptr);
    virtual ~VirtualComponent() = default;
    
    uint8_t getPin() const { return connectedPin; }
    QString getName() const { return componentName; }
    virtual void updateState() = 0;

protected:
    virtual void mousePressEvent(QMouseEvent *event) override;
    
    uint8_t connectedPin;
    QString componentName;
    GpioModel* gpioModel;
};

// LED component
class LedComponent : public VirtualComponent {
    Q_OBJECT

public:
    LedComponent(uint8_t pin, const QString& name, const QColor& color, GpioModel* gpioModel, QWidget* parent = nullptr);
    
    void updateState() override;
    void setColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor ledColor;
    bool isOn;
};

// Button component
class ButtonComponent : public VirtualComponent {
    Q_OBJECT

public:
    ButtonComponent(uint8_t pin, const QString& name, GpioModel* gpioModel, QWidget* parent = nullptr);
    
    void updateState() override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    bool isPressed;
    bool isPressedByUser;
};

// Potentiometer component
class PotentiometerComponent : public VirtualComponent {
    Q_OBJECT

public:
    PotentiometerComponent(uint8_t pin, const QString& name, GpioModel* gpioModel, QWidget* parent = nullptr);
    
    void updateState() override;
    void setValue(uint16_t value);

signals:
    void valueChanged(uint16_t value);

private:
    QSlider* slider;
    QLabel* valueLabel;
    uint16_t currentValue;
};

// Workspace widget
class WorkspaceWidget : public QWidget {
    Q_OBJECT

public:
    explicit WorkspaceWidget(GpioModel* gpioModel, QWidget *parent = nullptr);
    
    void addComponent(uint8_t pin, const QString& type, const QString& name);
    void removeComponent(uint8_t pin);
    void clearAllComponents();
    void updateAllComponents();
    void handleGpioEvent(const class GpioEvent& event);

signals:
    void componentAdded(uint8_t pin, const QString& type);
    void componentRemoved(uint8_t pin);

private slots:
    void onAddLed();
    void onAddButton();
    void onAddPotentiometer();
    void onComponentSelected();

private:
    void createLayout();
    void createComponentPalette();
    void createWorkspace();
    void updateComponentList();
    
    GpioModel* gpioModel;
    
    QScrollArea* workspaceScrollArea;
    QWidget* workspaceWidget;
    QGridLayout* workspaceLayout;
    
    QVBoxLayout* mainLayout;
    QHBoxLayout* contentLayout;
    QVBoxLayout* paletteLayout;
    
    QGroupBox* componentPalette;
    QToolBox* componentToolBox;
    
    QPushButton* addLedButton;
    QPushButton* addButtonButton;
    QPushButton* addPotButton;
    
    QGroupBox* componentList;
    QListWidget* componentListWidget;
    
    std::vector<std::unique_ptr<VirtualComponent>> components;
    std::map<uint8_t, VirtualComponent*> pinToComponent;
    
    uint8_t nextPin;
    uint8_t getAvailablePin();
};
