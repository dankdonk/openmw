set(ESM4_LIBRARY "esm4")

set(ESM4_SOURCE_FILES
    common.cpp
    tes4.cpp
    navi.cpp
    wrld.cpp
    navm.cpp
    land.cpp
    ltex.cpp
    cell.cpp
    regn.cpp
    stat.cpp
    refr.cpp
    anio.cpp
    cont.cpp
    misc.cpp
    acti.cpp
    armo.cpp
    npc_.cpp
    reader.cpp
)

add_library(${ESM4_LIBRARY} STATIC ${ESM4_SOURCE_FILES})
set(ESM4_LIBRARIES ${ESM4_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(ESM4_LIBRARIES ${ESM4_LIBRARIES} PARENT_SCOPE)
