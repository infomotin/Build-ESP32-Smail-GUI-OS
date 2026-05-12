/**
 * @file console_panel.cpp
 * @brief Console panel stub implementation
 */

#include "gui/console_panel.h"
#include "simulation_engine.h"

namespace esp32sim {

ConsolePanel::ConsolePanel(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    // Stub: no content
}

} // namespace esp32sim
