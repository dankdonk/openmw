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
#include "refr.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "formid.hpp" // FIXME: debug only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Reference::Reference() : mFormId(0), mFlags(0), mDisabled(false), mBaseObj(0), mScale(1.f),
                               mOwner(0), mGlobal(0), mFactionRank(0), mCount(1)
{
    mEditorId.clear();
    mFullName.clear();

    mEsp.parent = 0;
    mEsp.flags = 0;

    mDoor.destDoor = 0;
}

ESM4::Reference::~Reference()
{
}

void ESM4::Reference::load(ESM4::Reader& reader)
{
    //mFormId = reader.adjustFormId(reader.hdr().record.id); // FIXME: maybe use master adjusted?
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;
    // TODO: Let the engine apply this? Saved games?
    //mDisabled = ((mFlags & ESM4::Rec_Disabled) != 0) ? true : false;

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
                    throw std::runtime_error ("REFR FULL data read error");
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "REFR Full Name: " << mFullName << std::endl;
#endif
                break;
            }
            case ESM4::SUB_NAME:
            {
                reader.getFormId(mBaseObj);
#if 0
                if (mFlags & ESM4::Rec_Disabled)
                    std::cout << "REFR disable at start " << formIdToString(mFormId) <<
                        " baseobj " << formIdToString(mBaseObj) <<
                        " " << (mEditorId.empty() ? "" : mEditorId) << std::endl; // FIXME
#endif
                break;
            }
            case ESM4::SUB_DATA: reader.get(mPosition); break;
            case ESM4::SUB_XSCL: reader.get(mScale);    break;
            case ESM4::SUB_XOWN: reader.getFormId(mOwner);    break;
            case ESM4::SUB_XGLB: reader.getFormId(mGlobal);   break;
            case ESM4::SUB_XRNK: reader.get(mFactionRank); break;
            case ESM4::SUB_XESP:
            {
                reader.get(mEsp);
                reader.adjustFormId(mEsp.parent);
                //std::cout << "REFR  parent: " << formIdToString(mEsp.parent) << " ref " << formIdToString(mFormId)
                    //<< ", 0x" << std::hex << (mEsp.flags & 0xff) << std::endl;// FIXME
                break;
            }
            case ESM4::SUB_XTEL:
            {
                reader.get(mDoor.destDoor);
                reader.get(mDoor.destPos);
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    reader.get(mDoor.flags); // not in Obvlivion
                //std::cout << "REFR  dest door: " << formIdToString(mDoor.destDoor) << std::endl;// FIXME
                break;
            }
            case ESM4::SUB_XSED:
            {
                // 1 or 4 bytes
                if (subHdr.dataSize == 1)
                {
                    uint8_t data = reader.get(data);
                    //std::cout << "REFR XSED " << std::hex << (int)data << std::endl;
                    break;
                }
                else if (subHdr.dataSize == 4)
                {
                    uint32_t data = reader.get(data);
                    //std::cout << "REFR XSED " << std::hex << (int)data << std::endl;
                    break;
                }

                //std::cout << "REFR XSED dataSize: " << subHdr.dataSize << std::endl;// FIXME
                reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_XLOD:
            {
                // 12 bytes
                if (subHdr.dataSize == 12)
                {
                    uint32_t data = reader.get(data);
                    uint32_t data2 = reader.get(data);
                    uint32_t data3 = reader.get(data);
                    //std::cout << "REFR XLOD " << std::hex << (int)data << " " << (int)data2 << " " << (int)data3 << std::endl;
                    break;
                }
                //std::cout << "REFR XLOD dataSize: " << subHdr.dataSize << std::endl;// FIXME
                reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_XACT:
            {
                if (subHdr.dataSize == 4)
                {
                    uint32_t data = reader.get(data);
                    //std::cout << "REFR XACT " << std::hex << (int)data << std::endl;
                    break;
                }

                //std::cout << "REFR XACT dataSize: " << subHdr.dataSize << std::endl;// FIXME
                reader.skipSubRecordData();
                break;
            }
            // seems like another ref, e.g. 00064583 has base object 00000034 which is "XMarkerHeading"
            case ESM4::SUB_XRTM: // formId
            {
                FormId id;
                reader.get(id);
                std::cout << "REFR XRTM : " << formIdToString(id) << std::endl;// FIXME
                break;
            }
            // lighting
            case ESM4::SUB_LNAM: // lighting template formId
            case ESM4::SUB_XLIG: // struct, FOV, fade, etc
            case ESM4::SUB_XEMI: // LIGH formId
            case ESM4::SUB_XRDS: // Radius or Radiance
            case ESM4::SUB_XRGB:
            case ESM4::SUB_XRGD: // tangent data?
            case ESM4::SUB_XALP: // alpha cutoff
            //
            case ESM4::SUB_XLOC: // formId
            case ESM4::SUB_XMRK:
            case ESM4::SUB_FNAM:
            case ESM4::SUB_XTRG: // formId
            case ESM4::SUB_XPCI: // formId
            case ESM4::SUB_XLCM:
            case ESM4::SUB_XCNT:
            case ESM4::SUB_TNAM:
            case ESM4::SUB_ONAM:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_XPRM:
            case ESM4::SUB_INAM:
            case ESM4::SUB_PDTO:
            case ESM4::SUB_SCHR:
            case ESM4::SUB_SCTX:
            case ESM4::SUB_XAPD:
            case ESM4::SUB_XAPR:
            case ESM4::SUB_XCVL:
            case ESM4::SUB_XCZA:
            case ESM4::SUB_XCZC:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XFVC:
            case ESM4::SUB_XHTW:
            case ESM4::SUB_XIS2:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_XLIB:
            case ESM4::SUB_XLKR:
            case ESM4::SUB_XLRM:
            case ESM4::SUB_XLRT:
            case ESM4::SUB_XLTW:
            case ESM4::SUB_XMBO:
            case ESM4::SUB_XMBP:
            case ESM4::SUB_XMBR:
            case ESM4::SUB_XNDP:
            case ESM4::SUB_XOCP:
            case ESM4::SUB_XPOD:
            case ESM4::SUB_XPPA:
            case ESM4::SUB_XPRD:
            case ESM4::SUB_XPWR:
            case ESM4::SUB_XRMR:
            case ESM4::SUB_XSPC:
            case ESM4::SUB_XTNM:
            case ESM4::SUB_XTRI:
            case ESM4::SUB_XWCN:
            case ESM4::SUB_XWCU:
            case ESM4::SUB_XATR: // Dawnguard only?
            case MKTAG('X','H','L','T'): // Unofficial Oblivion Patch
            case MKTAG('X','C','H','G'): // thievery.exp
            {
                //std::cout << "REFR " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::REFR::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Reference::save(ESM4::Writer& writer) const
//{
//}

void ESM4::Reference::blank()
{
}
