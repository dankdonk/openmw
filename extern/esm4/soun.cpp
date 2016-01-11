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
#include "soun.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Sound::Sound()
{
    mEditorId.clear();
    mSoundFile.clear();
}

ESM4::Sound::~Sound()
{
}

void ESM4::Sound::load(ESM4::Reader& reader)
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
                    throw std::runtime_error ("SOUN EDID data read error");
                break;
            }
            case ESM4::SUB_FNAM:
            {
                if (!reader.getZString(mSoundFile))
                    throw std::runtime_error ("SOUN FNAM data read error");
                break;
            }
            case ESM4::SUB_SNDX:
            {
                reader.get(mData);
                break;
            }
            case ESM4::SUB_SNDD:
            {
                //std::cout << "SOUN " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                std::cout << "SOUN " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                //throw std::runtime_error("ESM4::SOUSOUNoad - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Sound::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Sound::blank()
//{
//}
