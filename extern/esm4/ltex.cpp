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
#include "ltex.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::LandTexture::LandTexture() : mHavokFriction(0), mHavokRestitution(0), mTextureSpecular(0),
                                   mGrass(0), mHavokMaterial(0), mTexture(0), mMaterial(0)
{
    mEditorId.clear();
    mTextureFile.clear();
}

ESM4::LandTexture::~LandTexture()
{
}

void ESM4::LandTexture::load(ESM4::Reader& reader)
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
                    throw std::runtime_error ("LTEX EDID data read error");
                break;
            }
            case ESM4::SUB_TNAM: // TES5 only
            {
                reader.get(mTexture);
                break;
            }
            case ESM4::SUB_MNAM: // TES5 only
            {
                reader.get(mMaterial);
                break;
            }
            case ESM4::SUB_HNAM:
            {
                if (reader.esmVersion() == ESM4::VER_094 || reader.esmVersion() == ESM4::VER_170)
                {
                    assert(subHdr.dataSize == 2 && "LTEX unexpected HNAM size");
                    reader.get(mHavokFriction);
                    reader.get(mHavokRestitution);
                }
                else
                {
                    assert(subHdr.dataSize == 3 && "LTEX unexpected HNAM size");
                    reader.get(mHavokMaterial);
                    reader.get(mHavokFriction);
                    reader.get(mHavokRestitution);
                }
                break;
            }
            case ESM4::SUB_ICON: // map filename, Oblivion only?
            {
                if (!reader.getZString(mTextureFile))
                    throw std::runtime_error ("LTEX ICON data read error");
                break;
            }
            case ESM4::SUB_SNAM:
            {
                reader.get(mTextureSpecular);
                break;
            }
            case ESM4::SUB_GNAM:
            {
                reader.get(mGrass);
                break;
            }
            default:
                throw std::runtime_error("ESM4::LTEX::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::LandTexture::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::LandTexture::blank()
//{
//}
