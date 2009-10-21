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

#include <QtopiaApplication>
#include <QDirectPainter>

screenRotationT screenRotation = SDL_QT_NO_ROTATION;


extern "C" {
  void SDL_ChannelExists(const char *channelName);
  void SDL_ShowSplash();
  void SDL_HideSplash();
}

static pid_t pid = -1;
void SDL_ShowSplash() {
  printf("%s\n",__func__);
}

void SDL_HideSplash() {
  printf("%s\n",__func__);
}

static inline bool needSuspend() {
  printf("%s\n",__func__);
}

void SDL_ChannelExists(const char *channelName) {
  printf("%s\n",__func__);
}

SDL_QWin::SDL_QWin(QWidget * parent, Qt::WindowFlags f)
    : QWidget(parent, f),
    my_image(0),
    my_inhibit_resize(false), my_mouse_pos(-1, -1),
    my_locked(0), my_special(false),
    my_suspended(false), last_mod(false) {
  setAttribute(Qt::WA_NoBackground  );
  
  painter = new QDirectPainter(this, QDirectPainter::Reserved);
  vmem = QDirectPainter::frameBuffer();

}

SDL_QWin::~SDL_QWin() {
  if (my_image) {
    delete my_image;
  }
}
void SDL_QWin::signalRaise() {
  resume();
}

void SDL_QWin::setImage(QImage *image) {
  if ( my_image ) {
    delete my_image;
  }
  my_image = image;
}

void SDL_QWin::showEvent(QShowEvent *) {
  painter->raise();
}

void SDL_QWin::resizeEvent(QResizeEvent *) {
  qDebug() << "Resized to" << geometry();
  painter->setRegion(geometry());
}

void SDL_QWin::moveEvent(QMoveEvent *) {
  painter->setRegion(geometry());
}

void SDL_QWin::init()
{
  grabKeyboard();
  grabMouse();
// my_suspend = false;
}

void SDL_QWin::suspend() {
  printf("suspend\n");
  releaseKeyboard();
  releaseMouse();
  SDL_PrivateAppActive(false, SDL_APPINPUTFOCUS);
//  my_suspend = true;
  hide();
}

void SDL_QWin::resume() {
  printf("resume\n");
  init();
  show();
  SDL_PrivateAppActive(true, SDL_APPINPUTFOCUS);
}

void SDL_QWin::closeEvent(QCloseEvent *e) {
  SDL_PrivateQuit();
  //e->ignore();
}

void SDL_QWin::setMousePos(const QPoint &pos) {
  if (my_image->width() == height()) {
    if (screenRotation == SDL_QT_ROTATION_90)
      my_mouse_pos = QPoint(height()-pos.y(), pos.x());
    else if (screenRotation == SDL_QT_ROTATION_270)
      my_mouse_pos = QPoint(pos.y(), width()-pos.x());
  } else {
    my_mouse_pos = pos;
  }
}

void SDL_QWin::mouseMoveEvent(QMouseEvent *e) {
  int sdlstate = 0;
  if (cur_mouse_button == EZX_LEFT_BUTTON) {
    sdlstate |= SDL_BUTTON_LMASK;
  } else {
    sdlstate |= SDL_BUTTON_RMASK;
  }
  //setMousePos(e->pos());
  SDL_PrivateMouseMotion(sdlstate, 0, my_mouse_pos.x(), my_mouse_pos.y());
}

void SDL_QWin::mousePressEvent(QMouseEvent *e) {
  printf("%s\n",__func__);

  /*  mouseMoveEvent(e);
    Qt::ButtonState button = e->button();
    cur_mouse_button = my_special ? EZX_RIGHT_BUTTON : EZX_LEFT_BUTTON;
    SDL_PrivateMouseButton(SDL_PRESSED, cur_mouse_button,
  			 my_mouse_pos.x(), my_mouse_pos.y()); */
}

void SDL_QWin::mouseReleaseEvent(QMouseEvent *e) {
  printf("%s\n",__func__);
  /*
    setMousePos(e->pos());
    Qt::ButtonState button = e->button();
    SDL_PrivateMouseButton(SDL_RELEASED, cur_mouse_button,
  			 my_mouse_pos.x(), my_mouse_pos.y());
    my_mouse_pos = QPoint(-1, -1); */
}

void SDL_QWin::flushRegion(const QRegion &region) {
  painter->startPainting();
  QRegion realRegion = region & painter->allocatedRegion();

  foreach(const QRect &rect, realRegion.rects()) {
    /* next - special for 18bpp framebuffer */
    /* so any other - back off */

#if 1
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
      uchar *src0 = my_image->bits();
      uchar *dst0 = vmem;
      uchar *dst, *src;

      int is_lim = rect.y() + rect.height();
      int s_offset = rect.y() * my_image->bytesPerLine() + rect.x() * 2;
      int offset = rect.y() * 720 + rect.x() * 3;

      for (int ii = rect.y(); ii < is_lim; ii++, offset += 720,
           s_offset += my_image->bytesPerLine()) {
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
  /*
  if(my_image) {
    repaintRect(ev->rect());
  }
  */
}

inline int SDL_QWin::keyUp() {
  return my_special ? SDLK_g : SDLK_UP;
}

inline int SDL_QWin::keyDown() {
  return my_special ? SDLK_h : SDLK_DOWN;
}

inline int SDL_QWin::keyLeft() {
  return my_special ? SDLK_i : SDLK_LEFT;
}

inline int SDL_QWin::keyRight() {
  return my_special ? SDLK_j : SDLK_RIGHT;
}

/* Function to translate a keyboard transition and queue the key event
 * This should probably be a table although this method isn't exactly
 * slow.
 */
void SDL_QWin::QueueKey(QKeyEvent *e, int pressed) {
  SDL_keysym keysym;
  int scancode = 0;//e->key();

  //if(pressed){
  if (last.scancode) {
    // we press/release mod-key without releasing another key
    if (last_mod != my_special) {
      SDL_PrivateKeyboard(SDL_RELEASED, &last);
    }
  }
  //}

  /* Set the keysym information */
  if (scancode >= 'A' && scancode <= 'Z') {
    // Qt sends uppercase, SDL wants lowercase
    keysym.sym = static_cast<SDLKey>(scancode + 32);
  } else if (scancode  >= 0x1000) {
    // Special keys
    switch (scancode) {
    case 0x1031: //Cancel
      scancode = my_special ? SDLK_a : SDLK_ESCAPE;
      break;
    case 0x1004: //Joystick center
      scancode = my_special ? SDLK_b : SDLK_RETURN;
      break;
    case 0x1012: // Qt::Key_Left
      if (screenRotation == SDL_QT_ROTATION_90) scancode = keyUp();
      else if (screenRotation == SDL_QT_ROTATION_270) scancode = keyDown();
      else scancode = keyLeft();
      break;
    case 0x1013: // Qt::Key_Up
      if (screenRotation == SDL_QT_ROTATION_90) scancode = keyRight();
      else if (screenRotation == SDL_QT_ROTATION_270) scancode = keyLeft();
      else scancode = keyUp();
      break;
    case 0x1014: // Qt::Key_Right
      if (screenRotation == SDL_QT_ROTATION_90) scancode = keyDown();
      else if (screenRotation == SDL_QT_ROTATION_270) scancode = keyUp();
      else scancode = keyRight();
      break;
    case 0x1015: // Qt::Key_Down
      if (screenRotation == SDL_QT_ROTATION_90) scancode = keyLeft();
      else if (screenRotation == SDL_QT_ROTATION_270) scancode = keyRight();
      else scancode = keyDown();
      break;
    case 0x1005: //special key
    case 0x104d: //special key
      if (pressed) my_special = true;
      else my_special = false;
      return;
    case 0x1016: //VolUp
      scancode = my_special ? SDLK_c : SDLK_PLUS;
      break;
    case 0x1017: //VolDown
      scancode = my_special ? SDLK_d : SDLK_MINUS;
      break;
    case 0x1034: //Photo
      scancode = my_special ? SDLK_e : SDLK_PAUSE;
      break;
    case 0x1030: //Call
      scancode = my_special ? SDLK_f : SDLK_SPACE;
      break;
    case 0x104b: //Prev
      scancode = my_special ? SDLK_k : SDLK_o;
      break;
    case 0x1049: //Play
      scancode = my_special ? SDLK_l : SDLK_p;
      break;
    case 0x104c: //Next
      scancode = my_special ? SDLK_m : SDLK_q;
      break;
    case 0x1033: //Browser
      scancode = my_special ? SDLK_n : SDLK_r;
      break;

    default:
      scancode = SDLK_UNKNOWN;
      break;
    }
    keysym.sym = static_cast<SDLKey>(scancode);
  } else {
    keysym.sym = static_cast<SDLKey>(scancode);
  }
  keysym.scancode = scancode;
  keysym.mod = KMOD_NONE;
  if ( SDL_TranslateUNICODE ) {
    QChar qchar = 0;//e->text()[0];
    keysym.unicode = qchar.unicode();
  } else {
    keysym.unicode = 0;
  }

  last = keysym;
  last_mod = my_special;

  /* Queue the key event */
  if ( pressed ) {
    SDL_PrivateKeyboard(SDL_PRESSED, &keysym);
  } else {
    last.scancode = 0;
    SDL_PrivateKeyboard(SDL_RELEASED, &keysym);
  }
}
