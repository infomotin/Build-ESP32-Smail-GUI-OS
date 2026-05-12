/**
 * @file properties_panel.h
 * @brief Properties editing panel for selected component (stub)
 */

#ifndef GUI_PROPERTIES_PANEL_H
#define GUI_PROPERTIES_PANEL_H

#include <QWidget>
#include <memory>

namespace esp32sim {

class SimulationEngine;
class VirtualComponent;

/**
 * @class PropertiesPanel
 * @brief Panel to edit properties of selected component (stub)
 */
class PropertiesPanel : public QWidget {
    Q_OBJECT

public:
    explicit PropertiesPanel(SimulationEngine* engine, QWidget* parent = nullptr);
    ~PropertiesPanel() override = default;

public slots:
    void setComponent(VirtualComponent* component);

private:
    SimulationEngine* engine_ = nullptr;
    VirtualComponent* current_component_ = nullptr;
};

} // namespace esp32sim

#endif // GUI_PROPERTIES_PANEL_H
