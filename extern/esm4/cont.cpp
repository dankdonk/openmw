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
#include "cont.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Container::Container() : mDataFlags(0), mWeight(0.f), mOpenSound(0), mCloseSound(0),
                               mScript(0), mItem(0), mItemCount(0)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();
}

ESM4::Container::~Container()
{
}

void ESM4::Container::load(ESM4::Reader& reader)
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
                    throw std::runtime_error ("CONT EDID data read error");
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
                    throw std::runtime_error ("CONT FULL data read error");
                break;
            }
            case ESM4::SUB_MODL:
            {
                if (!reader.getZString(mModel))
                    throw std::runtime_error ("CONT MODL data read error");

                //if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                //{
                    // read MODT/MODS here?
                //}
                break;
            }
            case ESM4::SUB_DATA:
            {
                reader.get(mDataFlags);
                reader.get(mWeight);
                break;
            }
            case ESM4::SUB_SNAM:
            {
                reader.get(mOpenSound);
                break;
            }
            case ESM4::SUB_QNAM:
            {
                reader.get(mCloseSound);
                break;
            }
            case ESM4::SUB_CNTO:
            {
                reader.get(mItem);
                reader.get(mItemCount);
                break;
            }
            case ESM4::SUB_MODB:
            case ESM4::SUB_MODT:
            case ESM4::SUB_SCRI:
            case ESM4::SUB_MODS: // TES5 only
            case ESM4::SUB_VMAD: // TES5 only
            case ESM4::SUB_OBND: // TES5 only
            case ESM4::SUB_COCT: // TES5 only
            case ESM4::SUB_COED: // TES5 only
            {
                //std::cout << "CONT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::CONT::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Container::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Container::blank()
//{
//}
