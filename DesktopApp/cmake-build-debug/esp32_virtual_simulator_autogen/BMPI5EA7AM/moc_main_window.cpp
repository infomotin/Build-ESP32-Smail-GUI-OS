/****************************************************************************
** Meta object code from reading C++ file 'main_window.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/main_window.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8esp32sim10MainWindowE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::MainWindow::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim10MainWindowE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::MainWindow",
        "newProject",
        "",
        "openProject",
        "saveProject",
        "saveProjectAs",
        "loadFirmware",
        "startSimulation",
        "pauseSimulation",
        "stopSimulation",
        "stepSimulation",
        "resetSimulation",
        "toggleBreakpoint",
        "uint32_t",
        "address",
        "showAbout",
        "showSettings",
        "selectAll",
        "updateStatus",
        "onFirmwareLoaded",
        "success",
        "error_msg",
        "onSimulationStateChanged",
        "SimulationState",
        "state",
        "onComponentSelected",
        "VirtualComponent*",
        "component",
        "onPinStateChanged",
        "uint8_t",
        "pin",
        "GPIOLevel",
        "level",
        "onPinSelection",
        "QPoint",
        "global_pos",
        "onPinContextMenu",
        "onComponentAdded"
    };

    QtMocHelpers::UintData qt_methods {
        // Slot 'newProject'
        QtMocHelpers::SlotData<void()>(1, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'openProject'
        QtMocHelpers::SlotData<void()>(3, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'saveProject'
        QtMocHelpers::SlotData<void()>(4, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'saveProjectAs'
        QtMocHelpers::SlotData<void()>(5, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'loadFirmware'
        QtMocHelpers::SlotData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'startSimulation'
        QtMocHelpers::SlotData<void()>(7, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'pauseSimulation'
        QtMocHelpers::SlotData<void()>(8, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'stopSimulation'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'stepSimulation'
        QtMocHelpers::SlotData<void()>(10, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'resetSimulation'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'toggleBreakpoint'
        QtMocHelpers::SlotData<void(uint32_t)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'showAbout'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'showSettings'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'selectAll'
        QtMocHelpers::SlotData<void()>(17, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'updateStatus'
        QtMocHelpers::SlotData<void()>(18, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'onFirmwareLoaded'
        QtMocHelpers::SlotData<void(bool, const QString &)>(19, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Bool, 20 }, { QMetaType::QString, 21 },
        }}),
        // Slot 'onFirmwareLoaded'
        QtMocHelpers::SlotData<void(bool)>(19, 2, QMC::AccessPublic | QMC::MethodCloned, QMetaType::Void, {{
            { QMetaType::Bool, 20 },
        }}),
        // Slot 'onSimulationStateChanged'
        QtMocHelpers::SlotData<void(SimulationState)>(22, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 23, 24 },
        }}),
        // Slot 'onComponentSelected'
        QtMocHelpers::SlotData<void(VirtualComponent *)>(25, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 26, 27 },
        }}),
        // Slot 'onPinStateChanged'
        QtMocHelpers::SlotData<void(uint8_t, GPIOLevel)>(28, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 29, 30 }, { 0x80000000 | 31, 32 },
        }}),
        // Slot 'onPinSelection'
        QtMocHelpers::SlotData<void(int, QPoint)>(33, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 30 }, { 0x80000000 | 34, 35 },
        }}),
        // Slot 'onPinContextMenu'
        QtMocHelpers::SlotData<void(int, QPoint)>(36, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { QMetaType::Int, 30 }, { 0x80000000 | 34, 35 },
        }}),
        // Slot 'onComponentAdded'
        QtMocHelpers::SlotData<void(VirtualComponent *)>(37, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 26, 27 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<MainWindow, qt_meta_tag_ZN8esp32sim10MainWindowE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim10MainWindowE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim10MainWindowE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim10MainWindowE_t>.metaTypes,
    nullptr
} };

void esp32sim::MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<MainWindow *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->newProject(); break;
        case 1: _t->openProject(); break;
        case 2: _t->saveProject(); break;
        case 3: _t->saveProjectAs(); break;
        case 4: _t->loadFirmware(); break;
        case 5: _t->startSimulation(); break;
        case 6: _t->pauseSimulation(); break;
        case 7: _t->stopSimulation(); break;
        case 8: _t->stepSimulation(); break;
        case 9: _t->resetSimulation(); break;
        case 10: _t->toggleBreakpoint((*reinterpret_cast<std::add_pointer_t<uint32_t>>(_a[1]))); break;
        case 11: _t->showAbout(); break;
        case 12: _t->showSettings(); break;
        case 13: _t->selectAll(); break;
        case 14: _t->updateStatus(); break;
        case 15: _t->onFirmwareLoaded((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QString>>(_a[2]))); break;
        case 16: _t->onFirmwareLoaded((*reinterpret_cast<std::add_pointer_t<bool>>(_a[1]))); break;
        case 17: _t->onSimulationStateChanged((*reinterpret_cast<std::add_pointer_t<SimulationState>>(_a[1]))); break;
        case 18: _t->onComponentSelected((*reinterpret_cast<std::add_pointer_t<VirtualComponent*>>(_a[1]))); break;
        case 19: _t->onPinStateChanged((*reinterpret_cast<std::add_pointer_t<uint8_t>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<GPIOLevel>>(_a[2]))); break;
        case 20: _t->onPinSelection((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 21: _t->onPinContextMenu((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 22: _t->onComponentAdded((*reinterpret_cast<std::add_pointer_t<VirtualComponent*>>(_a[1]))); break;
        default: ;
        }
    }
}

const QMetaObject *esp32sim::MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim10MainWindowE_t>.strings))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int esp32sim::MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 23)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 23;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 23)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 23;
    }
    return _id;
}
QT_WARNING_POP
