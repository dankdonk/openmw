cmake_minimum_required(VERSION 2.8)

# This is NOT intended as a stand-alone build system! Instead, you should include this from the main CMakeLists of your project.

set(STRVERSCMP_LIBRARY "strverscmp")

# Sources
set(SOURCE_FILES
    strverscmp.c
)

add_library(${STRVERSCMP_LIBRARY} STATIC ${SOURCE_FILES})

set(STRVERSCMP_LIBRARIES ${STRVERSCMP_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(STRVERSCMP_LIBRARIES ${STRVERSCMP_LIBRARIES} PARENT_SCOPE)
