#include "workspace_widget.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QToolBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QListWidgetItem>
#include <QtWidgets/QScrollArea>
#include <QtWidgets/QGridLayout>

// VirtualComponent implementation
VirtualComponent::VirtualComponent(uint8_t pin, const QString& name, GpioModel* gpioModel, QWidget* parent)
    : QFrame(parent), connectedPin(pin), componentName(name), gpioModel(gpioModel)
{
    setFixedSize(80, 80);
    setFrameStyle(QFrame::Box);
    setLineWidth(2);
    setStyleSheet("background-color: #f0f0f0; border: 2px solid #333;");
    
    QVBoxLayout* layout = new QVBoxLayout(this);
    QLabel* nameLabel = new QLabel(name);
    nameLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(nameLabel);
    
    QLabel* pinLabel = new QLabel(QString("GPIO %1").arg(pin));
    pinLabel->setAlignment(Qt::AlignCenter);
    pinLabel->setStyleSheet("color: #666; font-size: 10px;");
    layout->addWidget(pinLabel);
}

void VirtualComponent::mousePressEvent(QMouseEvent *event) {
    QFrame::mousePressEvent(event);
    // Could be used for component interaction
}

// LedComponent implementation
LedComponent::LedComponent(uint8_t pin, const QString& name, const QColor& color, GpioModel* gpioModel, QWidget* parent)
    : VirtualComponent(pin, name, gpioModel, parent), ledColor(color), isOn(false)
{
    setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(color.darker().name()));
    
    // Update initial state
    updateState();
}

void LedComponent::updateState() {
    if (!gpioModel) return;
    
    bool oldState = isOn;
    isOn = gpioModel->get_output_level(connectedPin);
    
    if (isOn != oldState) {
        QColor bgColor = isOn ? ledColor : ledColor.darker(200);
        setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(bgColor.name()));
    }
}

void LedComponent::setColor(const QColor& color) {
    ledColor = color;
    updateState();
}

void LedComponent::paintEvent(QPaintEvent *event) {
    QFrame::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw LED circle
    QRect ledRect(10, 10, 60, 60);
    QColor ledColor = isOn ? this->ledColor : this->ledColor.darker(200);
    painter.setBrush(ledColor);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(ledRect);
    
    // Draw highlight if on
    if (isOn) {
        QColor highlight = Qt::white;
        highlight.setAlpha(100);
        painter.setBrush(highlight);
        painter.drawEllipse(ledRect.adjusted(15, 15, -15, -15));
    }
}

// ButtonComponent implementation
ButtonComponent::ButtonComponent(uint8_t pin, const QString& name, GpioModel* gpioModel, QWidget* parent)
    : VirtualComponent(pin, name, gpioModel, parent), isPressed(false), isPressedByUser(false)
{
    setCursor(Qt::PointingHandCursor);
    updateState();
}

void ButtonComponent::updateState() {
    if (!gpioModel) return;
    
    // Update based on GPIO state
    isPressed = gpioModel->get_input_level(connectedPin);
    
    // Update appearance
    QColor bgColor = isPressed ? QColor(200, 100, 100) : QColor(100, 200, 100);
    setStyleSheet(QString("background-color: %1; border: 2px solid #333;").arg(bgColor.name()));
}

void ButtonComponent::paintEvent(QPaintEvent *event) {
    QFrame::paintEvent(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Draw button
    QRect buttonRect(15, 20, 50, 40);
    QColor buttonColor = isPressed ? QColor(150, 50, 50) : QColor(50, 150, 50);
    painter.setBrush(buttonColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(buttonRect, 5, 5);
    
    // Draw button highlight
    QColor highlight = isPressed ? QColor(100, 30, 30) : QColor(30, 100, 30);
    painter.setBrush(highlight);
    painter.drawRoundedRect(buttonRect.adjusted(5, 5, -5, -5), 3, 3);
    
    // Draw button text
    painter.setPen(Qt::white);
    QFont font("Arial", 10, QFont::Bold);
    painter.setFont(font);
    painter.drawText(buttonRect, Qt::AlignCenter, isPressed ? "ON" : "OFF");
}

void ButtonComponent::mousePressEvent(QMouseEvent *event) {
    VirtualComponent::mousePressEvent(event);
    
    if (event->button() == Qt::LeftButton) {
        isPressedByUser = true;
        // Simulate button press by driving GPIO low
        if (gpioModel) {
            gpioModel->set_output_level(connectedPin, false);
        }
        update();
    }
}

void ButtonComponent::mouseReleaseEvent(QMouseEvent *event) {
    VirtualComponent::mouseReleaseEvent(event);
    
    if (event->button() == Qt::LeftButton && isPressedByUser) {
        isPressedByUser = false;
        // Release button (let pull-up resistor handle it)
        if (gpioModel) {
            gpioModel->set_pull_up(connectedPin, true);
            gpioModel->set_pin_mode(connectedPin, GpioMode::INPUT_PULLUP);
        }
        update();
    }
}

// PotentiometerComponent implementation
PotentiometerComponent::PotentiometerComponent(uint8_t pin, const QString& name, GpioModel* gpioModel, QWidget* parent)
    : VirtualComponent(pin, name, gpioModel, parent), currentValue(2048) // Mid-scale default
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    
    // Create slider
    slider = new QSlider(Qt::Vertical);
    slider->setRange(0, 4095);
    slider->setValue(currentValue);
    slider->setTickPosition(QSlider::TicksBothSides);
    slider->setTickInterval(1024);
    connect(slider, &QSlider::valueChanged, this, &PotentiometerComponent::setValue);
    
    // Create value label
    valueLabel = new QLabel(QString("Value: %1").arg(currentValue));
    valueLabel->setAlignment(Qt::AlignCenter);
    
    layout->addWidget(slider);
    layout->addWidget(valueLabel);
    
    // Update GPIO with initial value
    updateState();
}

void PotentiometerComponent::updateState() {
    if (!gpioModel) return;
    
    // Convert value to voltage and set ADC input
    float voltage = (currentValue / 4095.0f) * 3.3f;
    gpioModel->set_adc_input_voltage(connectedPin, voltage);
    
    valueLabel->setText(QString("Value: %1\n%2V").arg(currentValue).arg(voltage, 0, 'f', 2));
}

void PotentiometerComponent::setValue(uint16_t value) {
    currentValue = value;
    slider->setValue(value);
    updateState();
    emit valueChanged(value);
}

// WorkspaceWidget implementation
WorkspaceWidget::WorkspaceWidget(GpioModel* gpioModel, QWidget *parent)
    : QWidget(parent), gpioModel(gpioModel), nextPin(2)
{
    createLayout();
    createComponentPalette();
    createWorkspace();
    
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &WorkspaceWidget::updateAllComponents);
    updateTimer->start(100); // Update every 100ms
}

void WorkspaceWidget::createLayout() {
    mainLayout = new QVBoxLayout(this);
    
    // Title
    QLabel* titleLabel = new QLabel("Virtual Components Workspace");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont("Arial", 12, QFont::Bold);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Content
    contentLayout = new QHBoxLayout();
    mainLayout->addLayout(contentLayout);
}

void WorkspaceWidget::createComponentPalette() {
    paletteLayout = new QVBoxLayout();
    
    // Component palette
    componentPalette = new QGroupBox("Component Palette");
    QVBoxLayout* paletteVBoxLayout = new QVBoxLayout(componentPalette);
    
    // Component tool box
    componentToolBox = new QToolBox();
    
    // LED section
    QWidget* ledWidget = new QWidget();
    QVBoxLayout* ledLayout = new QVBoxLayout(ledWidget);
    
    addLedButton = new QPushButton("Add LED");
    addLedButton->setToolTip("Add an LED component");
    connect(addLedButton, &QPushButton::clicked, this, &WorkspaceWidget::onAddLed);
    ledLayout->addWidget(addLedButton);
    
    ledWidget->setLayout(ledLayout);
    componentToolBox->addItem(ledWidget, "LEDs");
    
    // Button section
    QWidget* buttonWidget = new QWidget();
    QVBoxLayout* buttonLayout = new QVBoxLayout(buttonWidget);
    
    addButtonButton = new QPushButton("Add Button");
    addButtonButton->setToolTip("Add a push button component");
    connect(addButtonButton, &QPushButton::clicked, this, &WorkspaceWidget::onAddButton);
    buttonLayout->addWidget(addButtonButton);
    
    buttonWidget->setLayout(buttonLayout);
    componentToolBox->addItem(buttonWidget, "Buttons");
    
    // Sensor section
    QWidget* sensorWidget = new QWidget();
    QVBoxLayout* sensorLayout = new QVBoxLayout(sensorWidget);
    
    addPotButton = new QPushButton("Add Potentiometer");
    addPotButton->setToolTip("Add a potentiometer component");
    connect(addPotButton, &QPushButton::clicked, this, &WorkspaceWidget::onAddPotentiometer);
    sensorLayout->addWidget(addPotButton);
    
    sensorWidget->setLayout(sensorLayout);
    componentToolBox->addItem(sensorWidget, "Sensors");
    
    paletteVBoxLayout->addWidget(componentToolBox);
    paletteVBoxLayout->addStretch();
    
    componentPalette->setLayout(paletteVBoxLayout);
    contentLayout->addWidget(componentPalette);
    
    // Component list
    componentList = new QGroupBox("Active Components");
    QVBoxLayout* listLayout = new QVBoxLayout(componentList);
    
    componentListWidget = new QListWidget();
    componentListWidget->setMaximumHeight(200);
    connect(componentListWidget, &QListWidget::itemClicked, this, &WorkspaceWidget::onComponentSelected);
    
    listLayout->addWidget(componentListWidget);
    componentList->setLayout(listLayout);
    
    paletteLayout->addWidget(componentList);
}

void WorkspaceWidget::createWorkspace() {
    // Create workspace area
    workspaceScrollArea = new QScrollArea();
    workspaceScrollArea->setWidgetResizable(true);
    workspaceScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    workspaceScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    workspaceWidget = new QWidget();
    workspaceWidget->setMinimumSize(400, 300);
    workspaceLayout = new QGridLayout(workspaceWidget);
    
    workspaceScrollArea->setWidget(workspaceWidget);
    contentLayout->addWidget(workspaceScrollArea, 1);
}

void WorkspaceWidget::addComponent(uint8_t pin, const QString& type, const QString& name) {
    VirtualComponent* component = nullptr;
    
    if (type == "LED") {
        component = new LedComponent(pin, name, QColor(255, 0, 0), gpioModel, workspaceWidget);
    } else if (type == "Button") {
        component = new ButtonComponent(pin, name, gpioModel, workspaceWidget);
    } else if (type == "Potentiometer") {
        component = new PotentiometerComponent(pin, name, gpioModel, workspaceWidget);
    }
    
    if (component) {
        // Add to workspace
        int row = components.size() / 4;
        int col = components.size() % 4;
        workspaceLayout->addWidget(component, row, col);
        
        // Store component
        components.emplace_back(component);
        pinToComponent[pin] = component;
        
        // Connect to GPIO
        if (gpioModel) {
            gpioModel->connect_component(pin, name.toStdString());
        }
        
        // Update list
        updateComponentList();
        
        emit componentAdded(pin, type);
    }
}

void WorkspaceWidget::removeComponent(uint8_t pin) {
    auto it = pinToComponent.find(pin);
    if (it != pinToComponent.end()) {
        VirtualComponent* component = it->second;
        
        // Remove from workspace
        workspaceLayout->removeWidget(component);
        
        // Remove from GPIO
        if (gpioModel) {
            gpioModel->disconnect_component(pin);
        }
        
        // Remove from components vector
        components.erase(std::remove_if(components.begin(), components.end(),
            [component](const std::unique_ptr<VirtualComponent>& ptr) {
                return ptr.get() == component;
            }), components.end());
        
        // Remove from map
        pinToComponent.erase(it);
        
        // Update list
        updateComponentList();
        
        emit componentRemoved(pin);
    }
}

void WorkspaceWidget::clearAllComponents() {
    // Remove all components
    for (auto& component : components) {
        workspaceLayout->removeWidget(component.get());
        if (gpioModel) {
            gpioModel->disconnect_component(component->getPin());
        }
    }
    
    components.clear();
    pinToComponent.clear();
    
    updateComponentList();
}

void WorkspaceWidget::updateAllComponents() {
    for (auto& component : components) {
        component->updateState();
    }
}

void WorkspaceWidget::handleGpioEvent(const GpioEvent& event) {
    // Handle GPIO events that might affect components
    auto it = pinToComponent.find(event.pin);
    if (it != pinToComponent.end()) {
        it->second->updateState();
    }
}

void WorkspaceWidget::onAddLed() {
    uint8_t pin = getAvailablePin();
    if (pin != 255) {
        addComponent(pin, "LED", QString("LED %1").arg(pin));
    } else {
        // Show message that no pins are available
        // TODO: Implement proper message
    }
}

void WorkspaceWidget::onAddButton() {
    uint8_t pin = getAvailablePin();
    if (pin != 255) {
        addComponent(pin, "Button", QString("Button %1").arg(pin));
    } else {
        // Show message that no pins are available
        // TODO: Implement proper message
    }
}

void WorkspaceWidget::onAddPotentiometer() {
    uint8_t pin = getAvailablePin();
    if (pin != 255) {
        addComponent(pin, "Potentiometer", QString("POT %1").arg(pin));
    } else {
        // Show message that no pins are available
        // TODO: Implement proper message
    }
}

void WorkspaceWidget::onComponentSelected() {
    QListWidgetItem* item = componentListWidget->currentItem();
    if (item) {
        uint8_t pin = item->data(Qt::UserRole).toUInt();
        auto it = pinToComponent.find(pin);
        if (it != pinToComponent.end()) {
            // Highlight the component in the workspace
            // TODO: Implement component highlighting
        }
    }
}

void WorkspaceWidget::updateComponentList() {
    componentListWidget->clear();
    
    for (const auto& component : components) {
        QListWidgetItem* item = new QListWidgetItem(
            QString("GPIO %1: %2").arg(component->getPin()).arg(component->getName())
        );
        item->setData(Qt::UserRole, component->getPin());
        componentListWidget->addItem(item);
    }
}

uint8_t WorkspaceWidget::getAvailablePin() {
    // Simple logic: find next available pin (avoid input-only pins and used pins)
    for (uint8_t pin = 0; pin <= 39; pin++) {
        if (pin >= 34 && pin <= 39) continue; // Skip input-only pins
        if (pin == 0) continue; // Skip EN pin
        if (pinToComponent.find(pin) == pinToComponent.end()) {
            return pin;
        }
    }
    return 255; // No available pin
}
