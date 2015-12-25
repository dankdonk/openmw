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
#include "tes4.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debugging only
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "common.hpp"
#include "reader.hpp"
//#include "writer.hpp"

void ESM4::Header::load(ESM4::Reader& reader, const std::uint32_t size)
{
    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_HEDR:
            {
                if (!reader.get(mData.version) || !reader.get(mData.records) || !reader.get(mData.nextObjectId))
                    throw std::runtime_error ("TES4 HEDR data read error");

                assert((size_t)subHdr.dataSize == sizeof(mData.version)+sizeof(mData.records)+sizeof(mData.nextObjectId)
                        && "TES4 HEDR data size mismatch");
                break;
            }
            case ESM4::SUB_CNAM:
            {
                if (!reader.getZString(mAuthor))
                    throw std::runtime_error ("TES4 CNAM data read error");
                break;
            }
            case ESM4::SUB_SNAM:
            {
                if (!reader.getZString(mDesc))
                    throw std::runtime_error ("TES4 SNAM data read error");
                break;
            }
            case ESM4::SUB_MAST: // multiple
            {
                MasterData m;
                if (!reader.getZString(m.name) || !reader.getSubRecord(ESM4::SUB_DATA, m.size))
                    throw std::runtime_error ("TES4 MAST data read error");
                mMaster.push_back (m);
                break;
            }
            case ESM4::SUB_ONAM:
            case ESM4::SUB_INTV:
            case ESM4::SUB_INCC:
            case ESM4::SUB_OFST: // Oblivion only?
            case ESM4::SUB_DELE: // Oblivion only?
            {
                //std::cout << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData(); // FIXME: load/decode these
                break;
            }
            default:
                throw std::runtime_error("ESM4::Header::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Header::save(ESM4::Writer& writer)
//{
//}

//void ESM4::Header::blank()
//{
//}
