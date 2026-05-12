/**
 * @file console_panel.h
 * @brief Console output panel (stub)
 */

#ifndef GUI_CONSOLE_PANEL_H
#define GUI_CONSOLE_PANEL_H

#include <QWidget>

namespace esp32sim {

class SimulationEngine;

/**
 * @class ConsolePanel
 * @brief Panel displaying log messages (stub)
 */
class ConsolePanel : public QWidget {
    Q_OBJECT

public:
    explicit ConsolePanel(SimulationEngine* engine, QWidget* parent = nullptr);
    ~ConsolePanel() override = default;

signals:
    void clearConsole();

private:
    SimulationEngine* engine_ = nullptr;
};

} // namespace esp32sim

#endif // GUI_CONSOLE_PANEL_H
