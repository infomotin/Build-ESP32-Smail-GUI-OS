/**
 * @file component_library.cpp
 * @brief Component library stub implementation
 */

#include "gui/component_library.h"
#include "virtual_components/led_component.h"
#include "virtual_components/button_component.h"

namespace esp32sim {

ComponentLibrary::ComponentLibrary(QWidget* parent)
    : QWidget(parent) {
    // Stub: no content yet
}

VirtualComponent* ComponentLibrary::createComponent(ComponentType type) {
    switch (type) {
        case ComponentType::LED_SINGLE:
            return new LEDComponent();
        case ComponentType::LED_RGB:
            return new RGBLEDComponent();
        case ComponentType::BUTTON:
            return new ButtonComponent();
        case ComponentType::TOGGLE_SWITCH:
            return new ToggleSwitchComponent();
        default:
            return nullptr;
    }
}

} // namespace esp32sim
