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
#include "loadwrld.hpp"

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif
#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

unsigned int ESM4::World::sRecordId = ESM4::REC_WRLD;

ESM4::World::World()
{
}

ESM4::World::~World()
{
}

void ESM4::World::load(ESM4::Reader& reader)
{
    std::uint32_t subSize = 0; // for XXXX sub record

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                if (!reader.getZString(mEditorId, subHdr.dataSize))
                    throw std::runtime_error ("WRLD EDID data read error");

                assert((size_t)subHdr.dataSize-1 == mEditorId.size() && "WRLD EDID string size mismatch");
                std::cout << "Editor Id: " << mEditorId << std::endl;
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
                    break;
                }

                if (!reader.getZString(mFullName, subHdr.dataSize))
                    throw std::runtime_error ("WRLD FULL data read error");

                assert((size_t)subHdr.dataSize-1 == mFullName.size() && "WRLD FULL string size mismatch");
                std::cout << "Full Name: " << mFullName << std::endl;
                break;
            }
            case ESM4::SUB_RNAM: // multiple
            case ESM4::SUB_MHDT:
            case ESM4::SUB_WCTR:
            case ESM4::SUB_LTMP:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_CNAM:
            case ESM4::SUB_NAM2:
            case ESM4::SUB_NAM3:
            case ESM4::SUB_NAM4:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_MODL:
            case ESM4::SUB_MNAM:
            case ESM4::SUB_ONAM:
            case ESM4::SUB_NAMA:
            case ESM4::SUB_DATA:
            case ESM4::SUB_NAM0:
            case ESM4::SUB_NAM9:
            case ESM4::SUB_WNAM:
            case ESM4::SUB_PNAM:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_UNAM:
            case ESM4::SUB_ZNAM:
            case ESM4::SUB_XWEM:
            case ESM4::SUB_MODT: // from Dragonborn onwards?
            case ESM4::SUB_ICON: // Oblivion only?
            case ESM4::SUB_SNAM: // Oblivion only?
            {
                reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip
                break;
            }
            case ESM4::SUB_OFST:
            {
                if (subSize)
                {
                    reader.skipSubRecordData(subSize); // special post XXXX
                    subSize = 0;
                }
                else
                    reader.skipSubRecordData(); // FIXME: process the subrecord rather than skip

                break;
            }
            case ESM4::SUB_XXXX:
            {
                reader.get(subSize);
                break;
            }
            default:
                throw std::runtime_error("ESM4::World::load - Unknown subrecord");
        }
    }
}

//void ESM4::World::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::World::blank()
//{
//}
