/**
 * @file component_library.h
 * @brief Component library panel (stub)
 */

#ifndef GUI_COMPONENT_LIBRARY_H
#define GUI_COMPONENT_LIBRARY_H

#include <QWidget>

namespace esp32sim {

class SimulationEngine;

/**
 * @class ComponentLibrary
 * @brief Panel showing available components to drag onto workspace (stub)
 */
class ComponentLibrary : public QWidget {
    Q_OBJECT

public:
    explicit ComponentLibrary(QWidget* parent = nullptr);
    ~ComponentLibrary() override = default;
};

} // namespace esp32sim

#endif // GUI_COMPONENT_LIBRARY_H
