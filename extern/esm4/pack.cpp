/*
  Copyright (C) 2020 cc9cii

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

  Much of the information on the data structures are based on the information
  from Tes4Mod:Mod_File_Format and Tes5Mod:File_Formats but also refined by
  trial & error.  See http://en.uesp.net/wiki for details.

*/
#include "pack.hpp"

#include <stdexcept>
#include <iostream> // FIXME: for debugging only

#include "reader.hpp"
//#include "writer.hpp"

ESM4::AIPackage::AIPackage() : mFormId(0), mFlags(0)
{
    mEditorId.clear();

    std::memset(&mData, 0, sizeof(PKDT));
    std::memset(&mSchedule, 0, sizeof(PSDT));
    std::memset(&mLocation, 0, sizeof(PLDT));
    std::memset(&mTarget, 0, sizeof(PTDT));

    mConditions.clear();
}

ESM4::AIPackage::~AIPackage()
{
}

void ESM4::AIPackage::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_PKDT:
            {
                if (subHdr.dataSize != sizeof(PKDT) && subHdr.dataSize == 4)
                {
                    //std::cout << "skip fallout" << mEditorId << std::endl; // FIXME
                    reader.get(mData.flags);
                    mData.type = 0; // FIXME
                }
                else
                    reader.get(mData);

                break;
            }
            case ESM4::SUB_PSDT: reader.get(mSchedule); break;
            case ESM4::SUB_PLDT: reader.get(mLocation); break;
            case ESM4::SUB_PTDT: reader.get(mTarget);   break;
            case ESM4::SUB_CTDA:
            {
                static CTDA condition;
                reader.get(condition);
                mConditions.push_back(condition);

                break;
            }
            case ESM4::SUB_CTDT: // always 20
            {
                //std::cout << "PACK " << ESM4::printName(subHdr.typeId) << " skipping..."
                        //<< subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::PACK::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::AIPackage::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::AIPackage::blank()
//{
//}
