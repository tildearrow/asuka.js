set (Vorbis_Vorbis_LIBRARY vorbis)
set (Vorbis_Enc_LIBRARY vorbisenc)
set (Vorbis_File_LIBRARY vorbisfile)

set (Vorbis_Vorbis_LIBRARIES vorbis)
set (Vorbis_Enc_LIBRARIES vorbisenc)
set (Vorbis_File_LIBRARIES vorbisfile)

set (Vorbis_Vorbis_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libvorbis/include")
set (Vorbis_Enc_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libvorbis/include")
set (Vorbis_File_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libvorbis/include")

set (Vorbis_Vorbis_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/libvorbis/include")
set (Vorbis_Enc_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/libvorbis/include")
set (Vorbis_File_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/libvorbis/include")

set(Enc vorbisenc)

set (Ogg_FOUND TRUE)

set (Vorbis_FOUND TRUE)
set (Vorbis_Vorbis_FOUND TRUE)
set (Vorbis_Enc_FOUND TRUE)
set (Vorbis_File_FOUND TRUE)

add_library(vorbis STATIC IMPORTED)
add_library(vorbisenc STATIC IMPORTED)
 set_target_properties(vorbis PROPERTIES
 	INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_Vorbis_INCLUDE_DIRS}"
 	IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libsndfile/plswork/lib/libvorbis.a"
 )
 set_target_properties(vorbisenc PROPERTIES
 	INTERFACE_INCLUDE_DIRECTORIES "${Vorbis_Enc_INCLUDE_DIRS}"
 	IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/libsndfile/plswork/lib/libvorbisenc.a"
 )
