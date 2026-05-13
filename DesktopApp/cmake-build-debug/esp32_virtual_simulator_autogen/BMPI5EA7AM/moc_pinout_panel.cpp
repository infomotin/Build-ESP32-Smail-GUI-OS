/****************************************************************************
** Meta object code from reading C++ file 'pinout_panel.h'
**
** Created by: The Qt Meta Object Compiler version 69 (Qt 6.11.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../../include/gui/pinout_panel.h"
#include <QtCore/qmetatype.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'pinout_panel.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN8esp32sim11PinoutPanelE_t {};
} // unnamed namespace

template <> constexpr inline auto esp32sim::PinoutPanel::qt_create_metaobjectdata<qt_meta_tag_ZN8esp32sim11PinoutPanelE_t>()
{
    namespace QMC = QtMocConstants;
    QtMocHelpers::StringRefStorage qt_stringData {
        "esp32sim::PinoutPanel",
        "pinClicked",
        "",
        "pin",
        "QPoint",
        "global_pos",
        "pinContextMenuRequested",
        "pinStateChanged",
        "level",
        "updateFromEngine",
        "highlightPin",
        "highlight",
        "showPinContextMenu",
        "onUpdateTimer"
    };

    QtMocHelpers::UintData qt_methods {
        // Signal 'pinClicked'
        QtMocHelpers::SignalData<void(int, QPoint)>(1, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 4, 5 },
        }}),
        // Signal 'pinContextMenuRequested'
        QtMocHelpers::SignalData<void(int, QPoint)>(6, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 4, 5 },
        }}),
        // Signal 'pinStateChanged'
        QtMocHelpers::SignalData<void(int, bool)>(7, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Bool, 8 },
        }}),
        // Slot 'updateFromEngine'
        QtMocHelpers::SlotData<void()>(9, 2, QMC::AccessPublic, QMetaType::Void),
        // Slot 'highlightPin'
        QtMocHelpers::SlotData<void(int, bool)>(10, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { QMetaType::Bool, 11 },
        }}),
        // Slot 'showPinContextMenu'
        QtMocHelpers::SlotData<void(int, const QPoint &)>(12, 2, QMC::AccessPublic, QMetaType::Void, {{
            { QMetaType::Int, 3 }, { 0x80000000 | 4, 5 },
        }}),
        // Slot 'onUpdateTimer'
        QtMocHelpers::SlotData<void()>(13, 2, QMC::AccessPrivate, QMetaType::Void),
    };
    QtMocHelpers::UintData qt_properties {
    };
    QtMocHelpers::UintData qt_enums {
    };
    return QtMocHelpers::metaObjectData<PinoutPanel, qt_meta_tag_ZN8esp32sim11PinoutPanelE_t>(QMC::MetaObjectFlag{}, qt_stringData,
            qt_methods, qt_properties, qt_enums);
}
Q_CONSTINIT const QMetaObject esp32sim::PinoutPanel::staticMetaObject = { {
    QMetaObject::SuperData::link<QWidget::staticMetaObject>(),
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim11PinoutPanelE_t>.stringdata,
    qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim11PinoutPanelE_t>.data,
    qt_static_metacall,
    nullptr,
    qt_staticMetaObjectRelocatingContent<qt_meta_tag_ZN8esp32sim11PinoutPanelE_t>.metaTypes,
    nullptr
} };

void esp32sim::PinoutPanel::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<PinoutPanel *>(_o);
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: _t->pinClicked((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 1: _t->pinContextMenuRequested((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 2: _t->pinStateChanged((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 3: _t->updateFromEngine(); break;
        case 4: _t->highlightPin((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<bool>>(_a[2]))); break;
        case 5: _t->showPinContextMenu((*reinterpret_cast<std::add_pointer_t<int>>(_a[1])),(*reinterpret_cast<std::add_pointer_t<QPoint>>(_a[2]))); break;
        case 6: _t->onUpdateTimer(); break;
        default: ;
        }
    }
    if (_c == QMetaObject::IndexOfMethod) {
        if (QtMocHelpers::indexOfMethod<void (PinoutPanel::*)(int , QPoint )>(_a, &PinoutPanel::pinClicked, 0))
            return;
        if (QtMocHelpers::indexOfMethod<void (PinoutPanel::*)(int , QPoint )>(_a, &PinoutPanel::pinContextMenuRequested, 1))
            return;
        if (QtMocHelpers::indexOfMethod<void (PinoutPanel::*)(int , bool )>(_a, &PinoutPanel::pinStateChanged, 2))
            return;
    }
}

const QMetaObject *esp32sim::PinoutPanel::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *esp32sim::PinoutPanel::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_staticMetaObjectStaticContent<qt_meta_tag_ZN8esp32sim11PinoutPanelE_t>.strings))
        return static_cast<void*>(this);
    return QWidget::qt_metacast(_clname);
}

int esp32sim::PinoutPanel::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
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
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 7;
    }
    return _id;
}

// SIGNAL 0
void esp32sim::PinoutPanel::pinClicked(int _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 0, nullptr, _t1, _t2);
}

// SIGNAL 1
void esp32sim::PinoutPanel::pinContextMenuRequested(int _t1, QPoint _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 1, nullptr, _t1, _t2);
}

// SIGNAL 2
void esp32sim::PinoutPanel::pinStateChanged(int _t1, bool _t2)
{
    QMetaObject::activate<void>(this, &staticMetaObject, 2, nullptr, _t1, _t2);
}
QT_WARNING_POP
