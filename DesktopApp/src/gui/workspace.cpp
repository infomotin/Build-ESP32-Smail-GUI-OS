/**
 * @file workspace.cpp
 * @brief Workspace canvas implementation
 */

#include "gui/workspace.h"
#include "gui/component_library.h"
#include "virtual_components/component_base.h"
#include "virtual_components/led_component.h"
#include "virtual_components/button_component.h"
#include "simulation_engine.h"
#include "utils/logger.h"

#include <QDragEnterEvent>
#include <QMimeData>
#include <QBuffer>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDrag>
#include <QMouseEvent>
#include <QPainterPath>

namespace esp32sim {

Workspace::Workspace(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    setAcceptDrops(true);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Set up appearance
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(240, 240, 240));
    setPalette(pal);

    // Load default components on startup (for demo)
    loadDefaultComponents();
}

Workspace::~Workspace() = default;

void Workspace::addComponent(VirtualComponent* component, const QPointF& position) {
    if (!component) return;

    // Take ownership
    components_.emplace_back(component);

    // Set position (centered on drop point)
    QRectF bounds = component->boundingRect();
    component->setPosition(position - QPointF(bounds.width()/2, bounds.height()/2));

    connect(component, &VirtualComponent::changed, this, [this](){ update(); });
    connect(component, &VirtualComponent::connectionChanged,
            this, [this](int pin, bool connected) {
                Q_EMIT connectionMade(pin, connected);
            });

    LOG_DEBUG("Added component '{}' to workspace", component->name());
    Q_EMIT componentAdded(component);
    update();
}

void Workspace::removeComponent(VirtualComponent* component) {
    auto it = std::find_if(components_.begin(), components_.end(),
                           [component](const std::unique_ptr<VirtualComponent>& ptr) {
                               return ptr.get() == component;
                           });
    if (it != components_.end()) {
        // Disconnect all pins (TODO: notify engine)
        components_.erase(it);
        if (selected_component_ == component) {
            selected_component_ = nullptr;
        }

        LOG_DEBUG("Removed component '{}'", component->name());
        Q_EMIT componentRemoved(component);
        update();
    }
}

VirtualComponent* Workspace::componentAt(const QPointF& pos) const {
    for (auto& comp : components_) {
        if (comp->boundingRect().translated(comp->position()).contains(pos)) {
            return comp.get();
        }
    }
    return nullptr;
}

void Workspace::setZoom(float zoom) {
    zoom_ = std::max(0.1f, std::min(5.0f, zoom));
    update();
}

std::string Workspace::saveLayout() const {
    // Serialize to JSON
    QJsonArray components_array;
    for (const auto& comp : components_) {
        QJsonObject obj;
        obj["type"] = static_cast<int>(comp->type());
        obj["name"] = comp->name();
        obj["x"] = comp->position().x();
        obj["y"] = comp->position().y();
        obj["json"] = QString::fromStdString(comp->toJSON());
        components_array.append(obj);
    }

    QJsonObject root;
    root["components"] = components_array;
    root["zoom"] = zoom_;

    QJsonDocument doc(root);
    return doc.toJson(QJsonDocument::Compact).toStdString();
}

bool Workspace::loadLayout(const std::string& json) {
    clear();

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject root = doc.object();
    QJsonArray components = root["components"].toArray();

    for (const QJsonValue& val : components) {
        QJsonObject obj = val.toObject();
        int type = obj["type"].toInt();
        QString name = obj["name"].toString();
        qreal x = obj["x"].toDouble();
        qreal y = obj["y"].toDouble();
        QString comp_json = obj["json"].toString();

        VirtualComponent* comp = ComponentLibrary::createComponent(
            static_cast<ComponentType>(type));
        if (comp) {
            comp->setName(name);
            comp->fromJSON(comp_json.toStdString());
            addComponent(comp, QPointF(x, y));
        }
    }

    zoom_ = root["zoom"].toDouble(1.0);
    update();
    return true;
}

void Workspace::clear() {
    components_.clear();
    selected_component_ = nullptr;
    wires_.clear();
    update();
}

void Workspace::selectComponent(VirtualComponent* component) {
    selected_component_ = component;
    Q_EMIT selectionChanged(component);
    update();
}

void Workspace::updateFromGPIO() {
    // Update all components based on current GPIO state
    // Called periodically
    for (auto& comp : components_) {
        // Get component pin connections and update from engine
        // This would query the engine for each pin
    }
    update();
}

void Workspace::paintEvent(QPaintEvent* event) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw grid
    if (show_grid_) {
        drawGrid(&painter);
    }

    // Draw wires
    drawWires(&painter);

    // Draw components
    for (auto& comp : components_) {
        // Draw selection highlight
        if (comp.get() == selected_component_) {
            QRectF bounds = comp->boundingRect().translated(comp->position());
            painter.setPen(QPen(Qt::blue, 2));
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(bounds.adjusted(-2, -2, 2, 2));
        }

        comp->paint(&painter);
    }

    // Draw wiring in progress
    if (wiring_active_ && wiring_component_) {
        painter.setPen(QPen(Qt::red, 2, Qt::DashLine));
        painter.drawLine(wiring_start_point_, wiring_current_end_);
    }

    // Draw selection rectangle
    if (mode_ == InteractionMode::DRAGGING_SELECTION) {
        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter.setBrush(QColor(0, 0, 255, 50));
        painter.drawRect(selection_rect_);
    }

    QWidget::paintEvent(event);
}

void Workspace::mousePressEvent(QMouseEvent* event) {
    last_mouse_pos_ = event->pos();  // Track for panning
    QPointF workspace_pos = screenToWorkspace(event->pos());

    // Check for clicks on wire (implement later)

    // Check for component click
    VirtualComponent* clicked = componentAt(workspace_pos);

    if (event->button() == Qt::LeftButton) {
        if (clicked) {
            if (event->modifiers() & Qt::ControlModifier) {
                // Add to selection
                selected_components_.push_back(clicked);
            } else {
                selectComponent(clicked);
            }

            // Start dragging
            dragging_component_ = clicked;
            drag_start_pos_ = workspace_pos;
            drag_offset_ = clicked->position() - workspace_pos;
            mode_ = InteractionMode::DRAGGING_COMPONENT;
        } else {
            // Start selection rectangle
            selection_rect_ = QRect(event->pos(), QSize());
            mode_ = InteractionMode::DRAGGING_SELECTION;
            selected_components_.clear();
            selected_component_ = nullptr;
        }
    } else if (event->button() == Qt::RightButton) {
        // Context menu
        if (clicked) {
            selectComponent(clicked);
        } else {
            // Clear selection
            selected_component_ = nullptr;
        }
    } else if (event->button() == Qt::MiddleButton) {
        // Pan mode
        mode_ = InteractionMode::PANNING;
    }

    update();
}

void Workspace::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (mode_ == InteractionMode::DRAGGING_COMPONENT) {
            // Snap to grid if enabled
            if (snap_to_grid_ && dragging_component_) {
                QPointF new_pos = snapToGrid(dragging_component_->position());
                dragging_component_->setPosition(new_pos);
            }
        } else if (mode_ == InteractionMode::DRAGGING_SELECTION) {
            // Select components within rectangle
            selection_rect_ = selection_rect_.normalized();
            for (auto& comp : components_) {
                QPointF comp_pos = comp->position();
                if (selection_rect_.contains(comp_pos.toPoint()) ||
                    selection_rect_.contains((comp_pos + comp->boundingRect().bottomRight()).toPoint())) {
                    selected_components_.push_back(comp.get());
                }
            }
            if (!selected_components_.empty()) {
                selectComponent(selected_components_.back());
            }
        } else if (mode_ == InteractionMode::PANNING) {
            mode_ = InteractionMode::NONE;
        }
    }

    mode_ = InteractionMode::NONE;
    dragging_component_ = nullptr;
    update();
}

void Workspace::mouseMoveEvent(QMouseEvent* event) {
    QPoint current_pos = event->position().toPoint();
    QPointF workspace_pos = screenToWorkspace(current_pos);

    switch (mode_) {
        case InteractionMode::DRAGGING_COMPONENT:
            if (dragging_component_) {
                dragging_component_->setPosition(workspace_pos + drag_offset_);
                update();
            }
            break;

        case InteractionMode::DRAGGING_SELECTION:
            selection_rect_ = QRect(drag_start_pos_.toPoint(), current_pos).normalized();
            update();
            break;

        case InteractionMode::PANNING:
            view_offset_ += (current_pos - last_mouse_pos_) / zoom_;
            update();
            break;

        default:
            break;
    }

    // Update last mouse position for next move
    last_mouse_pos_ = current_pos;

    // Update wiring preview
    if (wiring_active_) {
        wiring_current_end_ = workspace_pos;
        update();
    }
}

void Workspace::wheelEvent(QWheelEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom
        float delta = event->angleDelta().y() / 120.0f;
        setZoom(zoom_ * (1.0f + delta * 0.1f));
    } else {
        // Pan
        QPoint delta = event->angleDelta();
        view_offset_ -= QPointF(delta.x(), delta.y()) * 0.5f / zoom_;
    }
    update();
    event->accept();
}

void Workspace::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasFormat("application/x-esp32-component")) {
        event->acceptProposedAction();
    }
}

void Workspace::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasFormat("application/x-esp32-component")) {
        event->acceptProposedAction();
    }
}

void Workspace::dropEvent(QDropEvent* event) {
    if (event->mimeData()->hasFormat("application/x-esp32-component")) {
        QByteArray data = event->mimeData()->data("application/x-esp32-component");
        int component_type = data.toInt();

        auto comp = ComponentLibrary::createComponent(
            static_cast<ComponentType>(component_type));
        if (comp) {
            QPointF drop_pos = screenToWorkspace(event->position().toPoint());
            addComponent(comp, drop_pos);
            event->acceptProposedAction();
        }
    }
}

void Workspace::keyPressEvent(QKeyEvent* event) {
    switch (event->key()) {
        case Qt::Key_Delete:
            if (selected_component_) {
                removeComponent(selected_component_);
            }
            break;
        case Qt::Key_C:
            if (event->modifiers() & Qt::ControlModifier) {
                copySelected();
            }
            break;
        case Qt::Key_V:
            if (event->modifiers() & Qt::ControlModifier) {
                paste();
            }
            break;
        case Qt::Key_A:
            if (event->modifiers() & Qt::ControlModifier) {
                selectAll();
            }
            break;
        default:
            QWidget::keyPressEvent(event);
    }
}

void Workspace::drawGrid(QPainter* painter) {
    painter->setPen(QPen(QColor(200, 200, 200), 0.5));

    // Calculate visible area
    QRect visible = rect();
    qreal left = -view_offset_.x() / zoom_;
    qreal top = -view_offset_.y() / zoom_;
    qreal right = left + visible.width() / zoom_;
    qreal bottom = top + visible.height() / zoom_;

    // Snap to grid
    int start_x = static_cast<int>(std::floor(left / GRID_SIZE)) * GRID_SIZE;
    int start_y = static_cast<int>(std::floor(top / GRID_SIZE)) * GRID_SIZE;

    for (int x = start_x; x < right; x += GRID_SIZE) {
        painter->drawLine(x * zoom_ + view_offset_.x(), top * zoom_ + view_offset_.y(),
                          x * zoom_ + view_offset_.x(), bottom * zoom_ + view_offset_.y());
    }
    for (int y = start_y; y < bottom; y += GRID_SIZE) {
        painter->drawLine(left * zoom_ + view_offset_.x(), y * zoom_ + view_offset_.y(),
                          right * zoom_ + view_offset_.x(), y * zoom_ + view_offset_.y());
    }
}

void Workspace::drawWires(QPainter* painter) {
    painter->setPen(QPen(Qt::black, 2));

    for (const auto& wire : wires_) {
        auto comp = wire.component;
        if (!comp) continue;

        QPointF comp_center = comp->position() + comp->boundingRect().center();
        // TODO: Get actual pin position from component
        QPointF pin_pos = comp->position();
        QPointF gpio_pos(100, 100);  // Would get from pinout location

        drawWire(painter, pin_pos, gpio_pos, wire.color);
    }
}

void Workspace::drawWire(QPainter* painter, const QPointF& start, const QPointF& end, const QColor& color) {
    // Orthogonal routing
    QPointF mid1(start.x() + 30, start.y());
    QPointF mid2(mid1.x(), end.y() - 30);
    QPointF mid3(end.x() - 30, end.y());

    QPainterPath path;
    path.moveTo(start);
    path.lineTo(mid1);
    path.lineTo(mid2);
    path.lineTo(mid3);
    path.lineTo(end);

    painter->setPen(QPen(color, 2));
    painter->drawPath(path);
}

QPointF Workspace::snapToGrid(const QPointF& point) const {
    return QPointF(
        std::round(point.x() / GRID_SIZE) * GRID_SIZE,
        std::round(point.y() / GRID_SIZE) * GRID_SIZE
    );
}

void Workspace::connectPinToGPIO(VirtualComponent* component, int component_pin, int gpio_pin) {
    // Find or create wire
    Wire wire;
    wire.component = component;
    wire.component_pin = component_pin;
    wire.gpio_pin = gpio_pin;
    wires_.push_back(wire);

    component->connectPin(gpio_pin, component_pin);

    LOG_DEBUG("Connected component pin {} to GPIO{}", component_pin, gpio_pin);
    Q_EMIT connectionMade(gpio_pin, component_pin);

    update();
}

void Workspace::disconnectPinFromGPIO(int gpio_pin) {
    auto it = std::remove_if(wires_.begin(), wires_.end(),
                            [gpio_pin](const Wire& w) { return w.gpio_pin == gpio_pin; });
    if (it != wires_.end()) {
        it->component->disconnectPin(gpio_pin);
        wires_.erase(it, wires_.end());
        Q_EMIT connectionRemoved(gpio_pin);
        update();
    }
}

void Workspace::deleteSelected() {
    if (selected_component_) {
        removeComponent(selected_component_);
    }
}

void Workspace::copySelected() {
    // Simple copy - serialize selected component to clipboard
    if (selected_component_) {
        clipboard_ = { selected_component_ };
    }
}

void Workspace::paste() {
    // Paste from clipboard
    for (auto comp : clipboard_) {
        QString data = QString::fromStdString(comp->serialize());
        QMimeData* mime = new QMimeData;
        mime->setData("application/x-esp32-component", data.toUtf8());

        QDrag* drag = new QDrag(this);
        drag->setMimeData(mime);
        drag->exec(Qt::CopyAction);
    }
}

void Workspace::selectAll() {
    selected_components_.clear();
    for (auto& comp : components_) {
        selected_components_.push_back(comp.get());
    }
    if (!selected_components_.empty()) {
        selected_component_ = selected_components_.back();
    }
    update();
    Q_EMIT selectionChanged(selected_component_);
}

void Workspace::updateSelectionRect() {
    if (selected_component_) {
        selection_rect_ = selected_component_->boundingRect().toRect()
                                .translated(selected_component_->position().toPoint());
    }
}

void Workspace::createContextMenu(const QPoint& pos) {
    QMenu menu(this);

    QAction* add_led = menu.addAction("Add LED");
    QAction* add_button = menu.addAction("Add Button");
    QAction* add_lcd = menu.addAction("Add LCD Display");
    menu.addSeparator();
    QAction* delete_comp = menu.addAction("Delete");
    QAction* properties = menu.addAction("Properties...");

    connect(add_led, &QAction::triggered, this, [this]() {
        auto led = std::make_unique<LEDComponent>();
        addComponent(led.release(), QPointF(100, 100));
    });

    // ... more actions
}

void Workspace::handlePinConnection(int component_pin) {
    // Start wiring mode
    wiring_active_ = true;
    wiring_component_pin_ = component_pin;
    wiring_component_ = selected_component_ ? selected_component_ : nullptr;

    if (wiring_component_) {
        // Get pin position
        wiring_start_point_ = QPointF(0, 0);  // Calculate from component
    }
}

void Workspace::updateWirePaths() {
    // Recalculate wire positions
}

void Workspace::loadDefaultComponents() {
    // Add demo components
    auto led = std::make_unique<LEDComponent>();
    led->setName("LED1");
    led->setColor(QColor(Qt::green));
    led->setPosition(QPointF(100, 100));
    addComponent(led.release(), QPointF(100, 100));

    auto button = std::make_unique<ButtonComponent>();
    button->setName("Button1");
    button->setPosition(QPointF(100, 200));
    addComponent(button.release(), QPointF(100, 200));
}

} // namespace esp32sim
