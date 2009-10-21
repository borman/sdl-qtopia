
/* Include the SDL main definition header */
#include "SDL_main.h"
#include <stdlib.h>
#include <unistd.h>
#ifdef main
#undef main
#endif
#ifdef QWS
#include <zapplication.h>
#include <stdlib.h>
#endif

extern int SDL_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#ifdef QWS
  ZApplication a(argc, argv);
  //QWidget dummy;
  //a.setMainWidget(&dummy);
  //dummy.show();
#endif
  int r = SDL_main(argc, argv);
  return r;
}
