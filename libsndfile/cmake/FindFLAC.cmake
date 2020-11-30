# - Find FLAC
# Find the native FLAC includes and libraries
#
#  FLAC_INCLUDE_DIRS - where to find FLAC headers.
#  FLAC_LIBRARIES    - List of libraries when using libFLAC.
#  FLAC_FOUND        - True if libFLAC found.
#  FLAC_DEFINITIONS  - FLAC compile definitons 

set(FLAC_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/flac/include)
set(FLAC_LIBRARIES FLAC)
set(FLAC_FOUND 1)

add_library(FLAC STATIC IMPORTED)
 set_target_properties(FLAC PROPERTIES
 	INTERFACE_INCLUDE_DIRECTORIES "${FLAC_INCLUDE_DIRS}"
 	IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libsndfile/plswork/lib/libFLAC.a"
 )
