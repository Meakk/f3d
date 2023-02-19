find_package(Git QUIET)
execute_process(
    COMMAND ${GIT_EXECUTABLE} lfs ls-files -n 
    WORKING_DIRECTORY ${CMAKE_ARGV3}
    OUTPUT_VARIABLE LFS_FILES
    OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REPLACE "\n" ";" LFS_FILES ${LFS_FILES})

foreach(LFS_FILE ${LFS_FILES})
  cmake_path(GET LFS_FILE PARENT_PATH LFS_PATH)
  file(COPY "${CMAKE_ARGV3}/${LFS_FILE}" DESTINATION "${CMAKE_ARGV4}/${LFS_PATH}")
endforeach()