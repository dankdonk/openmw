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
#include "cell.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Cell::Cell()
{
    mFullName.clear();
}

ESM4::Cell::~Cell()
{
}

void ESM4::Cell::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;
    mParent = reader.currWorld();

    reader.setCurrCell(mFormId); // save for LAND later
    reader.clearCellGrid(); // clear until XCLC FIXME: somehow do this automatically?

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error ("CELL EDID data read error");
//#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL Editor ID: " << mEditorId << std::endl;
//#endif
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
                //         7        56 : 63
                //         8        64 : 71

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
                std::cout << padding << "CELL formId " << std::hex << reader.hdr().record.id << std::endl;
                std::cout << padding << "CELL X " << std::dec << x << ", Y " << y << std::endl;
//#endif
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    reader.get(flags); // not in Obvlivion

                // Remember cell grid for later (loading LAND, NAVM which should be CELL temporary children)
                // Note that grids only apply for external cells.  For interior cells use the cell's formid.
                ESM4::CellGrid currCell;
                currCell.grid.x = (int16_t)x;
                currCell.grid.y = (int16_t)y;
                reader.setCurrCell(currCell);
                break;
            }
            case ESM4::SUB_FULL:
            {
                // NOTE: checking flags does not work, Skyrim.esm does not set the localized flag
                //
                // A possible hack is to look for SUB_FULL subrecord size of 4 to indicate that
                // a lookup is required.  This obviously does not work for a string size of 3,
                // but the chance of having that is assumed to be low.
                if ((reader.hdr().record.flags & Rec_Localized) != 0 || subHdr.dataSize == 4)
                {
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                    mFullName = "FIXME";
                    break;
                }

                if (!reader.getZString(mFullName))
                    throw std::runtime_error ("CELL FULL data read error");
//#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "Name: " << mFullName << std::endl;
//#endif
                break;
            }
            case ESM4::SUB_DATA:
            {
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    if (subHdr.dataSize == 2)
                        reader.get(mCellFlags);
                    else
                    {
                        assert(subHdr.dataSize == 1 && "CELL unexpected DATA flag size");
                        reader.get((std::uint8_t&)mCellFlags);
                    }
                else
                {
                    reader.get((std::uint8_t&)mCellFlags); // 8 bits in Obvlivion
                }
//#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding  << "flags: " << std::hex << mCellFlags << std::endl;
//#endif
                break;
            }
            case ESM4::SUB_XCLR:
            {
                mRegions.resize(subHdr.dataSize/sizeof(std::uint32_t));
                for (std::vector<std::uint32_t>::iterator it = mRegions.begin(); it != mRegions.end(); ++it)
                {
                    reader.get(*it);
//#if 0
                    std::string padding = "";
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "region: " << std::hex << *it << std::endl;
//#endif
                }
                break;
            }
            case ESM4::SUB_XCLL:
            case ESM4::SUB_TVDT:
            case ESM4::SUB_MHDT:
            case ESM4::SUB_XCGD:
            case ESM4::SUB_LTMP:
            case ESM4::SUB_LNAM:
            case ESM4::SUB_XCLW:
            case ESM4::SUB_XNAM:
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
                //std::cout << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::CELL::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Cell::save(ESM4::Writer& writer) const
//{
//}

void ESM4::Cell::blank()
{
}
