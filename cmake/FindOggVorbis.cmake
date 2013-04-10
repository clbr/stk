# - Find OggVorbis
# Find the OggVorbis includes and libraries
#
# Following variables are provided:
# OGGVORBIS_FOUND
#     True if OggVorbis has been found
# OGGVORBIS_INCLUDE_DIRS
#     The include directories of OggVorbis
# OGGVORBIS_LIBRARIES
#     OggVorbis library list


find_path(OGGVORBIS_OGG_INCLUDE_DIR NAMES ogg/ogg.h PATHS "${PROJECT_SOURCE_DIR}/dependencies/include")
find_path(OGGVORBIS_VORBIS_INCLUDE_DIR NAMES vorbis/vorbisfile.h PATHS "${PROJECT_SOURCE_DIR}/dependencies/include")
find_library(OGGVORBIS_OGG_LIBRARY NAMES ogg Ogg libogg PATHS "${PROJECT_SOURCE_DIR}/dependencies/lib")
find_library(OGGVORBIS_VORBIS_LIBRARY NAMES vorbis Vorbis libvorbis PATHS "${PROJECT_SOURCE_DIR}/dependencies/lib")
find_library(OGGVORBIS_VORBISFILE_LIBRARY NAMES vorbisfile libvorbisfile PATHS "${PROJECT_SOURCE_DIR}/dependencies/lib")

if (APPLE)
    set(OGGVORBIS_OGG_INCLUDE_DIR "/Library/Frameworks/Ogg.framework/Headers/")
    set(OGGVORBIS_VORBIS_INCLUDE_DIR "/Library/Frameworks/Vorbis.framework/Headers/")
endif()

if(APPLE AND NOT OGGVORBIS_VORBISFILE_LIBRARY)
    # Seems to be the same on Apple systems
    set(OGGVORBIS_VORBISFILE_LIBRARY ${OGGVORBIS_VORBIS_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OggVorbis DEFAULT_MSG
    OGGVORBIS_OGG_INCLUDE_DIR OGGVORBIS_VORBIS_INCLUDE_DIR
    OGGVORBIS_OGG_LIBRARY OGGVORBIS_VORBIS_LIBRARY OGGVORBIS_VORBISFILE_LIBRARY)

# Publish variables
set(OGGVORBIS_INCLUDE_DIRS ${OGGVORBIS_OGG_INCLUDE_DIR} ${OGGVORBIS_VORBIS_INCLUDE_DIR})
set(OGGVORBIS_LIBRARIES ${OGGVORBIS_OGG_LIBRARY} ${OGGVORBIS_VORBIS_LIBRARY} ${OGGVORBIS_VORBISFILE_LIBRARY})
list(REMOVE_DUPLICATES OGGVORBIS_INCLUDE_DIRS)
list(REMOVE_DUPLICATES OGGVORBIS_LIBRARIES)
mark_as_advanced(OGGVORBIS_OGG_INCLUDE_DIR OGGVORBIS_VORBIS_INCLUDE_DIR)
mark_as_advanced(OGGVORBIS_OGG_LIBRARY OGGVORBIS_VORBIS_LIBRARY OGGVORBIS_VORBISFILE_LIBRARY)
