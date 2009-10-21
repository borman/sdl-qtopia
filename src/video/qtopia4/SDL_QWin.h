/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

#ifndef _SDL_QWin_h
#define _SDL_QWin_h

#include <stdio.h>

#include <QImage>
#include <QPixmap>
#include <QWidget>
#include <QObject>
#include <QDirectPainter>

#include "SDL_events.h"

extern "C" {
#include "../../events/SDL_events_c.h"
};

typedef enum {
  SDL_QT_NO_ROTATION = 0,
  SDL_QT_ROTATION_90,
  SDL_QT_ROTATION_270
} screenRotationT;

extern screenRotationT screenRotation;

class SDL_QWin : public QWidget {
  Q_OBJECT
private:
  enum {
    EZX_LEFT_BUTTON = 1,
    EZX_RIGHT_BUTTON = 2
  };
  void QueueKey(QKeyEvent *e, int pressed);
public:
  SDL_QWin(QWidget *parent = 0, Qt::WindowFlags f = 0);
  virtual ~SDL_QWin();

  /* If called, the next resize event will not be forwarded to SDL. */
  virtual void inhibitResize(void) {
    my_inhibit_resize = true;
  }
  void setImage(QImage *image);
  void setOffset(int x, int y) {
    my_offset = QPoint(x, y);
  }
  void GetXYOffset(int &x, int &y) {
    x = my_offset.x();
    y = my_offset.y();
  }
  QImage *image(void) {
    return my_image;
  }

  const QPoint& mousePos() const {
    return my_mouse_pos;
  }
  void setMousePos(const QPoint& newpos);

  void flushRegion(const QRegion& region);

public slots:
  void signalRaise();
  //void clickAppIcon();

protected:
  /* Handle resizing of the window */
  void resizeEvent(QResizeEvent *e);
  void moveEvent(QMoveEvent *e);
  void showEvent(QShowEvent *e);
#ifdef MOTOEZX_TEST
  void focusInEvent(QFocusEvent *);
  void focusOutEvent(QFocusEvent *);
#endif
  void closeEvent(QCloseEvent *e);
  void mouseMoveEvent(QMouseEvent *e);
  void mousePressEvent(QMouseEvent *e);
  void mouseReleaseEvent(QMouseEvent *e);
  void paintEvent(QPaintEvent *ev);
  void keyPressEvent(QKeyEvent *e)   {
    QueueKey(e, 1);
  }
  void keyReleaseEvent(QKeyEvent *e) {
    QueueKey(e, 0);
  }
private:
  void init();
  void suspend();
  void resume();

  int keyUp();
  int keyDown();
  int keyLeft();
  int keyRight();

  uchar *vmem;
  QDirectPainter *painter;
  QImage *my_image;
  bool my_inhibit_resize;
  QPoint my_offset;
  QPoint my_mouse_pos;
  unsigned int my_locked;
  int cur_mouse_button;
  bool my_special;
  int my_timer;
  bool my_suspended;
  SDL_keysym last;
  bool last_mod;
};

#endif /* _SDL_QWin_h */
