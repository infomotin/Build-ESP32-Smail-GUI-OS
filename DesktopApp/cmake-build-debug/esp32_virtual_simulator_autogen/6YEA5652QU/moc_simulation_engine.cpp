/****************************************************************************
** Meta object code from reading C++ file 'simulation_engine.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/simulation_engine.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'simulation_engine.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8esp32sim16SimulationEngineE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::SimulationEngine::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim16SimulationEngineE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::SimulationEngine",
        "stateChanged",
        "",
        "SimulationState",
        "new_state",
        "firmwareLoaded",
        "FirmwareInfo",
        "info",
        "success",
        "exceptionOccurred",
        "std::string",
        "message",
        "statisticsUpdated",
        "SimulationStatistics",
        "stats",
        "tick",
        "updateStats"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'stateChanged'
        QtMocHelpers::SignalData<void(SimulationState)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'firmwareLoaded'
        QtMocHelpers::SignalData<void(const FirmwareInfo &, bool)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 6, 7 }, { QMetaType::Bool, 8 },
        }}),
        // Signal 'exceptionOccurred'
        QtMocHelpers::SignalData<void(const std::string &)>(9, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 10, 11 },
        }}),
        // Signal 'statisticsUpdated'
        QtMocHelpers::SignalData<void(const SimulationStatistics &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
        // Slot 'tick'
        QtMocHelpers::SlotData<void()>(15, 2, QMC::AccessPrivate, QMetaType::Void),
        // Slot 'updateStats'
        QtMocHelpers::SlotData<void()>(16, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<SimulationEngine, qt_meta_tag_ZN8esp32sim16SimulationEngineE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::SimulationEngine::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim16SimulationEngineE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim16SimulationEngineE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim16SimulationEngineE_t>.metaTypes,
    nullptr
} };

void esp32sim::SimulationEngine::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<SimulationEngine *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->stateChanged((*reinterpret_cast<std::add_pointer_t<SimulationState>>(_a[1]))); break;
        case 1: _t->firmwareLoaded((*reinterpret_cast<std::add_pointer_t<FirmwareInfo>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 2: _t->exceptionOccurred((*reinterpret_cast<std::add_pointer_t<std::string>>(_a[1]))); break;
        case 3: _t->statisticsUpdated((*reinterpret_cast<std::add_pointer_t<SimulationStatistics>>(_a[1]))); break;
        case 4: _t->tick(); break;
        case 5: _t->updateStats(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (SimulationEngine::*)(SimulationState )>(_a, &SimulationEngine::stateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (SimulationEngine::*)(const FirmwareInfo & , bool )>(_a, &SimulationEngine::firmwareLoaded, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (SimulationEngine::*)(const std::string & )>(_a, &SimulationEngine::exceptionOccurred, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (SimulationEngine::*)(const SimulationStatistics & )>(_a, &SimulationEngine::statisticsUpdated, 3))
            return;
    }
}

const QMetaObject *esp32sim::SimulationEngine::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::SimulationEngine::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim16SimulationEngineE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int esp32sim::SimulationEngine::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void esp32sim::SimulationEngine::stateChanged(SimulationState _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void esp32sim::SimulationEngine::firmwareLoaded(const FirmwareInfo & _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void esp32sim::SimulationEngine::exceptionOccurred(const std::string & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void esp32sim::SimulationEngine::statisticsUpdated(const SimulationStatistics & _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1);
}
QT_WARNING_POP
