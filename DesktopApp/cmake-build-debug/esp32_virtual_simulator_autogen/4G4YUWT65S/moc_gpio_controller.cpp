/****************************************************************************
** Meta object code from reading C++ file 'gpio_controller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/peripherals/gpio_controller.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'gpio_controller.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8esp32sim14GPIOControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::GPIOController::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim14GPIOControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::GPIOController",
        "pinStateChanged",
        "",
        "uint8_t",
        "pin",
        "GPIOLevel",
        "level",
        "interruptRaised",
        "GPIOInterruptType",
        "type"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'pinStateChanged'
        QtMocHelpers::SignalData<void(uint8_t, GPIOLevel)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 5, 6 },
        }}),
        // Signal 'interruptRaised'
        QtMocHelpers::SignalData<void(uint8_t, GPIOInterruptType)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 }, { 0x80000000 | 8, 9 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<GPIOController, qt_meta_tag_ZN8esp32sim14GPIOControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::GPIOController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim14GPIOControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim14GPIOControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim14GPIOControllerE_t>.metaTypes,
    nullptr
} };

void esp32sim::GPIOController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<GPIOController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pinStateChanged((*reinterpret_cast<std::add_pointer_t<uint8_t>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<GPIOLevel>>(_a[2]))); break;
        case 1: _t->interruptRaised((*reinterpret_cast<std::add_pointer_t<uint8_t>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<GPIOInterruptType>>(_a[2]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (GPIOController::*)(uint8_t , GPIOLevel )>(_a, &GPIOController::pinStateChanged, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (GPIOController::*)(uint8_t , GPIOInterruptType )>(_a, &GPIOController::interruptRaised, 1))
            return;
    }
}

const QMetaObject *esp32sim::GPIOController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::GPIOController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim14GPIOControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int esp32sim::GPIOController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 2)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 2)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 2;
    }
    return _id;
}

// SIGNAL 0
void esp32sim::GPIOController::pinStateChanged(uint8_t _t1, GPIOLevel _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void esp32sim::GPIOController::interruptRaised(uint8_t _t1, GPIOInterruptType _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}
QT_WARNING_POP
