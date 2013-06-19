#ifndef GLWRAP_HEADER_H
#define GLWRAP_HEADER_H

// No way we're copying this ifdef monster in every file.

#ifdef __APPLE__
#  include <OpenGL/gl.h>
#else
#  define _WINSOCKAPI_
#  ifdef WIN32
#    include <windows.h>
#  endif
#  ifdef ANDROID
#    include <GLES/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

#endif
