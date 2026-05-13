/****************************************************************************
** Meta object code from reading C++ file 'debug_panel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/debug_panel.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'debug_panel.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 69
#error "This file was generated using the moc from 6.11.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN8esp32sim10DebugPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::DebugPanel::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim10DebugPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::DebugPanel",
        "breakpointToggled",
        "",
        "uint32_t",
        "address",
        "enabled",
        "memoryEdited",
        "value",
        "onSimulationStateChanged",
        "SimulationState",
        "state",
        "onRegisterChanged",
        "index",
        "onBreakpointDoubleClicked",
        "row",
        "col",
        "onAddBreakpoint",
        "onRemoveBreakpoint",
        "onMemoryAddressChanged",
        "text",
        "onMemoryValueChanged",
        "onRefreshMemory",
        "onStepInto",
        "onStepOver",
        "onStepOut"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'breakpointToggled'
        QtMocHelpers::SignalData<void(uint32_t, bool)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { QMetaType::Bool, 5 },
        }}),
        // Signal 'memoryEdited'
        QtMocHelpers::SignalData<void(uint32_t, uint32_t)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 3, 7 },
        }}),
        // Slot 'onSimulationStateChanged'
        QtMocHelpers::SlotData<void(SimulationState)>(8, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 9, 10 },
        }}),
        // Slot 'onRegisterChanged'
        QtMocHelpers::SlotData<void(uint32_t, uint32_t)>(11, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 12 }, { 0x80000000 | 3, 7 },
        }}),
        // Slot 'onBreakpointDoubleClicked'
        QtMocHelpers::SlotData<void(int, int)>(13, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 14 }, { QMetaType::Int, 15 },
        }}),
        // Slot 'onAddBreakpoint'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onRemoveBreakpoint'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onMemoryAddressChanged'
        QtMocHelpers::SlotData<void(const QString &)>(18, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 },
        }}),
        // Slot 'onMemoryValueChanged'
        QtMocHelpers::SlotData<void(const QString &)>(20, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::QString, 19 },
        }}),
        // Slot 'onRefreshMemory'
        QtMocHelpers::SlotData<void()>(21, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepInto'
        QtMocHelpers::SlotData<void()>(22, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepOver'
        QtMocHelpers::SlotData<void()>(23, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'onStepOut'
        QtMocHelpers::SlotData<void()>(24, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<DebugPanel, qt_meta_tag_ZN8esp32sim10DebugPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::DebugPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim10DebugPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim10DebugPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim10DebugPanelE_t>.metaTypes,
    nullptr
} };

void esp32sim::DebugPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<DebugPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->breakpointToggled((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 1: _t->memoryEdited((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[2]))); break;
        case 2: _t->onSimulationStateChanged((*reinterpret_cast<std::add_pointer_t<SimulationState>>(_a[1]))); break;
        case 3: _t->onRegisterChanged((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[2]))); break;
        case 4: _t->onBreakpointDoubleClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 5: _t->onAddBreakpoint(); break;
        case 6: _t->onRemoveBreakpoint(); break;
        case 7: _t->onMemoryAddressChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 8: _t->onMemoryValueChanged((*reinterpret_cast<std::add_pointer_t<QString>>(_a[1]))); break;
        case 9: _t->onRefreshMemory(); break;
        case 10: _t->onStepInto(); break;
        case 11: _t->onStepOver(); break;
        case 12: _t->onStepOut(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)(uint32_t , bool )>(_a, &DebugPanel::breakpointToggled, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (DebugPanel::*)(uint32_t , uint32_t )>(_a, &DebugPanel::memoryEdited, 1))
            return;
    }
}

const QMetaObject *esp32sim::DebugPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::DebugPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim10DebugPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int esp32sim::DebugPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void esp32sim::DebugPanel::breakpointToggled(uint32_t _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void esp32sim::DebugPanel::memoryEdited(uint32_t _t1, uint32_t _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
QT_WARNING_POP
