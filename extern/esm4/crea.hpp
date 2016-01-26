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
#ifndef ESM4_CREA_H
#define ESM4_CREA_H

#include <string>
#include <cstdint>
#include <vector>

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Creature
    {
#pragma pack(push, 1)
        struct Inventory
        {
            FormId        item;
            std::uint32_t count;
        };

        struct AIData
        {
            std::uint8_t  aggression;
            std::uint8_t  confidence;
            std::uint8_t  energyLevel;
            std::uint8_t  responsibility;
            std::uint32_t aiFlags;
            std::uint8_t  trainSkill;
            std::uint8_t  trainLevel;
            std::uint16_t unknown;
        };

        struct Data
        {
            std::uint8_t  unknown;
            std::uint8_t  combat;
            std::uint8_t  magic;
            std::uint8_t  stealth;
            std::uint16_t soul;
            std::uint16_t health;
            std::uint16_t unknown2;
            std::uint16_t damage;
            std::uint8_t  strength;
            std::uint8_t  intelligence;
            std::uint8_t  willpower;
            std::uint8_t  agility;
            std::uint8_t  speed;
            std::uint8_t  endurance;
            std::uint8_t  personality;
            std::uint8_t  luck;
        };
#pragma pack(pop)

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        std::string mModel;

        FormId mDeathItem;
        FormId mSpell;
        FormId mScript;
        Inventory mInventory;
        AIData mAIData;
        FormId mAIPackages;
        Data   mData;
        FormId mCombatStyle;
        FormId mSoundBase;
        FormId mSound;
        std::uint8_t mSoundChance;
        float mBaseScale;
        float mTurningSpeed;
        float mFootWeight;
        std::string mBloodSpray;
        std::string mBloodDecal;

        float mBoundRadius;
        std::vector<std::string> mNif; // NIF filenames, get directory from mModel
        std::vector<std::string> mKf;

        Creature();
        ~Creature();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& reader) const;

        //void blank();
    };
}

#endif // ESM4_CREA_H
