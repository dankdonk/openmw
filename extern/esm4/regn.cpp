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
#include "regn.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Region::Region()
{
}

ESM4::Region::~Region()
{
}

void ESM4::Region::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: // Editor name or the worldspace
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error ("REGN EDID data read error");

                assert((size_t)subHdr.dataSize-1 == mEditorId.size() && "REGN EDID string size mismatch");
                std::cout << "Editor Id: " << mEditorId << std::endl; // FIXME: temp
                break;
            }
            case ESM4::SUB_RCLR:
            {
                reader.get(mColour);
                break;
            }
            case ESM4::SUB_WNAM:
            {
                reader.get(mWorldId);
                break;
            }
            case ESM4::SUB_ICON:
            {
                if (!reader.getZString(mShader))
                    throw std::runtime_error ("REGN ICON data read error");

                assert((size_t)subHdr.dataSize-1 == mShader.size() && "REGN ICON string size mismatch");
                break;
            }
            case ESM4::SUB_RPLI:
            case ESM4::SUB_RPLD:
            case ESM4::SUB_RDAT:
            case ESM4::SUB_RDMD: // Only in Oblivion?
            case ESM4::SUB_RDSD: // Only in Oblivion?  Possibly the same as RDSA
            case ESM4::SUB_RDGS: // Only in Oblivion?
            case ESM4::SUB_RDMO:
            case ESM4::SUB_RDSA:
            case ESM4::SUB_RDWT:
            case ESM4::SUB_RDOT:
            case ESM4::SUB_RDMP:
            {
                reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                break;
            }
            default:
                throw std::runtime_error("ESM4::Region::load - Unknown subrecord");
        }
    }
}

//void ESM4::Region::save(ESM4::Writer& writer) const
//{
//}

void ESM4::Region::blank()
{
}
