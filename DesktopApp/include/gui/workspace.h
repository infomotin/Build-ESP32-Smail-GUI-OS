/**
 * @file workspace.h
 * @brief Workspace canvas for placing and connecting virtual components
 */

#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMenu>
#include <vector>
#include <memory>

#include "../virtual_components/component_base.h"

QT_BEGIN_NAMESPACE
class QGraphicsView;
class QGraphicsScene;
class QPropertyAnimation;
QT_END_NAMESPACE

namespace esp32sim {

class SimulationEngine;
class WiringOverlay;

/**
 * @class Workspace
 * @brief Canvas area for placing and wiring virtual components
 *
 * Features:
 * - Drag-and-drop components from library
 * - Grid-aligned placement
 * - Orthogonal wire routing between component pins and ESP32 GPIOs
 * - Selection and manipulation
 * - Zoom and pan
 * - Component property editing
 */
class Workspace : public QWidget {
    Q_OBJECT

public:
    explicit Workspace(SimulationEngine* engine, QWidget* parent = nullptr);
    ~Workspace() override;

    /**
     * @brief Add component to workspace
     */
    void addComponent(VirtualComponent* component, const QPointF& position);

    /**
     * @brief Remove component from workspace
     */
    void removeComponent(VirtualComponent* component);

    /**
     * @brief Get all components
     */
    const std::vector<std::unique_ptr<VirtualComponent>>& components() const {
        return components_;
    }

    /**
     * @brief Get component at position
     */
    VirtualComponent* componentAt(const QPointF& pos) const;

    /**
     * @brief Set grid visibility
     */
    void setGridVisible(bool visible) { show_grid_ = visible; update(); }

    /**
     * @brief Set snap to grid
     */
    void setSnapToGrid(bool snap) { snap_to_grid_ = snap; }

    /**
     * @brief Set zoom level
     */
    void setZoom(float zoom);

    /**
     * @brief Get current zoom
     */
    float zoom() const { return zoom_; }

    /**
     * @brief Save layout to JSON
     */
    std::string saveLayout() const;

    /**
     * @brief Load layout from JSON
     */
    bool loadLayout(const std::string& json);

    /**
     * @brief Clear all components
     */
    void clear();

    /**
     * @brief Select a component
     */
    void selectComponent(VirtualComponent* component);

    /**
     * @brief Get currently selected component
     */
    VirtualComponent* selectedComponent() const { return selected_component_; }

signals:
    /**
     * @brief Component added
     */
    void componentAdded(VirtualComponent* component);

    /**
     * @brief Component removed
     */
    void componentRemoved(VirtualComponent* component);

    /**
     * @brief Selection changed
     */
    void selectionChanged(VirtualComponent* component);

    /**
     * @brief Connection established
     */
    void connectionMade(int gpio_pin, int component_pin);

    /**
     * @brief Connection removed
     */
    void connectionRemoved(int gpio_pin);

public slots:
    /**
     * @brief Update all components from current GPIO state
     */
    void updateFromGPIO();

private slots:
    /**
     * @brief Handle context menu request
     */
    void showContextMenu(const QPoint& pos);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    SimulationEngine* engine_;

    // Components
    std::vector<std::unique_ptr<VirtualComponent>> components_;
    VirtualComponent* selected_component_ = nullptr;

    // View properties
    float zoom_ = 1.0f;
    QPointF view_offset_ = QPointF(0, 0);
    bool show_grid_ = true;
    bool snap_to_grid_ = true;
    static constexpr float GRID_SIZE = 10.0f;

    // Interaction state
    enum class InteractionMode {
        NONE,
        DRAGGING_COMPONENT,
        DRAGGING_SELECTION,
        WIRING,
        PANNING
    };
    InteractionMode mode_ = InteractionMode::NONE;
    VirtualComponent* dragging_component_ = nullptr;
    QPointF drag_start_pos_;
    QPointF drag_offset_;

    // Wiring state
    bool wiring_active_ = false;
    int wiring_component_pin_ = -1;
    VirtualComponent* wiring_component_ = nullptr;
    QPointF wiring_start_point_;
    QPointF wiring_current_end_;

    // Context menu
    QMenu* context_menu_ = nullptr;

    // Wiring visualization
    struct Wire {
        VirtualComponent* component = nullptr;
        int component_pin = -1;
        int gpio_pin = -1;
        QColor color = Qt::black;
    };
    std::vector<Wire> wires_;

    // Selection
    QRect selection_rect_;
    std::vector<VirtualComponent*> selected_components_;

    // Clipboard
    std::vector<VirtualComponent*> clipboard_;

    // Internal methods
    void drawGrid(QPainter* painter);
    void drawWires(QPainter* painter);
    void drawWire(QPainter* painter, const QPointF& start, const QPointF& end, const QColor& color);
    QPointF snapToGrid(const QPointF& point) const;
    void connectPinToGPIO(VirtualComponent* component, int component_pin, int gpio_pin);
    void disconnectPinFromGPIO(int gpio_pin);
    void deleteSelected();
    void copySelected();
    void paste();
    void selectAll();
    void updateSelectionRect();
    void drawSelectionBox(QPainter* painter, const VirtualComponent* comp);
    void createContextMenu(const QPoint& pos);
    void handlePinConnection(int component_pin);
    void updateWirePaths();
    void loadDefaultComponents();

    /**
     * @brief Convert screen coordinates to workspace coordinates
     */
    QPointF screenToWorkspace(const QPoint& screen_pos) const {
        return (screen_pos - rect().center()) / zoom_ - view_offset_;
    }

    /**
     * @brief Convert workspace coordinates to screen coordinates
     */
    QPoint workspaceToScreen(const QPointF& workspace_pos) const {
        return (workspace_pos + view_offset_) * zoom_ + rect().center().toPoint();
    }
};

} // namespace esp32sim
