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
#include "idle.hpp"

#include <cassert>
#include <stdexcept>

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::IdleAnimation::IdleAnimation() : mFormId(0), mFlags(0), mParent(0), mPrevious(0)
{
    mEditorId.clear();
    mCollision.clear();
    mEvent.clear();
}

ESM4::IdleAnimation::~IdleAnimation()
{
}

void ESM4::IdleAnimation::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId);  break;
            case ESM4::SUB_DNAM: reader.getZString(mCollision); break;
            case ESM4::SUB_ENAM: reader.getZString(mEvent);     break;
            case ESM4::SUB_ANAM:
            {
                reader.get(mParent);
                reader.get(mPrevious);
                break;
            }
            case ESM4::SUB_CTDA:
            case ESM4::SUB_DATA:
            {
                //std::cout << "IDLE " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::IDLE::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::IdleAnimation::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::IdleAnimation::blank()
//{
//}
