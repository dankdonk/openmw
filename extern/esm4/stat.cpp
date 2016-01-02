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
#include "stat.hpp"

#include <cassert>
#include <stdexcept>

#include <iostream> // FIXME: debug only
#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Static::Static()
{
}

ESM4::Static::~Static()
{
}

void ESM4::Static::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                if (!reader.getZString(mEditorId))
                    throw std::runtime_error ("STAT EDID data read error");
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "STAT Editor ID: " << mEditorId << std::endl;
#endif
                break;
            }
            case ESM4::SUB_MODL:
            {
                if (!reader.getZString(mModel))
                    throw std::runtime_error ("STAT MODL data read error");
#if 0
                std::string padding = "";
                padding.insert(0, reader.stackSize()*2, ' ');
                std::cout << padding << "Model: " << mModel << std::endl;
#endif
                break;
            }
            case ESM4::SUB_MODB:
            {
                mMODB.resize(subHdr.dataSize/sizeof(std::uint8_t));
                for (std::vector<std::uint8_t>::iterator it = mMODB.begin(); it != mMODB.end(); ++it)
                {
                    reader.get(*it);
#if 0
                    std::string padding = "";
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "MODB: " << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_MODT:
            {
                // version is only availabe in TES5
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                    std::cout << "STAT MODT ver: " << std::hex << reader.hdr().record.version << std::endl;

                // for TES4 these are just a sequence of bytes
                mMODT.resize(subHdr.dataSize/sizeof(std::uint8_t));
                for (std::vector<std::uint8_t>::iterator it = mMODT.begin(); it != mMODT.end(); ++it)
                {
                    reader.get(*it);
#if 0
                    std::string padding = "";
                    padding.insert(0, reader.stackSize()*2, ' ');
                    std::cout << padding  << "MODT: " << std::hex << *it << std::endl;
#endif
                }
                break;
            }
            case ESM4::SUB_OBND:
            case ESM4::SUB_DNAM:
            {
                reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_MNAM:
            case ESM4::SUB_MODS:
            {
                std::cout << "STAT " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::STAT::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Static::save(ESM4::Writer& writer) const
//{
//}

void ESM4::Static::blank()
{
}
