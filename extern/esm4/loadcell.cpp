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
#include "loadcell.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif
#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Cell::Cell()
{
}

ESM4::Cell::~Cell()
{
}

void ESM4::Cell::load(ESM4::Reader& reader)
{
    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                std::string editorId;
                if (!reader.getZString(editorId, subHdr.dataSize))
                    throw std::runtime_error ("CELL EDID data read error");

                assert((size_t)subHdr.dataSize-1 == editorId.size() && "CELL EDID string size mismatch");
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "Editor Id: " << editorId << std::endl;
                break;
            }
            case ESM4::SUB_XCLC:
            {
                // group grid   cell grid range
                //
                //        -7       -56 :-49
                //        -6       -48 :-41
                //        -5       -40 :-33
                //        -4       -32 :-25
                //        -3       -24 :-17
                //        -2       -16 : -9
                //        -1        -8 : -1
                //         0         0 :  7
                //         1         8 : 15
                //         2        16 : 23
                //         3        24 : 31
                //         4        32 : 39
                //         5        40 : 47
                //         6        48 : 55

                //(X, Y) grid location of the cell followed by flags. Always in
                //exterior cells and never in interior cells.
                //
                //    int32 - X
                //    int32 - Y
                //    uint32 - flags (high bits look random)
                //
                //        0x1 - Force Hide Land Quad 1
                //        0x2 - Force Hide Land Quad 2
                //        0x4 - Force Hide Land Quad 3
                //        0x8 - Force Hide Land Quad 4
                int32_t x;
                int32_t y;
                uint32_t flags;
                reader.get(x);
                reader.get(y);
//#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL group " << ESM4::printLabel(reader.grp().label, reader.grp().type) << std::endl;
                std::cout << padding << "CELL X " << std::dec << x << ", Y " << y << std::endl;
//#endif
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    reader.get(flags); // not in Obvlivion
                // HACK // FIXME
                // Remember cell grid for later (loading LAND, NAVM which should be CELL temporary children)
                // Note that grids only apply for external cells.  For interior cells use the cell's formid.
                ESM4::CellGrid currCell;
                currCell.grid.x = (int16_t)x;
                currCell.grid.y = (int16_t)y;
                reader.setCurrCell(currCell);
                break;
            }
            case ESM4::SUB_FULL:
            case ESM4::SUB_DATA:
            case ESM4::SUB_XCLL:
            case ESM4::SUB_TVDT:
            case ESM4::SUB_MHDT:
            case ESM4::SUB_XCGD:
            case ESM4::SUB_LTMP:
            case ESM4::SUB_LNAM:
            case ESM4::SUB_XCLW:
            case ESM4::SUB_XNAM:
            case ESM4::SUB_XCLR:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_XCWT:
            case ESM4::SUB_XWCS:
            case ESM4::SUB_XWCU:
            case ESM4::SUB_XWCN:
            case ESM4::SUB_XCAS:
            case ESM4::SUB_XCIM:
            case ESM4::SUB_XCMO:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XOWN:
            case ESM4::SUB_XCCM:
            case ESM4::SUB_XWEM:
            case ESM4::SUB_XILL:
            case ESM4::SUB_XCMT: // Oblivion only?
            case ESM4::SUB_XRNK: // Oblivion only?
            case ESM4::SUB_XGLB: // Oblivion only?
            {
                //std::cout << ESM4::printName(reader.subRecordHeader().typeId)
                          //<< " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                std::cout << ESM4::printName(reader.subRecordHeader().typeId)
                          << " unhandled TES4 CELL field" << std::endl;
                throw std::runtime_error("ESM4::Cell::load - Unknown subrecord");
        }
    }
}

//void ESM4::Cell::save(ESM4::Writer& writer) const
//{
//}
