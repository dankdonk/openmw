#
#  Copyright (C) 2015-2017 cc9cii
#
#  This software is provided 'as-is', without any express or implied
#  warranty.  In no event will the authors be held liable for any damages
#  arising from the use of this software.
#
#  Permission is granted to anyone to use this software for any purpose,
#  including commercial applications, and to alter it and redistribute it
#  freely, subject to the following restrictions:
#
#  1. The origin of this software must not be misrepresented; you must not
#     claim that you wrote the original software. If you use this software
#     in a product, an acknowledgment in the product documentation would be
#     appreciated but is not required.
#  2. Altered source versions must be plainly marked as such, and must not be
#     misrepresented as being the original software.
#  3. This notice may not be removed or altered from any source distribution.
#
#  cc9cii cc9c@iinet.net.au
#

set(NIBTOGRE_LIBRARY "nibtogre")

set(NIBTOGRE_SOURCE_FILES
    nistream.cpp
    header.cpp
    nimodel.cpp

    niobject.cpp
    niavobject.cpp
    nicollisionobject.cpp
    nidata.cpp
    niinterpolator.cpp
    niobjectnet.cpp
    niparticlemodifier.cpp
    niproperty.cpp
    nipsyscollider.cpp
    nipsysmodifier.cpp
    nisequence.cpp
    nitimecontroller.cpp
    bhkrefobject.cpp
)

add_library(${NIBTOGRE_LIBRARY} STATIC ${NIBTOGRE_SOURCE_FILES})
set(NIBTOGRE_LIBRARIES ${NIBTOGRE_LIBRARY})

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(NIBTOGRE_LIBRARIES ${NIBTOGRE_LIBRARIES} PARENT_SCOPE)
