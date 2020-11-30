# - Find opus
# Find the native opus includes and libraries
#
#  OPUS_INCLUDE_DIRS - where to find opus.h, etc.
#  OPUS_LIBRARIES    - List of libraries when using opus.
#  OPUS_FOUND        - True if Opus found.

set(OPUS_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/libsndfile/plswork/include)
set(OPUS_LIBRARIES opus)
set(OPUS_FOUND 1)

add_library(opus STATIC IMPORTED)
 set_target_properties(opus PROPERTIES
 	INTERFACE_INCLUDE_DIRECTORIES "${OPUS_INCLUDE_DIRS}"
 	IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libsndfile/plswork/lib/libopus.a"
 )
