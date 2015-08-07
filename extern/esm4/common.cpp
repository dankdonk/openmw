/*
  Copyright (C) 2015 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#include "common.hpp"

#include <sstream>

namespace ESM4
{
    static const char *sGroupType[] =
    {
        "Record Type", "World Child", "Interior Cell", "Interior Sub Cell", "Exterior Cell",
        "Exterior Sub Cell", "Cell Child", "Topic Child", "Cell Persistent Child",
        "Cell Temporary Child", "Cell Visible Dist Child"
    };

    std::string printLabel(const GroupLabel& label, const std::uint32_t type)
    {
        std::ostringstream ss;
        ss << std::string(sGroupType[type]);
        switch (type)
        {
            case ESM4::Grp_RecordType:
            {
                ss << ": " << std::string((char*)label.recordType, 4);
                return ss.str();
            }
            case ESM4::Grp_ExteriorCell:
            case ESM4::Grp_ExteriorSubCell:
            {
                //short x, y;
                //y = label & 0xff;
                //x = (label >> 16) & 0xff;
                ss << ": grid (x, y) " << std::dec << label.grid[1] << ", " << label.grid[0];

                return ss.str();
            }
            case ESM4::Grp_InteriorCell:
            case ESM4::Grp_InteriorSubCell:
            {
                ss << ": block 0x" << std::hex << label.value;
                return ss.str();
            }
            case ESM4::Grp_WorldChild:
            case ESM4::Grp_CellChild:
            case ESM4::Grp_TopicChild:
            case ESM4::Grp_CellPersistentChild:
            case ESM4::Grp_CellTemporaryChild:
            case ESM4::Grp_CellVisibleDistChild:
            {
                ss << ": FormId 0x" << std::hex << label.value;
                return ss.str();
            }
            default: return "";
        }
    }

    std::string printName(const std::uint32_t typeId)
    {
        unsigned char typeName[4];
        typeName[0] =  typeId        & 0xff;
        typeName[1] = (typeId >>  8) & 0xff;
        typeName[2] = (typeId >> 16) & 0xff;
        typeName[3] = (typeId >> 24) & 0xff;

        return std::string((char*)typeName, 4);
    }
}
