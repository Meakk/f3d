include(FindPackageHandleStandardArgs)

find_path(FFmpeg_INCLUDE_DIR
  NAMES libavcodec/avcodec.h
)

find_library(FFmpeg_AVCODEC_LIBRARY
  NAMES avcodec libavcodec
)

find_library(FFmpeg_AVUTIL_LIBRARY
  NAMES avutil libavutil
)

find_package_handle_standard_args(FFmpeg
  REQUIRED_VARS
    FFmpeg_INCLUDE_DIR
    FFmpeg_AVCODEC_LIBRARY
    FFmpeg_AVUTIL_LIBRARY
)

if(FFmpeg_FOUND)
  set(FFmpeg_INCLUDE_DIRS ${FFmpeg_INCLUDE_DIR})

  set(FFmpeg_LIBRARIES
    ${FFmpeg_AVCODEC_LIBRARY}
    ${FFmpeg_AVUTIL_LIBRARY}
  )

  if(NOT TARGET FFmpeg::avutil)
    add_library(FFmpeg::avutil UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avutil PROPERTIES
      IMPORTED_LOCATION "${FFmpeg_AVUTIL_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
    )
  endif()

  if(NOT TARGET FFmpeg::avcodec)
    add_library(FFmpeg::avcodec UNKNOWN IMPORTED)
    set_target_properties(FFmpeg::avcodec PROPERTIES
      IMPORTED_LOCATION "${FFmpeg_AVCODEC_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${FFmpeg_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES FFmpeg::avutil
    )
  endif()
endif()

mark_as_advanced(
  FFmpeg_INCLUDE_DIR
  FFmpeg_AVCODEC_LIBRARY
  FFmpeg_AVUTIL_LIBRARY
)
