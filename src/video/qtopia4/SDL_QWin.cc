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

#include "SDL_QWin.h"
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#include <QObject>
#include <QPaintEvent>
#include <QScreen>

#include <QtopiaApplication>
#include <QDirectPainter>

SDL_QWin::SDL_QWin(QWidget * parent, Qt::WindowFlags f)
  : QWidget(parent, f), 
  rotationMode(NoRotation), backBuffer(NULL), useRightMouseButton(false)
{
  painter = new QDirectPainter(this, QDirectPainter::NonReserved);
  vmem = QDirectPainter::frameBuffer();
}

SDL_QWin::~SDL_QWin() {
  delete backBuffer;
}

void SDL_QWin::setBackBuffer(SDL_QWin::Rotation new_rotation, QImage *new_buffer) {
  int w = QDirectPainter::screenWidth();
  int h = QDirectPainter::screenHeight();
  rotationMode = new_rotation;
  switch (rotationMode) {
    case NoRotation:
      toSDL = QMatrix(); // No conversion actually => use identity matrix
      break;
    case Clockwise:
      toSDL = QMatrix(
           0, 1, 
          -1, 0,
         h-1, 0);
      break;
    case CounterClockwise:
      toSDL = QMatrix(
        0,  -1,
        1,   0,
        0, w-1);
  }      
  toScreen = toSDL.inverted();
  delete backBuffer;
  backBuffer = new_buffer;
}

/**
 * According to Qt documentation, widget must be visible 
 * when grabbing keyboard and mouse.
 **/ 
void SDL_QWin::showEvent(QShowEvent *) {
  grabKeyboard();
  grabMouse();
}

void SDL_QWin::suspend() {
  printf("suspend\n");
  releaseKeyboard();
  releaseMouse();
  SDL_PrivateAppActive(false, SDL_APPINPUTFOCUS);
  hide();
}

void SDL_QWin::resume() {
  printf("resume\n");
  show();
  SDL_PrivateAppActive(true, SDL_APPINPUTFOCUS);
}

void SDL_QWin::closeEvent(QCloseEvent *e) {
  SDL_PrivateQuit();
}

void SDL_QWin::mouseMoveEvent(QMouseEvent *e) {
  int sdlstate = 0;
  if (pressedButton == Qt::LeftButton) {
    sdlstate |= SDL_BUTTON_LMASK;
  } else {
    sdlstate |= SDL_BUTTON_RMASK;
  }
  
  mousePosition = toSDL.map(e->globalPos());
  SDL_PrivateMouseMotion(sdlstate, 0, mousePosition.x(), mousePosition.y());
}

void SDL_QWin::mousePressEvent(QMouseEvent *e) {
  mouseMoveEvent(e);
  if (useRightMouseButton)
    pressedButton = Qt::RightButton;
  else
    pressedButton = e->button();

  mousePosition = toSDL.map(e->globalPos());
  qDebug() << e->globalPos() << mousePosition;
  SDL_PrivateMouseButton(SDL_PRESSED, (pressedButton==Qt::LeftButton)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT,
     mousePosition.x(), mousePosition.y());
}

void SDL_QWin::mouseReleaseEvent(QMouseEvent *e) {
  mousePosition = toSDL.map(e->globalPos());
  SDL_PrivateMouseButton(SDL_RELEASED, (pressedButton==Qt::LeftButton)?SDL_BUTTON_LEFT:SDL_BUTTON_RIGHT,
     mousePosition.x(), mousePosition.y());
}

void SDL_QWin::flushRegion(const QRegion &region) {
  QRegion realRegion = region & visibleRegion();

  painter->raise();
  painter->startPainting();

  if (rotationMode!=NoRotation)
    return;

  foreach(const QRect &rect, realRegion.rects()) {
    /* next - special for 18bpp framebuffer */
    /* so any other - back off */

#if 0 // Disable rotation for now
    // 18 bpp - really 3 bytes per pixel
    if (screenRotation == SDL_QT_ROTATION_90) {
      QRect rs = my_image->rect();
      QRect rd;

      int id, jd;

      if (rect.y() + rect.height() > 240) {
        rs.setRect(rect.y(), 240 - rect.width() - rect.x(), rect.height(), rect.width());
        rd = rect;
        jd = rect.y() + rect.height() - 1;
        id = rect.x();
      } else {
        rs = rect;
        rd.setRect(rect.y(), 320 - rect.width() - rect.x(), rect.height(), rect.width());
        jd = 319 - rect.x();
        id = rect.y();
      }

      //printf("rs: %d %d %d %d\n", rs.x(), rs.y(), rs.width(), rs.height());
      //printf("rd: %d %d %d %d\n", rd.x(), rd.y(), rd.width(), rd.height());
      //printf("id: %d, jd: %d\n", id, jd);

      int ii = id, jj;
      uchar *src0 = my_image->bits();
      uchar *dst0 = vmem;
      uchar *dst, *src;

      src += rs.y() * my_image->bytesPerLine() + rs.x() * 2;

      int is_lim = rs.y() + rs.height();
      int dst_offset = jd * 720 + id * 3;
      int src_offset = rs.y() * my_image->bytesPerLine() + rs.x() * 2;

      for (int ii = rs.y(); ii < is_lim;
           dst_offset += 3, src_offset += my_image->bytesPerLine(), ii++) {
        dst = dst0 + dst_offset;
        src = src0 + src_offset;
        for (int j = 0; j < rs.width(); j++) {
          unsigned short tmp = ((unsigned short)(src[1] & 0xf8)) << 2;
          dst[0] = src[0] << 1;
          dst[1] = ((src[0] & 0x80) >> 7) | ((src[1] & 0x7) << 1) | (tmp & 0xff);
          dst[2] = (tmp & 0x300) >> 8;
          dst -= 720;
          src += 2;
        }
      }
      //printf("done\n");
    } else if (screenRotation == SDL_QT_ROTATION_270) {
      QRect rs = my_image->rect();
      QRect rd;

      int id, jd;

      if (rect.y() + rect.height() > 240) {
        rs.setRect(rect.y(), 240 - rect.width() - rect.x(), rect.height(), rect.width());
        rd = rect;
        jd = rect.y();
        id = rect.x() + rect.width() - 1;
      } else {
        rs = rect;
        rd.setRect(rect.y(), 320 - rect.width() - rect.x(), rect.height(), rect.width());
        jd = rect.x();
        id = 239 - rect.y();
      }

      int ii = id, jj;
      uchar *src0 = my_image->bits();
      uchar *dst0 = vmem;
      uchar *dst, *src;

      src += rs.y() * my_image->bytesPerLine() + rs.x() * 2;

      int is_lim = rs.y() + rs.height();
      int dst_offset = jd * 720 + id * 3;
      int src_offset = rs.y() * my_image->bytesPerLine() + rs.x() * 2;

      for (int ii = rs.y(); ii < is_lim;
           dst_offset -= 3, src_offset += my_image->bytesPerLine(), ii++) {
        dst = dst0 + dst_offset;
        src = src0 + src_offset;
        for (int j = 0; j < rs.width(); j++) {
          unsigned short tmp = ((unsigned short)(src[1] & 0xf8)) << 2;
          dst[0] = src[0] << 1;
          dst[1] = ((src[0] & 0x80) >> 7) | ((src[1] & 0x7) << 1) | (tmp & 0xff);
          dst[2] = (tmp & 0x300) >> 8;
          dst += 720;
          src += 2;
        }
      }
    } else
#endif
    {
      uchar *src0 = backBuffer->bits();
      uchar *dst0 = vmem;
      uchar *dst, *src;

      int is_lim = rect.y() + rect.height();
      int s_offset = rect.y() * backBuffer->bytesPerLine() + rect.x() * 2;
      int offset = rect.y() * 720 + rect.x() * 3;

      for (int ii = rect.y(); ii < is_lim; ii++, offset += 720,
           s_offset += backBuffer->bytesPerLine()) {
        dst = dst0 + offset;
        src = src0 + s_offset;
        for (int j = 0; j < rect.width(); j++) {
          unsigned short tmp = ((unsigned short)(src[1] & 0xf8)) << 2;
          dst[0] = src[0] << 1;
          dst[1] = ((src[0] & 0x80) >> 7) | ((src[1] & 0x7) << 1) | (tmp & 0xff);
          dst[2] = (tmp & 0x300) >> 8;
          src += 2;
          dst += 3;
        }
      }
    }
  }

  painter->endPainting(realRegion);
}

// This paints the current buffer to the screen, when desired.
void SDL_QWin::paintEvent(QPaintEvent *ev) {
  if(backBuffer) 
    flushRegion(toSDL.map(QRegion(ev->rect())));
}


