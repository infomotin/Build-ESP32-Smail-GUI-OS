#pragma once

#include <QtWidgets/QWidget>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QToolTip>
#include <QtWidgets/QMenu>
#include <QtCore/QTimer>
#include <QtGui/QPainter>
#include <QtGui/QMouseEvent>
#include <QtGui/QFontMetrics>

#include "../peripherals/gpio/gpio_model.h"

// ESP32 pin information
struct PinInfo {
    uint8_t number;
    QString name;
    int row;
    int col;
    bool inputOnly;
    
    PinInfo(uint8_t num, const QString& n, int r, int c, bool input = false)
        : number(num), name(n), row(r), col(c), inputOnly(input) {}
};

// Custom pin widget
class PinWidget : public QFrame {
    Q_OBJECT

public:
    PinWidget(uint8_t pinNumber, const PinInfo& info, GpioModel* gpioModel, QWidget* parent = nullptr);
    
    void updateState();
    void setHighlighted(bool highlighted);
    bool isHighlighted() const { return highlighted; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void pinClicked(uint8_t pin);
    void pinRightClicked(uint8_t pin, const QPoint& position);

private:
    uint8_t pinNumber;
    const PinInfo pinInfo;
    GpioModel* gpioModel;
    bool highlighted;
    bool mousePressed;
    
    QColor getPinColor() const;
    QString getPinTooltip() const;
};

// Pinout panel widget
class PinoutPanel : public QWidget {
    Q_OBJECT

public:
    explicit PinoutPanel(GpioModel* gpioModel, QWidget *parent = nullptr);
    
    void updateDisplay();
    void setHighlightedPin(uint8_t pin);
    void clearHighlight();

signals:
    void pinSelected(uint8_t pin);
    void pinConfigRequested(uint8_t pin);

private slots:
    void onPinClicked(uint8_t pin);
    void onPinRightClicked(uint8_t pin, const QPoint& position);
    void onPinContextMenuTriggered(QAction* action);

private:
    void createLayout();
    void createPinWidgets();
    void createLegend();
    void updatePinStates();
    
    GpioModel* gpioModel;
    QGridLayout* mainLayout;
    QGridLayout* pinLayout;
    QVBoxLayout* legendLayout;
    
    std::vector<std::unique_ptr<PinWidget>> pinWidgets;
    std::vector<PinInfo> pinInfos;
    
    QLabel* titleLabel;
    QWidget* legendWidget;
    
    uint8_t highlightedPin;
    QTimer* updateTimer;
};
