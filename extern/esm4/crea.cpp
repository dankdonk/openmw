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
#include "crea.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iostream> // FIXME

#ifdef NDEBUG // FIXME: debuggigng only
#undef NDEBUG
#endif

#include "reader.hpp"
//#include "writer.hpp"

ESM4::Creature::Creature() : mFormId(0), mFlags(0), mDeathItem(0), mSpell(0), mScript(0), mAIPackages(0),
                             mCombatStyle(0), mSoundBase(0), mSound(0), mSoundChance(0), mBaseScale(0.f),
                             mTurningSpeed(0.f), mFootWeight(0.f), mBoundRadius(0.f)
{
    mEditorId.clear();
    mFullName.clear();
    mModel.clear();

    mBloodSpray.clear();
    mBloodDecal.clear();

    mInventory.item = 0;
    mInventory.count = 0;

    mAIData.aggression = 0;
    mAIData.confidence = 0;
    mAIData.energyLevel = 0;
    mAIData.responsibility = 0;
    mAIData.aiFlags = 0;
    mAIData.trainSkill = 0;
    mAIData.trainLevel = 0;

    mData.combat = 0;
    mData.magic = 0;
    mData.stealth = 0;
    mData.soul = 0;
    mData.health = 0;
    mData.damage = 0;
    mData.strength = 0;
    mData.intelligence = 0;
    mData.willpower = 0;
    mData.agility = 0;
    mData.speed = 0;
    mData.endurance = 0;
    mData.personality = 0;
    mData.luck = 0;
}

ESM4::Creature::~Creature()
{
}

void ESM4::Creature::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    mFlags  = reader.hdr().record.flags;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID: reader.getZString(mEditorId); break;
            case ESM4::SUB_FULL: reader.getZString(mFullName); break;
            case ESM4::SUB_MODL: reader.getZString(mModel); break;
            case ESM4::SUB_INAM: reader.get(mDeathItem);    break;
            case ESM4::SUB_SPLO: reader.get(mSpell);        break;
            case ESM4::SUB_CNTO: reader.get(mInventory);    break;
            case ESM4::SUB_SCRI: reader.get(mScript);       break;
            case ESM4::SUB_AIDT: reader.get(mAIData);       break;
            case ESM4::SUB_PKID: reader.get(mAIPackages);   break;
            case ESM4::SUB_DATA: reader.get(mData);         break;
            case ESM4::SUB_ZNAM: reader.get(mCombatStyle);  break;
            case ESM4::SUB_CSCR: reader.get(mSoundBase);    break;
            case ESM4::SUB_CSDI: reader.get(mSound);        break;
            case ESM4::SUB_CSDC: reader.get(mSoundChance);  break;
            case ESM4::SUB_BNAM: reader.get(mBaseScale);    break;
            case ESM4::SUB_TNAM: reader.get(mTurningSpeed); break;
            case ESM4::SUB_WNAM: reader.get(mFootWeight);   break;
            case ESM4::SUB_MODB: reader.get(mBoundRadius);  break;
            case ESM4::SUB_NAM0: reader.getZString(mBloodSpray); break;
            case ESM4::SUB_NAM1: reader.getZString(mBloodDecal); break;
            case ESM4::SUB_NIFZ:
            {
                std::string str;
                if (!reader.getZString(str))
                    throw std::runtime_error ("CREA NIFZ data read error");

                std::stringstream ss(str);
                std::string file;
                while (std::getline(ss, file, '\0')) // split the strings
                    mNif.push_back(file);

                break;
            }
            case ESM4::SUB_NIFT:
            {
                assert(subHdr.dataSize == 4 && "CREA NIFT datasize error");
                std::uint32_t nift;
                reader.get(nift);
                if (nift)
                    std::cout << "CREA NIFT " << mFormId << ", non-zero " << nift << std::endl;
                break;
            }
            case ESM4::SUB_KFFZ:
            {
                std::string str;
                if (!reader.getZString(str))
                    throw std::runtime_error ("CREA KFFZ data read error");

                std::stringstream ss(str);
                std::string file;
                while (std::getline(ss, file, '\0')) // split the strings
                    mKf.push_back(file);

                break;
            }
            case ESM4::SUB_MODT:
            case ESM4::SUB_ACBS:
            case ESM4::SUB_SNAM:
            case ESM4::SUB_RNAM:
            case ESM4::SUB_CSDT:
            {
                //std::cout << "CREA " << ESM4::printName(subHdr.typeId) << " skipping..." << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::CREA::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Creature::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Creature::blank()
//{
//}