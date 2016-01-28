/*
  Copyright (C) 2016 cc9cii

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
#include "npc_.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Npc::Npc() : mFormId(0), mFlags(0), mBoundRadius(0.f)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
}

ESM4::Npc::~Npc()
{
}

void ESM4::Npc::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
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
                    throw std::runtime_error ("NPC_ FULL data read error");
                break;
            }
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_KFFZ:
            {
                std::string str;
                if (!reader.getZString(str))
                    throw std::runtime_error ("NPC_ KFFZ data read error");

                std::stringstream ss(str);
                std::string file;
                while (std::getline(ss, file, '\0')) // split the strings
                    mKf.push_back(file);

                break;
            }
            case ESM4::SUB_CNTO:
            {
                FormId item;
                reader.get(item);
                std::uint32_t count;
                reader.get(count);
                //if (mFormId == 0x0004b939) //FIXME Necromancer Wellspring cave
                    //std::cout << formIdToString(item) << " " << count << std::endl;
                if (mFormId == 0x0000bfdf) //FIXME Anvil Guard
                    std::cout << formIdToString(item) << " " << count << std::endl;

                break;
            }
            case ESM4::SUB_ACBS:
            case ESM4::SUB_SNAM:
            case ESM4::SUB_INAM:
            case ESM4::SUB_RNAM:
            case ESM4::SUB_SPLO:
            case ESM4::SUB_SCRI:
            case ESM4::SUB_AIDT:
            case ESM4::SUB_PKID:
            case ESM4::SUB_CNAM:
            case ESM4::SUB_DATA:
            case ESM4::SUB_HNAM:
            case ESM4::SUB_LNAM:
            case ESM4::SUB_ENAM:
            case ESM4::SUB_HCLR:
            case ESM4::SUB_ZNAM:
            case ESM4::SUB_FGGS:
            case ESM4::SUB_FGGA:
            case ESM4::SUB_FGTS:
            case ESM4::SUB_FNAM:
            case ESM4::SUB_ATKR:
            case ESM4::SUB_COCT:
            case ESM4::SUB_CRIF:
            case ESM4::SUB_CSCR:
            case ESM4::SUB_CSDC:
            case ESM4::SUB_CSDI:
            case ESM4::SUB_CSDT:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_DOFT:
            case ESM4::SUB_DPLT:
            case ESM4::SUB_ECOR:
            case ESM4::SUB_ANAM:
            case ESM4::SUB_ATKD:
            case ESM4::SUB_ATKE:
            case ESM4::SUB_DEST:
            case ESM4::SUB_DSTD:
            case ESM4::SUB_DSTF:
            case ESM4::SUB_FTST:
            case ESM4::SUB_HCLF:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_NAM5:
            case ESM4::SUB_NAM6:
            case ESM4::SUB_NAM7:
            case ESM4::SUB_NAM8:
            case ESM4::SUB_NAM9:
            case ESM4::SUB_NAMA:
            case ESM4::SUB_OBND:
            case ESM4::SUB_PNAM:
            case ESM4::SUB_PRKR:
            case ESM4::SUB_PRKZ:
            case ESM4::SUB_QNAM:
            case ESM4::SUB_SOFT:
            case ESM4::SUB_SPCT:
            case ESM4::SUB_TIAS:
            case ESM4::SUB_TINC:
            case ESM4::SUB_TINI:
            case ESM4::SUB_TINV:
            case ESM4::SUB_TPLT:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_VTCK:
            case ESM4::SUB_WNAM:
            case ESM4::SUB_GNAM:
            case ESM4::SUB_SHRT:
            case ESM4::SUB_SPOR:
            {
                //std::cout << "NPC_ " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::NPC_::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Npc::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Npc::blank()
//{
//}
