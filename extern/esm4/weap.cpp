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
#include "weap.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Weapon::Weapon() : mFormId(0), mFlags(0), mBoundRadius(0.f), mScript(0),
                         mEnchantmentPoints(0), mEnchantment(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
    mIcon.clear();
}

ESM4::Weapon::~Weapon()
{
}

void ESM4::Weapon::load(ESM4::Reader& reader)
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
                    throw std::runtime_error ("WEAP FULL data read error");
                break;
            }
            case ESM4::SUB_DATA:
            {
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                {
                    reader.get(mData.value);
                    reader.get(mData.weight);
                    reader.get(mData.damage);
                }
                else
                {
                    reader.get(mData.type);
                    reader.get(mData.speed);
                    reader.get(mData.reach);
                    reader.get(mData.flags);
                    reader.get(mData.value);
                    reader.get(mData.health);
                    reader.get(mData.weight);
                    reader.get(mData.damage);
                }
                break;
            }
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_ICON: reader.getZString(mIcon);  break;
            case ESM4::SUB_SCRI: reader.get(mScript);       break;
            case ESM4::SUB_ANAM: reader.get(mEnchantmentPoints); break;
            case ESM4::SUB_ENAM: reader.get(mEnchantment);  break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_MODT:
            case ESM4::SUB_BAMT:
            case ESM4::SUB_BIDS:
            case ESM4::SUB_INAM:
            case ESM4::SUB_CNAM:
            case ESM4::SUB_CRDT:
            case ESM4::SUB_DESC:
            case ESM4::SUB_DNAM:
            case ESM4::SUB_EAMT:
            case ESM4::SUB_EITM:
            case ESM4::SUB_ETYP:
            case ESM4::SUB_KSIZ:
            case ESM4::SUB_KWDA:
            case ESM4::SUB_NAM8:
            case ESM4::SUB_NAM9:
            case ESM4::SUB_OBND:
            case ESM4::SUB_SNAM:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_UNAM:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_VNAM:
            case ESM4::SUB_WNAM:
            case ESM4::SUB_XNAM: // Dawnguard only?
            case ESM4::SUB_NNAM:
            case ESM4::SUB_MODS:
            {
                //std::cout << "WEAP " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::WEAP::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Weapon::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Weapon::blank()
//{
//}
