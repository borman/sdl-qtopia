/****************************************************************************
** Meta object code from reading C++ file 'SDL_QWin.h'
**
** Created: Sat Sep 26 14:39:16 2009
**      by: The Qt Meta Object Compiler version 59 (Qt 4.3.5)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#if !defined(Q_MOC_OUTPUT_REVISION)
//#error "The header file 'SDL_QWin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 59
#error "This file was generated using the moc from 4.3.5. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif
#include <QObject>
#include "SDL_QWin.h"
#include <qtopiaapplication.h>
#include "qmetaobject.h"
static const uint qt_meta_data_SDL_QWin[] = {

 // content:
       1,       // revision
       0,       // classname
       0,    0, // classinfo
       1,   10, // methods
       0,    0, // properties
       0,    0, // enums/sets

 // slots: signature, parameters, type, tag, flags
      10,    9,    9,    9, 0x0a,

       0        // eod
};

static const char qt_meta_stringdata_SDL_QWin[] = {
    "SDL_QWin\0\0signalRaise()\0"
};

const QMetaObject SDL_QWin::staticMetaObject = {
    { &QWidget::staticMetaObject, qt_meta_stringdata_SDL_QWin,
      qt_meta_data_SDL_QWin, 0 }
};

const QMetaObject *SDL_QWin::metaObject() const
{
    return &staticMetaObject;
}

void *SDL_QWin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_SDL_QWin))
	return static_cast<void*>(const_cast< SDL_QWin*>(this));
    return QWidget::qt_metacast(_clname);
}

int SDL_QWin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: signalRaise(); break;
        }
        _id -= 1;
    }
    return _id;
}
