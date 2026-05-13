/**
 * @file properties_panel.cpp
 * @brief Properties panel stub implementation
 */

#include "gui/properties_panel.h"
#include "virtual_components/component_base.h"
#include "simulation_engine.h"

namespace esp32sim {

PropertiesPanel::PropertiesPanel(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    // Stub: no content
}

void PropertiesPanel::setComponent(VirtualComponent* component) {
    current_component_ = component;
    // Stub: no property UI
}

} // namespace esp32sim
