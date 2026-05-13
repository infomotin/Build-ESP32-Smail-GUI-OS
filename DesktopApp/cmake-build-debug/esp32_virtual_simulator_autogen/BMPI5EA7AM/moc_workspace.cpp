/****************************************************************************
** Meta object code from reading C++ file 'workspace.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/workspace.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'workspace.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8esp32sim9WorkspaceE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::Workspace::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim9WorkspaceE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::Workspace",
        "componentAdded",
        "",
        "VirtualComponent*",
        "component",
        "componentRemoved",
        "selectionChanged",
        "connectionMade",
        "gpio_pin",
        "component_pin",
        "connectionRemoved",
        "updateFromGPIO",
        "showContextMenu",
        "QPoint",
        "pos"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'componentAdded'
        QtMocHelpers::SignalData<void(VirtualComponent *)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'componentRemoved'
        QtMocHelpers::SignalData<void(VirtualComponent *)>(5, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'selectionChanged'
        QtMocHelpers::SignalData<void(VirtualComponent *)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { 0x80000000 | 3, 4 },
        }}),
        // Signal 'connectionMade'
        QtMocHelpers::SignalData<void(int, int)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 }, { QMetaType::Int, 9 },
        }}),
        // Signal 'connectionRemoved'
        QtMocHelpers::SignalData<void(int)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 8 },
        }}),
        // Slot 'updateFromGPIO'
        QtMocHelpers::SlotData<void()>(11, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'showContextMenu'
        QtMocHelpers::SlotData<void(const QPoint &)>(12, 2, QMC::AccessPrivate, QMetaType::Void, {{
            { 0x80000000 | 13, 14 },
        }}),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<Workspace, qt_meta_tag_ZN8esp32sim9WorkspaceE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::Workspace::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim9WorkspaceE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim9WorkspaceE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim9WorkspaceE_t>.metaTypes,
    nullptr
} };

void esp32sim::Workspace::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<Workspace *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->componentAdded((*reinterpret_cast<std::add_pointer_t<VirtualComponent*>>(_a[1]))); break;
        case 1: _t->componentRemoved((*reinterpret_cast<std::add_pointer_t<VirtualComponent*>>(_a[1]))); break;
        case 2: _t->selectionChanged((*reinterpret_cast<std::add_pointer_t<VirtualComponent*>>(_a[1]))); break;
        case 3: _t->connectionMade((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<int>>(_a[2]))); break;
        case 4: _t->connectionRemoved((*reinterpret_cast<std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->updateFromGPIO(); break;
        case 6: _t->showContextMenu((*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[1]))); break;
        default: ;
        }
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 0:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< VirtualComponent* >(); break;
            }
            break;
        case 1:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< VirtualComponent* >(); break;
            }
            break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< VirtualComponent* >(); break;
            }
            break;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (Workspace::*)(VirtualComponent * )>(_a, &Workspace::componentAdded, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (Workspace::*)(VirtualComponent * )>(_a, &Workspace::componentRemoved, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (Workspace::*)(VirtualComponent * )>(_a, &Workspace::selectionChanged, 2))
            return;
        if (QtMocHelpers::indexOfMethod<void (Workspace::*)(int , int )>(_a, &Workspace::connectionMade, 3))
            return;
        if (QtMocHelpers::indexOfMethod<void (Workspace::*)(int )>(_a, &Workspace::connectionRemoved, 4))
            return;
    }
}

const QMetaObject *esp32sim::Workspace::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::Workspace::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim9WorkspaceE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int esp32sim::Workspace::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 7)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void esp32sim::Workspace::componentAdded(VirtualComponent * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1);
}

// SIGNAL 1
void esp32sim::Workspace::componentRemoved(VirtualComponent * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1);
}

// SIGNAL 2
void esp32sim::Workspace::selectionChanged(VirtualComponent * _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1);
}

// SIGNAL 3
void esp32sim::Workspace::connectionMade(int _t1, int _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 3, nullptr, _t1, _t2);
}

// SIGNAL 4
void esp32sim::Workspace::connectionRemoved(int _t1)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 4, nullptr, _t1);
}
QT_WARNING_POP
