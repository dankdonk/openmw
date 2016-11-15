/*
  Copyright (C) 2015, 2016 cc9cii

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

ESM4::Cell::Cell() : mParent(0), mFormId(0), mFlags(0), mCellFlags(0), mX(0), mY(0), mOwner(0),
                     mGlobal(0), mClimate(0), mWater(0), mWaterHeight(0.f), mPreloaded(false)
{
    mEditorId.clear();
    mFullName.clear();

    mLighting.ambient = 0;
    mLighting.directional = 0;
    mLighting.fogNear = 0;
    mLighting.unknown1 = 0.f;
    mLighting.unknown2 = 0.f;
    mLighting.unknown3 = 0;
    mLighting.unknown4 = 0;
    mLighting.unknown5 = 0.f;
    mLighting.unknown6 = 0.f;
}

ESM4::Cell::~Cell()
{
}

void ESM4::Cell::init(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;
    mParent = reader.currWorld();

    reader.clearCellGrid(); // clear until XCLC FIXME: somehow do this automatically?

    // Sometimes cell 0,0 does not have an XCLC sub record (e.g. ToddLand 000009BF)
    // To workaround this issue put a default value if group is "exterior sub cell" and its
    // grid is "0 0".  Note the reversed X/Y order (no matter since they're both 0 anyway).
    if (reader.grp().type == ESM4::Grp_ExteriorSubCell
            && reader.grp().label.grid[1] == 0 && reader.grp().label.grid[0] == 0)
    {
        ESM4::CellGrid currCell;
        currCell.grid.x = 0;
        currCell.grid.y = 0;
        reader.setCurrCell(currCell);  // side effect: sets mCellGridValid  true
    }
}

// Only load EDID and XCLC (along with mFormId, mFlags and mParent)
bool ESM4::Cell::preload(ESM4::Reader& reader, ESM4::ReaderContext& ctx)
{
    init(reader);

    // TODO: optimise by skipping the rest of the CELL record once both EDID and XCLC are read
    // (assuming they occur early).  Also skip XCLC if internal cell.
    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error ("CELL EDID data read error");
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL Editor ID: " << mEditorId << std::endl;
#endif
                break;
            }
            case ESM4::SUB_XCLC:
            {
                //(X, Y) grid location of the cell followed by flags. Always in exterior cells
                //and never in interior cells.
                //
                //    int32 - X
                //    int32 - Y
                //    uint32 - flags (high bits look random)
                //
                //        0x1 - Force Hide Land Quad 1
                //        0x2 - Force Hide Land Quad 2
                //        0x4 - Force Hide Land Quad 3
                //        0x8 - Force Hide Land Quad 4
                uint32_t flags;
                reader.get(mX);
                reader.get(mY);
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL group " << ESM4::printLabel(reader.grp().label, reader.grp().type) << std::endl;
                std::cout << padding << "CELL formId " << std::hex << reader.hdr().record.id << std::endl;
                std::cout << padding << "CELL X " << std::dec << mX << ", Y " << mY << std::endl;
#endif
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    reader.get(flags); // not in Obvlivion

                // Remember cell grid for later (loading LAND, NAVM which should be CELL temporary children)
                // Note that grids only apply for external cells.  For interior cells use the cell's formid.
                ESM4::CellGrid currCell;
                currCell.grid.x = (int16_t)mX;
                currCell.grid.y = (int16_t)mY;
                reader.setCurrCell(currCell);
                break;
            }
            default:
            {
                reader.skipSubRecordData();
                break;
            }
        }
    }

    mPreloaded = true;
    return reader.skipNextGroupCellChild(ctx);
}

void ESM4::Cell::load(ESM4::Reader& reader)
{
    if (!mPreloaded)
        init(reader);
    else
        reader.setCurrCell(mFormId); // save for LAND (and other children) to access later

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                if (mPreloaded)
                {
                    reader.skipSubRecordData();
                    break;
                }

                if (!reader.getZString(mEditorId))
                    throw std::runtime_error ("CELL EDID data read error");
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL Editor ID: " << mEditorId << std::endl;
#endif
                break;
            }
            case ESM4::SUB_XCLC:
            {
                if (mPreloaded)
                {
                    reader.skipSubRecordData();
                    break;
                }
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
                uint32_t flags;
                reader.get(mX);
                reader.get(mY);
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "CELL group " << ESM4::printLabel(reader.grp().label, reader.grp().type) << std::endl;
                std::cout << padding << "CELL formId " << std::hex << reader.hdr().record.id << std::endl;
                std::cout << padding << "CELL X " << std::dec << mX << ", Y " << mY << std::endl;
#endif
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    reader.get(flags); // not in Obvlivion

                // Remember cell grid for later (loading LAND, NAVM which should be CELL temporary children)
                // Note that grids only apply for external cells.  For interior cells use the cell's formid.
                ESM4::CellGrid currCell;
                currCell.grid.x = (int16_t)mX;
                currCell.grid.y = (int16_t)mY;
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
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "Name: " << mFullName << std::endl;
#endif
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
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding  << "flags: " << std::hex << mCellFlags << std::endl;
#endif
                break;
            }
            case ESM4::SUB_XCLR:
            {
                mRegions.resize(subHdr.dataSize/sizeof(FormId));
                for (std::vector<FormId>::iterator it = mRegions.begin(); it != mRegions.end(); ++it)
                {
                    reader.getFormId(*it);
#if 0
                    std::string padding = "";
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "region: " << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_XOWN: reader.getFormId(mOwner);   break;
            case ESM4::SUB_XGLB: reader.getFormId(mGlobal);  break; // Oblivion only?
            case ESM4::SUB_XCCM: reader.getFormId(mClimate); break;
            case ESM4::SUB_XCWT: reader.getFormId(mWater);   break;
            case ESM4::SUB_XCLW: reader.get(mWaterHeight);   break;
            case ESM4::SUB_XCLL:
            {
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    reader.skipSubRecordData();
                else
                {
                    assert(subHdr.dataSize == 36 && "CELL lighting size error");
                    reader.get(mLighting);
                }
                break;
            }
            case ESM4::SUB_TVDT:
            case ESM4::SUB_MHDT:
            case ESM4::SUB_XCGD:
            case ESM4::SUB_LTMP:
            case ESM4::SUB_LNAM:
            case ESM4::SUB_XNAM:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_XWCS:
            case ESM4::SUB_XWCU:
            case ESM4::SUB_XWCN:
            case ESM4::SUB_XCAS:
            case ESM4::SUB_XCIM:
            case ESM4::SUB_XCMO:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XWEM:
            case ESM4::SUB_XILL:
            case ESM4::SUB_XCMT: // Oblivion only?
            case ESM4::SUB_XRNK: // Oblivion only?
            {
                //std::cout << "CELL " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
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
