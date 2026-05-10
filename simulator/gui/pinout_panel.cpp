#include "pinout_panel.h"
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QMenu>
#include <QtWidgets/QAction>
#include <QtWidgets/QToolTip>

// PinWidget implementation
PinWidget::PinWidget(uint8_t pinNumber, const PinInfo& info, GpioModel* gpioModel, QWidget* parent)
    : QFrame(parent), pinNumber(pinNumber), pinInfo(info), gpioModel(gpioModel), highlighted(false), mousePressed(false)
{
    setFixedSize(30, 30);
    setFrameStyle(QFrame::Box);
    setLineWidth(2);
    setMouseTracking(true);
    
    // Set tooltip
    setToolTip(getPinTooltip());
}

void PinWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    QRect rect = this->rect().adjusted(2, 2, -2, -2);
    
    // Draw pin background
    QColor pinColor = getPinColor();
    painter.fillRect(rect, pinColor);
    
    // Draw pin border
    QPen pen(highlighted ? Qt::yellow : Qt::black, highlighted ? 3 : 2);
    painter.setPen(pen);
    painter.drawRect(rect);
    
    // Draw pin number
    painter.setPen(pinColor.lightness() > 0.5 ? Qt::black : Qt::white);
    QFont font("Arial", 8, QFont::Bold);
    painter.setFont(font);
    painter.drawText(rect, Qt::AlignCenter, QString::number(pinNumber));
}

void PinWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        mousePressed = true;
        update();
    }
}

void PinWidget::mouseMoveEvent(QMouseEvent *event) {
    // Update tooltip on hover
    if (!mousePressed) {
        QToolTip::showText(event->globalPos(), getPinTooltip());
    }
}

void PinWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && mousePressed) {
        mousePressed = false;
        emit pinClicked(pinNumber);
        update();
    } else if (event->button() == Qt::RightButton) {
        emit pinRightClicked(pinNumber, event->globalPos());
    }
}

void PinWidget::enterEvent(QEvent *event) {
    setHighlighted(true);
    update();
}

void PinWidget::leaveEvent(QEvent *event) {
    setHighlighted(false);
    update();
}

void PinWidget::updateState() {
    // Update tooltip and appearance
    setToolTip(getPinTooltip());
    update();
}

void PinWidget::setHighlighted(bool highlighted) {
    this->highlighted = highlighted;
    update();
}

QColor PinWidget::getPinColor() const {
    if (!gpioModel) {
        return Qt::gray;
    }
    
    GpioPinState state = gpioModel->get_pin_state(pinNumber);
    
    switch (state.mode) {
        case GpioMode::DISABLED:
            return Qt::lightGray;
            
        case GpioMode::INPUT:
        case GpioMode::INPUT_PULLUP:
        case GpioMode::INPUT_PULLDOWN:
            return state.input_level ? Qt::green : Qt::darkGray;
            
        case GpioMode::OUTPUT:
        case GpioMode::OUTPUT_OD:
            return state.output_level ? Qt::green : Qt::darkGray;
            
        case GpioMode::ADC:
            return Qt::blue;
            
        case GpioMode::DAC:
            return Qt::orange;
            
        case GpioMode::PWM:
            return Qt::magenta;
            
        case GpioMode::I2C:
            return Qt::yellow;
            
        case GpioMode::SPI:
            return Qt::cyan;
            
        case GpioMode::UART:
            return Qt::darkGreen;
            
        default:
            return Qt::gray;
    }
}

QString PinWidget::getPinTooltip() const {
    if (!gpioModel) {
        return QString("GPIO %1").arg(pinNumber);
    }
    
    GpioPinState state = gpioModel->get_pin_state(pinNumber);
    
    QString tooltip = QString("GPIO %1").arg(pinNumber);
    tooltip += QString("\nMode: %1").arg(static_cast<int>(state.mode));
    tooltip += QString("\nOutput Level: %1").arg(state.output_level ? "HIGH" : "LOW");
    tooltip += QString("\nInput Level: %1").arg(state.input_level ? "HIGH" : "LOW");
    
    if (state.connected_to_component) {
        tooltip += QString("\nConnected to: %1").arg(QString::fromStdString(state.component_name));
    }
    
    if (state.adc_enabled) {
        tooltip += QString("\nADC Value: %1 (%2V)")
                    .arg(state.adc_value)
                    .arg(state.adc_voltage, 0, 'f', 3);
    }
    
    if (state.dac_enabled) {
        tooltip += QString("\nDAC Value: %1 (%2V)")
                    .arg(state.dac_value)
                    .arg(state.dac_voltage, 0, 'f', 3);
    }
    
    if (state.pwm_enabled) {
        tooltip += QString("\nPWM: %1Hz, %2%")
                    .arg(state.pwm_frequency)
                    .arg(state.pwm_duty);
    }
    
    if (pinInfo.inputOnly) {
        tooltip += "\n(Input-only pin)";
    }
    
    return tooltip;
}

// PinoutPanel implementation
PinoutPanel::PinoutPanel(GpioModel* gpioModel, QWidget *parent)
    : QWidget(parent), gpioModel(gpioModel), highlightedPin(255)
{
    createLayout();
    createPinWidgets();
    createLegend();
    
    updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &PinoutPanel::updateDisplay);
    updateTimer->start(100); // Update every 100ms
}

void PinoutPanel::createLayout() {
    mainLayout = new QVBoxLayout(this);
    
    // Title
    titleLabel = new QLabel("ESP32 Pinout");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont("Arial", 12, QFont::Bold);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Main content
    QHBoxLayout* contentLayout = new QHBoxLayout();
    
    // Pin layout
    QWidget* pinContainer = new QWidget();
    pinLayout = new QGridLayout(pinContainer);
    pinLayout->setSpacing(2);
    pinLayout->setContentsMargins(5, 5, 5, 5);
    
    contentLayout->addWidget(pinContainer);
    
    // Legend
    legendWidget = new QWidget();
    legendLayout = new QVBoxLayout(legendWidget);
    contentLayout->addWidget(legendWidget);
    
    mainLayout->addLayout(contentLayout);
}

void PinoutPanel::createPinWidgets() {
    // Define ESP32 pin layout (simplified representation)
    pinInfos = {
        // Left side pins
        PinInfo(0, "EN", 0, 0),
        PinInfo(1, "A2/IO1", 1, 0),
        PinInfo(2, "A4/IO2", 2, 0),
        PinInfo(3, "A10/IO3", 3, 0),
        PinInfo(4, "A9/IO4", 4, 0),
        PinInfo(5, "A12/IO5", 5, 0),
        PinInfo(6, "A13/IO6", 6, 0),
        PinInfo(7, "A14/IO7", 7, 0),
        PinInfo(8, "A15/IO8", 8, 0),
        PinInfo(9, "A16/IO9", 9, 0),
        PinInfo(10, "A17/IO10", 10, 0),
        PinInfo(11, "A18/IO11", 11, 0),
        PinInfo(12, "A19/IO12", 12, 0),
        PinInfo(13, "A21/IO13", 13, 0),
        PinInfo(14, "A22/IO14", 14, 0),
        PinInfo(15, "A23/IO15", 15, 0),
        PinInfo(16, "A24/IO16", 16, 0),
        PinInfo(17, "A25/IO17", 17, 0),
        PinInfo(18, "A26/IO18", 18, 0),
        PinInfo(19, "A27/IO19", 19, 0),
        
        // Right side pins
        PinInfo(21, "SDIO_CMD", 0, 2),
        PinInfo(22, "SDIO_CLK", 1, 2),
        PinInfo(23, "SDIO_D0", 2, 2),
        PinInfo(24, "SDIO_D1", 3, 2),
        PinInfo(25, "SDIO_D2", 4, 2),
        PinInfo(26, "SDIO_D3", 5, 2),
        PinInfo(27, "SHD/SD2", 6, 2),
        PinInfo(28, "SWP/SD3", 7, 2),
        PinInfo(29, "SCS/CMD", 8, 2),
        PinInfo(30, "SCK/CLK", 9, 2),
        PinInfo(31, "SDO/SD0", 10, 2),
        PinInfo(32, "SDI/SD1", 11, 2),
        PinInfo(33, "IO35", 12, 2, true),
        PinInfo(34, "IO34", 13, 2, true),
        PinInfo(35, "IO25", 14, 2),
        PinInfo(36, "IO26", 15, 2),
        PinInfo(37, "IO27", 16, 2),
        PinInfo(38, "IO14", 17, 2),
        PinInfo(39, "IO12", 18, 2),
        PinInfo(40, "IO13", 19, 2),
        
        // Bottom pins
        PinInfo(41, "IO23", 0, 3),
        PinInfo(42, "IO22", 1, 3),
        PinInfo(43, "TXD0", 2, 3),
        PinInfo(44, "RXD0", 3, 3),
        PinInfo(45, "IO21", 4, 3),
        PinInfo(46, "IO19", 5, 3),
        PinInfo(47, "IO18", 6, 3),
        PinInfo(48, "IO5", 7, 3),
        PinInfo(49, "IO17", 8, 3),
        PinInfo(50, "IO16", 9, 3),
        PinInfo(51, "IO4", 10, 3),
        PinInfo(52, "IO0", 11, 3),
        PinInfo(53, "IO2", 12, 3),
        PinInfo(54, "IO15", 13, 3),
        PinInfo(55, "IO13", 14, 3),
        PinInfo(56, "IO12", 15, 3),
        PinInfo(57, "IO14", 16, 3),
        PinInfo(58, "IO27", 17, 3),
        PinInfo(59, "IO26", 18, 3),
        PinInfo(60, "IO25", 19, 3)
    };
    
    // Create pin widgets
    for (const auto& pinInfo : pinInfos) {
        if (pinInfo.number <= 39) { // Only create widgets for actual GPIO pins
            auto pinWidget = std::make_unique<PinWidget>(pinInfo.number, pinInfo, gpioModel, this);
            
            connect(pinWidget.get(), &PinWidget::pinClicked, this, &PinoutPanel::onPinClicked);
            connect(pinWidget.get(), &PinWidget::pinRightClicked, this, &PinoutPanel::onPinRightClicked);
            
            pinLayout->addWidget(pinWidget.get(), pinInfo.row, pinInfo.col);
            pinWidgets.push_back(std::move(pinWidget));
        }
    }
    
    // Add empty spaces for better layout
    for (int row = 0; row < 4; ++row) {
        for (int col = 0; col < 20; ++col) {
            if (!pinLayout->itemAtPosition(row, col)) {
                QLabel* emptyLabel = new QLabel();
                emptyLabel->setFixedSize(30, 30);
                pinLayout->addWidget(emptyLabel, row, col);
            }
        }
    }
}

void PinoutPanel::createLegend() {
    legendLayout->addWidget(new QLabel("Pin Colors:"));
    
    // Create color legend
    struct ColorInfo {
        QString name;
        QColor color;
    };
    
    std::vector<ColorInfo> colors = {
        {"Disabled", Qt::lightGray},
        {"Input LOW", Qt::darkGray},
        {"Input HIGH", Qt::green},
        {"Output LOW", Qt::darkGray},
        {"Output HIGH", Qt::green},
        {"ADC", Qt::blue},
        {"DAC", Qt::orange},
        {"PWM", Qt::magenta},
        {"I2C", Qt::yellow},
        {"SPI", Qt::cyan},
        {"UART", Qt::darkGreen}
    };
    
    for (const auto& colorInfo : colors) {
        QWidget* colorWidget = new QWidget();
        colorWidget->setFixedSize(20, 20);
        colorWidget->setStyleSheet(QString("background-color: %1; border: 1px solid black;").arg(colorInfo.color.name()));
        
        QHBoxLayout* colorLayout = new QHBoxLayout();
        colorLayout->addWidget(colorWidget);
        colorLayout->addWidget(new QLabel(colorInfo.name));
        colorLayout->addStretch();
        
        QWidget* colorContainer = new QWidget();
        colorContainer->setLayout(colorLayout);
        legendLayout->addWidget(colorContainer);
    }
    
    legendLayout->addStretch();
}

void PinoutPanel::updateDisplay() {
    updatePinStates();
}

void PinoutPanel::setHighlightedPin(uint8_t pin) {
    highlightedPin = pin;
    updatePinStates();
}

void PinoutPanel::clearHighlight() {
    highlightedPin = 255;
    updatePinStates();
}

void PinoutPanel::onPinClicked(uint8_t pin) {
    emit pinSelected(pin);
}

void PinoutPanel::onPinRightClicked(uint8_t pin, const QPoint& position) {
    QMenu contextMenu(this);
    
    QAction* configAction = contextMenu.addAction("Configure Pin");
    connect(configAction, &QAction::triggered, this, [this, pin]() {
        emit pinConfigRequested(pin);
    });
    
    contextMenu.addSeparator();
    
    // Add mode-specific actions
    if (gpioModel) {
        GpioPinState state = gpioModel->get_pin_state(pin);
        
        switch (state.mode) {
            case GpioMode::OUTPUT:
            case GpioMode::OUTPUT_OD:
                contextMenu.addAction("Set LOW")->triggered();
                contextMenu.addAction("Set HIGH")->triggered();
                break;
                
            case GpioMode::INPUT:
            case GpioMode::INPUT_PULLUP:
            case GpioMode::INPUT_PULLDOWN:
                contextMenu.addAction("Connect Component")->triggered();
                break;
                
            case GpioMode::ADC:
                contextMenu.addAction("Set Voltage...")->triggered();
                break;
                
            case GpioMode::DAC:
                contextMenu.addAction("Set Value...")->triggered();
                break;
                
            case GpioMode::PWM:
                contextMenu.addAction("Set Duty Cycle...")->triggered();
                contextMenu.addAction("Set Frequency...")->triggered();
                break;
                
            default:
                break;
        }
    }
    
    contextMenu.exec(position);
}

void PinoutPanel::onPinContextMenuTriggered(QAction* action) {
    // Handle context menu actions
    QString actionText = action->text();
    
    // TODO: Implement specific actions
    if (actionText == "Set LOW") {
        // Set pin low
    } else if (actionText == "Set HIGH") {
        // Set pin high
    }
}

void PinoutPanel::updatePinStates() {
    for (auto& pinWidget : pinWidgets) {
        pinWidget->updateState();
        
        // Highlight specific pin if requested
        if (highlightedPin != 255 && pinWidget->getPinNumber() == highlightedPin) {
            pinWidget->setHighlighted(true);
        } else {
            pinWidget->setHighlighted(false);
        }
    }
}
