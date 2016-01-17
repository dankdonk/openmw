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
#include "achr.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Character::Character() : mBaseObj(0), mScale(1.f)
{
    mEditorId.clear();
    mFullName.clear();
}

ESM4::Character::~Character()
{
}

void ESM4::Character::load(ESM4::Reader& reader)
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
                    throw std::runtime_error ("ACHR EDID data read error");
                break;
            }
            case ESM4::SUB_FULL:
            {
                if (!reader.getZString(mFullName))
                    throw std::runtime_error ("ACHR FULL data read error");
                break;
            }
            case ESM4::SUB_NAME:
            {
                reader.get(mBaseObj);
                break;
            }
            case ESM4::SUB_DATA:
            {
                reader.get(mPosition);
                break;
            }
            case ESM4::SUB_XSCL:
            {
                reader.get(mScale);
                break;
            }
            case ESM4::SUB_XRGD: // ragdoll
            case ESM4::SUB_XESP: // parent obj
            case ESM4::SUB_XHRS:
            case ESM4::SUB_XMRC:
            case ESM4::SUB_XPCI:
            case ESM4::SUB_XLOD:
            case ESM4::SUB_INAM:
            case ESM4::SUB_PDTO:
            case ESM4::SUB_VMAD:
            case ESM4::SUB_XAPD:
            case ESM4::SUB_XAPR:
            case ESM4::SUB_XEZN:
            case ESM4::SUB_XHOR:
            case ESM4::SUB_XIS2:
            case ESM4::SUB_XLCM:
            case ESM4::SUB_XLCN:
            case ESM4::SUB_XLKR:
            case ESM4::SUB_XLRT:
            case ESM4::SUB_XOWN:
            case ESM4::SUB_XPPA:
            case ESM4::SUB_XPRD:
            case ESM4::SUB_XRGB:
            {
                //std::cout << "ACHR " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::ACHR::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Character::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Character::blank()
//{
//}
