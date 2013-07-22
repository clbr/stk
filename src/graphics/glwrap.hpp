#ifndef GLWRAP_HEADER_H
#define GLWRAP_HEADER_H

// No way we're copying this ifdef monster in every file.

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else

#define GL_GLEXT_PROTOTYPES 1

#  ifdef ANDROID
#    include <GLES/gl.h>
#  else
#    include <GL/gl.h>
#  endif

#  ifdef WIN32
#    define _WINSOCKAPI_
#    include <windows.h>
// Windows has intentionally handicapped GL support. Not going to declare individual
// tokens here, better include the glext.h already shipped in the project.
#    include "../../lib/irrlicht/source/Irrlicht/glext.h"
#  else
#    include <GL/glext.h>
#  endif
#endif

#endif
