set(ESM4_LIBRARY "esm4")

set(ESM4_SOURCE_FILES
    common.cpp
    loadtes4.cpp
    loadnavi.cpp
    loadwrld.cpp
    loadnavm.cpp
    loadland.cpp
    loadcell.cpp
    reader.cpp
)

add_library(${ESM4_LIBRARY} STATIC ${ESM4_SOURCE_FILES})
set(ESM4_LIBRARIES ${ESM4_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(ESM4_LIBRARIES ${ESM4_LIBRARIES} PARENT_SCOPE)
