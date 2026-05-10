/**
 * @file pinout_panel.h
 * @brief Interactive ESP32 pinout diagram panel
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <vector>
#include <map>

#include "../peripherals/gpio_controller.h"

QT_BEGIN_NAMESPACE
class QMenu;
class QToolTip;
QT_END_NAMESPACE

namespace esp32sim {

class SimulationEngine;

/**
 * @class PinoutPanel
 * @brief Widget displaying ESP32-WROOM-32 pinout diagram
 *
 * Shows a 2D representation of the ESP32 module with all 38 exposed pins.
 * Pins are color-coded based on their mode:
 *   - Green: Digital output HIGH
 *   - Dark Gray: Digital output LOW
 *   - Blue: ADC input
 *   - Orange: PWM output
 *   - Yellow: I2C
 *   - Purple: SPI
 *   - Cyan: UART
 *   - White: Unconfigured
 *
 * Features:
 * - Click to show pin details tooltip
 * - Right-click for context menu (drive pin, connect component, view waveform)
 * - Real-time updates (60 Hz)
 */
class PinoutPanel : public QWidget {
    Q_OBJECT

public:
    explicit PinoutPanel(SimulationEngine* engine, QWidget* parent = nullptr);
    ~PinoutPanel() override;

    /**
     * @brief Set scale factor for the diagram
     */
    void setScale(float scale) { scale_ = scale; update(); }

    /**
     * @brief Get currently selected pin
     */
    int selectedPin() const { return selected_pin_; }

    /**
     * @brief Force refresh of all pins
     */
    void refresh();

signals:
    /**
     * @brief Pin clicked
     */
    void pinClicked(int pin, QPoint global_pos);

    /**
     * @brief Context menu requested
     */
    void pinContextMenuRequested(int pin, QPoint global_pos);

    /**
     * @brief Pin state changed via GUI
     */
    void pinStateChanged(int pin, bool level);

public slots:
    /**
     * @brief Update display from engine state
     */
    void updateFromEngine();

    /**
     * @brief Highlight a specific pin
     */
    void highlightPin(int pin, bool highlight);

private slots:
    /**
     * @brief Handle timer update
     */
    void onUpdateTimer();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    SimulationEngine* engine_;

    // Pin positions (relative coordinates for ESP32-WROOM-32)
    struct PinInfo {
        int number;
        QPointF position;
        bool top_side;      // true = top, false = bottom
        bool left_side;     // true = left half, false = right half
        QString label;      // Pin name (e.g., "GPIO0", "GND", "3V3")
        QString function;   // Primary function
    };

    std::vector<PinInfo> pin_positions_;
    std::map<int, QColor> pin_colors_;
    std::map<int, QString> pin_tooltips_;
    int hovered_pin_ = -1;
    int selected_pin_ = -1;

    // Layout
    float scale_ = 1.0f;
    QPointF offset_ = QPointF(0, 0);
    QRect module_rect_;

    // Update timer (60 Hz)
    QTimer* update_timer_ = nullptr;

    // Cursor
    QCursor magnifier_cursor_;

    // Internal methods
    void initializePinPositions();
    void updatePinColors();
    QColor pinColor(const GPIOPinState& pin_state) const;
    QString pinTooltip(int pin) const;
    void drawPin(QPainter* painter, const PinInfo& pin);
    void drawModuleOutline(QPainter* painter);
    int pinAtPosition(const QPointF& pos) const;
    void showPinContextMenu(int pin, const QPoint& global_pos);
    void drivePinManually(int pin, bool high);
    void openWaveformForPin(int pin);

    /**
     * @brief Check if pin is input-only
     */
    static bool isInputOnlyPin(int pin) {
        return pin >= 34 && pin <= 39;
    }
};

} // namespace esp32sim
