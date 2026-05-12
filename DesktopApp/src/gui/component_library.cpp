/**
 * @file component_library.cpp
 * @brief Component library stub implementation
 */

#include "gui/component_library.h"
#include "virtual_components/led_component.h"
#include "virtual_components/button_component.h"
#include "virtual_components/toggle_switch_component.h" // if exists
// Note: toggle component header name? button_component.h includes ToggleSwitchComponent? Actually button_component.h defines both ButtonComponent and ToggleSwitchComponent. So include that.

namespace esp32sim {

ComponentLibrary::ComponentLibrary(QWidget* parent)
    : QWidget(parent) {
    // Stub: no content yet
}

VirtualComponent* ComponentLibrary::createComponent(VirtualComponent::ComponentType type) {
    switch (type ) {
        case VirtualComponent::ComponentType::LED_SINGLE:
            return new LEDComponent();
        case VirtualComponent::ComponentType::LED_RGB:
            return new RGBLEDComponent();
        case VirtualComponent::ComponentType::BUTTON:
            return new ButtonComponent();
        case VirtualComponent::ComponentType::TOGGLE_SWITCH:
            return new ToggleSwitchComponent();
        default:
            return nullptr;
    }
}

} // namespace esp32sim
