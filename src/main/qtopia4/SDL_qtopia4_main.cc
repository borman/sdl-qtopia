
/* Include the SDL main definition header */
#include "SDL_main.h"
#include <stdlib.h>
#include <unistd.h>
#ifdef main
#undef main
#endif
#ifdef QWS
#include <Qtopia>
#include <QtopiaApplication>
#include <stdlib.h>
#endif

extern int SDL_main(int argc, char *argv[]);

int main(int argc, char *argv[])
{
#ifdef QWS
  QtopiaApplication a( argc, argv );
  
  QString prefix = Qtopia::sandboxDir();
  if (prefix.isEmpty()) // sandbox path must be respected everywhere it works
  {
    // TODO: This duplicates Qtopia::sandboxDir(). Remove this when it works.
    qWarning("Qtopia::sandboxDir() broken");
    QString appPath = QCoreApplication::applicationFilePath(); 
    if ( appPath.startsWith( Qtopia::packagePath() ) ) 
      prefix = appPath.left( Qtopia::packagePath().length() + 32 ) + QLatin1String("/"); //32 is the md5sum length
    else
      prefix = Qtopia::qtopiaDir(); // Finally, fall back to Qtopia prefix
  }

  qDebug("APP_PREFIX := %s", prefix.toLocal8Bit().constData());
  setenv("APP_PREFIX", prefix.toLocal8Bit().constData(), TRUE);
#endif
  int r = SDL_main(argc, argv);
  return r;
}

