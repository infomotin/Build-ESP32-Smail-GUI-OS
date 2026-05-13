/****************************************************************************
** Meta object code from reading C++ file 'uart_controller.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/peripherals/uart_controller.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'uart_controller.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8esp32sim14UARTControllerE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::UARTController::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim14UARTControllerE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::UARTController",
        "byteTransmitted",
        "",
        "uint8_t",
        "byte",
        "byteReceived",
        "breakDetected"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'byteTransmitted'
        QtMocHelpers::SignalData<void(uint8_t)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'byteReceived'
        QtMocHelpers::SignalData<void(uint8_t)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'breakDetected'
        QtMocHelpers::SignalData<void()>(6, 2, QMC::AccessPublic, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<UARTController, qt_meta_tag_ZN8esp32sim14UARTControllerE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::UARTController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim14UARTControllerE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim14UARTControllerE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim14UARTControllerE_t>.metaTypes,
    nullptr
} };

void esp32sim::UARTController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<UARTController *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->byteTransmitted((*reinterpret_cast<std::add_pointer_t<uint8_t>>(_a[1]))); break;
        case 1: _t->byteReceived((*reinterpret_cast<std::add_pointer_t<uint8_t>>(_a[1]))); break;
        case 2: _t->breakDetected(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (UARTController::*)(uint8_t )>(_a, &UARTController::byteTransmitted, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (UARTController::*)(uint8_t )>(_a, &UARTController::byteReceived, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (UARTController::*)()>(_a, &UARTController::breakDetected, 2))
            return;
    }
}

const QMetaObject *esp32sim::UARTController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::UARTController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim14UARTControllerE_t>.strings))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int esp32sim::UARTController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 3)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 3;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 3)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 3;
    }
    return _id;
}

// SIGNAL 0
void esp32sim::UARTController::byteTransmitted(uint8_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void esp32sim::UARTController::byteReceived(uint8_t _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void esp32sim::UARTController::breakDetected()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
