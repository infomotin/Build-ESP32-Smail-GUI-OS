/**
 * @file debug_panel.cpp
 * @brief Debug panel implementation
 */

#include "gui/debug_panel.h"
#include "simulation_engine.h"
#include "debug/debug_controller.h"
#include "utils/logger.h"

#include <QTableWidget>
#include <QHeaderView>
#include <QTextEdit>
#include <QTreeWidget>
#include <QComboBox>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

namespace esp32sim {

DebugPanel::DebugPanel(SimulationEngine* engine, QWidget* parent)
    : QWidget(parent), engine_(engine) {
    if (!engine_) return;

    debug_ = engine_->debug();
    if (!debug_) return;

    setupUI();
    updateDisplay();
}

DebugPanel::~DebugPanel() = default;

void esp32sim::DebugPanel::setupUI() {
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    setLayout(main_layout);

    tab_widget_ = new QTabWidget(this);
    main_layout->addWidget(tab_widget_);

    // Registers tab
    QWidget* registers_tab = new QWidget();
    QVBoxLayout* reg_layout = new QVBoxLayout(registers_tab);
    register_table_ = new QTableWidget(64, 2, registers_tab);
    register_table_->setHorizontalHeaderLabels({"Register", "Value"});
    reg_layout->addWidget(register_table_);
    tab_widget_->addTab(registers_tab, "Registers");

    // Memory tab
    QWidget* memory_tab = new QWidget();
    QVBoxLayout* mem_layout = new QVBoxLayout(memory_tab);
    QHBoxLayout* mem_controls = new QHBoxLayout();
    QLabel* addr_lbl = new QLabel("Address:");
    memory_addr_edit_ = new QLineEdit("0x3FFB0000");
    QLabel* value_lbl = new QLabel("Value (hex):");
    memory_value_edit_ = new QLineEdit("0x00000000");
    QPushButton* go_btn = new QPushButton("Go");
    QPushButton* refresh_btn = new QPushButton("Refresh");
    connect(refresh_btn, &QPushButton::clicked, this, &DebugPanel::onRefreshMemory);
    mem_controls->addWidget(addr_lbl);
    mem_controls->addWidget(memory_addr_edit_);
    mem_controls->addWidget(value_lbl);
    mem_controls->addWidget(memory_value_edit_);
    mem_controls->addWidget(go_btn);
    mem_controls->addWidget(refresh_btn);
    memory_display_ = new QTextEdit();
    memory_display_->setReadOnly(true);
    memory_display_->setFont(QFont("Monospace", 9));
    mem_layout->addLayout(mem_controls);
    mem_layout->addWidget(memory_display_);
    tab_widget_->addTab(memory_tab, "Memory");

    // Call Stack tab
    QWidget* stack_tab = new QWidget();
    QVBoxLayout* stack_layout = new QVBoxLayout(stack_tab);
    call_stack_tree_ = new QTreeWidget();
    call_stack_tree_->setHeaderLabels({"Function", "Location", "Address"});
    stack_layout->addWidget(call_stack_tree_);
    tab_widget_->addTab(stack_tab, "Call Stack");

    // Breakpoints tab
    QWidget* bp_tab = new QWidget();
    QVBoxLayout* bp_layout = new QVBoxLayout(bp_tab);
    breakpoint_table_ = new QTableWidget(0, 3);
    breakpoint_table_->setHorizontalHeaderLabels({"Address", "Enabled", "Hits"});
    breakpoint_table_->horizontalHeader()->setStretchLastSection(true);
    QHBoxLayout* bp_buttons = new QHBoxLayout();
    add_bp_button_ = new QPushButton("Add");
    remove_bp_button_ = new QPushButton("Remove");
    bp_buttons->addWidget(add_bp_button_);
    bp_buttons->addWidget(remove_bp_button_);
    bp_layout->addWidget(breakpoint_table_);
    bp_layout->addLayout(bp_buttons);
    tab_widget_->addTab(bp_tab, "Breakpoints");

    // Connect signals
    connect(add_bp_button_, &QPushButton::clicked, this, &DebugPanel::onAddBreakpoint);
    connect(remove_bp_button_, &QPushButton::clicked, this, &DebugPanel::onRemoveBreakpoint);
    connect(breakpoint_table_, &QTableWidget::cellDoubleClicked,
            this, &DebugPanel::onBreakpointDoubleClicked);
}

void esp32sim::DebugPanel::updateDisplay() {
    if (!engine_ || !debug_) return;

    updateRegisterTable();
    updateMemoryDisplay(0);
    updateCallStack();
    updateBreakpointTable();
}

void esp32sim::DebugPanel::updateRegisterTable() {
    if (!register_table_) return;

    register_table_->setRowCount(64);
    for (int i = 0; i < 64; i++) {
        uint32_t val = debug_->getRegister(i);
        QString reg_name = QString("AR%1").arg(i);
        if (i == 0) reg_name = "PC";
        else if (i == 1) reg_name = "PS";

        QTableWidgetItem* name_item = new QTableWidgetItem(reg_name);
        QTableWidgetItem* value_item = new QTableWidgetItem(
            QString("0x%1").arg(val, 8, 16, QChar('0')));

        register_table_->setItem(i, 0, name_item);
        register_table_->setItem(i, 1, value_item);
    }
}

void esp32sim::DebugPanel::updateMemoryDisplay(uint32_t address, uint32_t num_bytes) {
    Q_UNUSED(num_bytes);
    if (!memory_display_) return;

    QString text;
    uint32_t start = address;

    for (int row = 0; row < 16; row++) {
        text += QString("0x%1: ").arg(start + row * 16, 8, 16, QChar('0'));
        for (int col = 0; col < 16; col++) {
            uint32_t addr = start + row * 16 + col;
            uint8_t byte = engine_->memory()->read_byte(addr);
            text += QString("%1 ").arg(byte, 2, 16, QChar('0'));
        }
        text += "\n";
    }

    memory_display_->setPlainText(text);
}

void esp32sim::DebugPanel::updateCallStack() {
    if (!call_stack_tree_) return;

    call_stack_tree_->clear();

    auto frames = debug_->getCallStack();
    for (const auto& frame : frames) {
        QTreeWidgetItem* item = new QTreeWidgetItem();
        item->setText(0, QString::fromStdString(frame.function_name));
        item->setText(1, QString::fromStdString(frame.source_file) + ":" + QString::number(frame.line_number));
        item->setText(2, QString("0x%1").arg(frame.return_address, 8, 16, QChar('0')));
        call_stack_tree_->addTopLevelItem(item);
    }
}

void esp32sim::DebugPanel::updateBreakpointTable() {
    if (!breakpoint_table_ || !debug_) return;

    const auto& bps = debug_->breakpoints();
    breakpoint_table_->setRowCount(bps.size());

    for (size_t i = 0; i < bps.size(); i++) {
        const auto& bp = bps[i];
        QTableWidgetItem* addr_item = new QTableWidgetItem(
            QString("0x%1").arg(bp.address, 8, 16, QChar('0')));
        QTableWidgetItem* enabled_item = new QTableWidgetItem(
            bp.enabled ? "Enabled" : "Disabled");
        QTableWidgetItem* hits_item = new QTableWidgetItem(
            QString::number(bp.hit_count));

        breakpoint_table_->setItem(i, 0, addr_item);
        breakpoint_table_->setItem(i, 1, enabled_item);
        breakpoint_table_->setItem(i, 2, hits_item);
    }
}

void esp32sim::DebugPanel::onSimulationStateChanged(SimulationState state) {
    setEnabled(state != SimulationState::STOPPED);
}

void esp32sim::DebugPanel::onAddBreakpoint() {
    bool ok;
    QString addr_str = QInputDialog::getText(this, "Add Breakpoint",
                                             "Address (hex):", QLineEdit::Normal,
                                             "0x", &ok);
    if (ok && !addr_str.isEmpty()) {
        bool ok_addr;
        uint32_t addr = addr_str.toUInt(&ok_addr, 0);
        if (ok_addr) {
            debug_->setBreakpoint(addr);
            updateBreakpointTable();
        }
    }
}

void esp32sim::DebugPanel::onRemoveBreakpoint() {
    int row = breakpoint_table_->currentRow();
    if (row >= 0) {
        auto bps = debug_->breakpoints();
        if (row < static_cast<int>(bps.size())) {
            debug_->clearBreakpoint(bps[row].address);
            updateBreakpointTable();
        }
    }
}

void esp32sim::DebugPanel::onBreakpointDoubleClicked(int row, int column) {
    Q_UNUSED(row);
    Q_UNUSED(column);
    // TODO: toggle breakpoint state or edit
}

void esp32sim::DebugPanel::onRefreshMemory() {
    bool ok;
    uint32_t addr = memory_addr_edit_->text().toUInt(&ok, 0);
    if (ok) {
        updateMemoryDisplay(addr);
    }
}

QString DebugPanel::formatHex(uint32_t value, int width) const {
    return QString("0x%1").arg(value, width, 16, QChar('0'));
}

QString DebugPanel::formatBinary(uint32_t value) const {
    return "b'" + QString::number(value, 2) + "'";
}

} // namespace esp32sim
void esp32sim::DebugPanel::onRegisterChanged(uint32_t, uint32_t) {}
void esp32sim::DebugPanel::onMemoryAddressChanged(const QString&) {}
void esp32sim::DebugPanel::onMemoryValueChanged(const QString&) {}
void esp32sim::DebugPanel::onStepInto() {}
void esp32sim::DebugPanel::onStepOver() {}
void esp32sim::DebugPanel::onStepOut() {}
