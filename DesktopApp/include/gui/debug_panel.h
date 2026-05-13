/**
 * @file debug_panel.h
 * @brief Debug panel with registers, memory, breakpoints
 */

#pragma once

#include <QWidget>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextEdit>
#include <QTreeWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QTimer>

#include "../debug/debug_controller.h"
#include "../simulation_engine.h"

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QSpinBox;
QT_END_NAMESPACE

namespace esp32sim {

class SimulationEngine;

/**
 * @class DebugPanel
 * @brief Debugging panel with registers, memory, call stack
 *
 * Features:
 * - Register view (AR0-AR63, PC, PS, etc.)
 * - Memory viewer with hex/ASCII display
 * - Call stack trace
 * - Breakpoint management
 * - Watchpoint management
 * - Step/continue/run controls
 */
class DebugPanel : public QWidget {
    Q_OBJECT

public:
    explicit DebugPanel(SimulationEngine* engine, QWidget* parent = nullptr);
    ~DebugPanel() override;

    /**
     * @brief Update display from current CPU state
     */
    void updateDisplay();

    /**
     * @brief Refresh all views
     */
    void refresh();

signals:
    /**
     * @brief Breakpoint toggled
     */
    void breakpointToggled(uint32_t address, bool enabled);

    /**
     * @brief Memory edited
     */
    void memoryEdited(uint32_t address, uint32_t value);

public slots:
    /**
     * @brief Handle simulation state change
     */
    void onSimulationStateChanged(SimulationState state);

    /**
     * @brief Handle register value change
     */
    void onRegisterChanged(uint32_t index, uint32_t value);

private slots:
    /**
     * @brief Breakpoint table double-clicked
     */
    void onBreakpointDoubleClicked(int row, int col);

    /**
     * @brief Add breakpoint button clicked
     */
    void onAddBreakpoint();

    /**
     * @brief Remove breakpoint button clicked
     */
    void onRemoveBreakpoint();

    /**
     * @brief Memory address changed
     */
    void onMemoryAddressChanged(const QString& text);

    /**
     * @brief Memory value changed
     */
    void onMemoryValueChanged(const QString& text);

    /**
     * @brief Memory refresh
     */
    void onRefreshMemory();

    /**
     * @brief Step into
     */
    void onStepInto();

    /**
     * @brief Step over
     */
    void onStepOver();

    /**
     * @brief Step out
     */
    void onStepOut();

private:
    SimulationEngine* engine_ = nullptr;
    DebugController* debug_ = nullptr;

    // UI components
    QTabWidget* tab_widget_ = nullptr;

    // Registers tab
    QTableWidget* register_table_ = nullptr;

    // Memory tab
    QLineEdit* memory_addr_edit_ = nullptr;
    QLineEdit* memory_value_edit_ = nullptr;
    QTextEdit* memory_display_ = nullptr;

    // Call stack tab
    QTreeWidget* call_stack_tree_ = nullptr;

    // Breakpoints tab
    QTableWidget* breakpoint_table_ = nullptr;
    QPushButton* add_bp_button_ = nullptr;
    QPushButton* remove_bp_button_ = nullptr;

    // Watchpoints tab
    QTableWidget* watchpoint_table_ = nullptr;

    // Internal methods
    void setupRegisterTable();
    void updateRegisterTable();
    void updateMemoryDisplay(uint32_t address, uint32_t num_bytes = 256);
    void updateCallStack();
    void updateBreakpointTable();
    void updateWatchpointTable();
    QString formatHex(uint32_t value, int width = 8) const;
    QString formatBinary(uint32_t value) const;
    QString decodeRegisterName(uint32_t index) const;
};

} // namespace esp32sim
