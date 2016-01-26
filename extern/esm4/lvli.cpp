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
#include "lvli.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::LeveledItem::LeveledItem() : mFormId(0), mFlags(0), mChanceNone(0), mLvlItemFlags(0), mData(0)
{
    mEditorId.clear();
}

ESM4::LeveledItem::~LeveledItem()
{
}

void ESM4::LeveledItem::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_LVLD: reader.get(mChanceNone);   break;
            case ESM4::SUB_LVLF: reader.get(mLvlItemFlags); break;
            case ESM4::SUB_DATA: reader.get(mData);         break;
            case ESM4::SUB_LVLO:
            {
                static LVLO lvlo;
                if (subHdr.dataSize != 12)
                {
                    if (subHdr.dataSize == 8)
                    {
                        reader.get(lvlo.level);
                        reader.get(lvlo.item);
                        reader.get(lvlo.count);
                        std::cout << "LVLI " << mEditorId << " LVLO lev " << lvlo.level << ", item " << lvlo.item
                                  << ", count " << lvlo.count << std::endl;
                        break;
                    }
                    else
                        throw std::runtime_error("ESM4::LVLI::load - " + mEditorId + " LVLO size error");
                }
                else
                    reader.get(lvlo);

                mLvlObject.push_back(lvlo);
                break;
            }
            default:
                throw std::runtime_error("ESM4::LVLI::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::LeveledItem::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::LeveledItem::blank()
//{
//}
