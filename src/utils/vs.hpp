// Visual studio workarounds in one place

#if defined(WIN32) && !defined(__CYGWIN__)  && !defined(__MINGW32__)
#  include <windows.h>
#  include <math.h>

#  define isnan _isnan
#  define round(x) (floor(x + 0.5f))
#  define roundf(x) (floor(x + 0.5f))
#endif

