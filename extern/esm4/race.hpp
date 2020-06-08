/*
  Copyright (C) 2016, 2018-2020 cc9cii

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
#ifndef ESM4_RACE
#define ESM4_RACE

#include <cstdint>
#include <vector>
#include <map>

#include "formid.hpp"
#include "actor.hpp" // AttributeValues

namespace ESM4
{
    class Reader;
    class Writer;
    typedef std::uint32_t FormId;

    struct Race
    {
#pragma pack(push, 1)
        struct Data
        {
            std::uint8_t flags; // 0x01 = not playable, 0x02 = not male, 0x04 = not female, ?? = fixed
        };
#pragma pack(pop)

        enum SkillIndex
        {
            Skill_Armorer     = 0x0C,
            Skill_Athletics   = 0x0D,
            Skill_Blade       = 0x0E,
            Skill_Block       = 0x0F,
            Skill_Blunt       = 0x10,
            Skill_HandToHand  = 0x11,
            Skill_HeavyArmor  = 0x12,
            Skill_Alchemy     = 0x13,
            Skill_Alteration  = 0x14,
            Skill_Conjuration = 0x15,
            Skill_Destruction = 0x16,
            Skill_Illusion    = 0x17,
            Skill_Mysticism   = 0x18,
            Skill_Restoration = 0x19,
            Skill_Acrobatics  = 0x1A,
            Skill_LightArmor  = 0x1B,
            Skill_Marksman    = 0x1C,
            Skill_Mercantile  = 0x1D,
            Skill_Security    = 0x1E,
            Skill_Sneak       = 0x1F,
            Skill_Speechcraft = 0x20,
            Skill_None        = 0xFF,
            Skill_Unknown     = 0x00
        };

        enum HeadPartIndex // TES4
        {
            Head              = 0,
            EarMale           = 1,
            EarFemale         = 2,
            Mouth             = 3,
            TeethLower        = 4,
            TeethUpper        = 5,
            Tongue            = 6,
            EyeLeft           = 7, // no texture
            EyeRight          = 8, // no texture
            NumHeadParts      = 9
        };

        enum BodyPartIndex // TES4
        {
            UpperBody         = 0,
            LowerBody         = 1,
            Hands             = 2,
            Feet              = 3,
            Tail              = 4,
            NumBodyParts      = 5
        };

        struct BodyPart
        {
            std::string mesh;    // can be empty for arms, hands, etc
            std::string texture; // can be empty e.g. eye left, eye right
        };

        struct BodyTemplate // TES5
        {
            // 0x00000001 - Head
            // 0x00000002 - Hair
            // 0x00000004 - Body
            // 0x00000008 - Hands
            // 0x00000010 - Forearms
            // 0x00000020 - Amulet
            // 0x00000040 - Ring
            // 0x00000080 - Feet
            // 0x00000100 - Calves
            // 0x00000200 - Shield
            // 0x00000400 - Tail
            // 0x00000800 - Long Hair
            // 0x00001000 - Circlet
            // 0x00002000 - Ears
            // 0x00004000 - Body AddOn 3
            // 0x00008000 - Body AddOn 4
            // 0x00010000 - Body AddOn 5
            // 0x00020000 - Body AddOn 6
            // 0x00040000 - Body AddOn 7
            // 0x00080000 - Body AddOn 8
            // 0x00100000 - Decapitate Head
            // 0x00200000 - Decapitate
            // 0x00400000 - Body AddOn 9
            // 0x00800000 - Body AddOn 10
            // 0x01000000 - Body AddOn 11
            // 0x02000000 - Body AddOn 12
            // 0x04000000 - Body AddOn 13
            // 0x08000000 - Body AddOn 14
            // 0x10000000 - Body AddOn 15
            // 0x20000000 - Body AddOn 16
            // 0x40000000 - Body AddOn 17
            // 0x80000000 - FX01
            std::uint32_t bodyPart;
            std::uint8_t flags;
            std::uint8_t unknown1; // probably padding
            std::uint8_t unknown2; // probably padding
            std::uint8_t unknown3; // probably padding
            std::uint32_t type; // 0 = light, 1 = heavy, 2 = none (cloth?)
        };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        bool mIsTES5;

        std::string mEditorId;
        std::string mFullName;
        std::string mDesc;
        std::string mModelMale;   // TES5 skeleton (in TES4 skeleton is found in npc_)
        std::string mModelFemale; // TES5 skeleton (in TES4 skeleton is found in npc_)

        AttributeValues mAttribMale;
        AttributeValues mAttribFemale;
        std::map<SkillIndex, std::uint8_t> mSkillBonus;

        // DATA
        float mHeightMale;
        float mHeightFemale;
        float mWeightMale;
        float mWeightFemale;
        std::uint32_t mRaceFlags; // 0x0001 = playable?

        std::vector<BodyPart> mHeadParts;       // see HeadPartIndex
        std::vector<BodyPart> mHeadPartsFemale; // see HeadPartIndex

        std::vector<BodyPart> mBodyPartsMale;   // see BodyPartIndex
        std::vector<BodyPart> mBodyPartsFemale; // see BodyPartIndex

        std::vector<FormId> mEyeChoices;        // texture only
        std::vector<FormId> mHairChoices; // not for TES5

        float mFaceGenMainClamp;
        float mFaceGenFaceClamp;
        std::vector<float> mSymShapeModeCoefficients;   // should be 50
        std::vector<float> mSymShapeModeCoeffFemale;    // should be 50
        std::vector<float> mAsymShapeModeCoefficients;  // should be 30
        std::vector<float> mAsymShapeModeCoeffFemale;   // should be 30
        std::vector<float> mSymTextureModeCoefficients; // should be 50
        std::vector<float> mSymTextureModeCoeffFemale;  // should be 50

        std::map<FormId, std::int32_t> mDisposition; // race adjustments
        std::vector<FormId> mBonusSpells;            // race ability/power
        std::vector<FormId> mVNAM; // don't know what these are; 1 or 2 RACE FormIds
        std::vector<FormId> mDefaultHair; // male/female (HAIR FormId for TES4)

        std::uint32_t mNumKeywords;

        FormId mSkin; // TES5
        BodyTemplate mBodyTemplate; // TES5

        // FIXME: there's no fixed order?
        // head, mouth, eyes, brow, hair
        std::vector<FormId> mHeadPartIdsMale;   // TES5
        std::vector<FormId> mHeadPartIdsFemale; // TES5

        Race();
        virtual ~Race();

        virtual void load(ESM4::Reader& reader);
        //virtual void save(ESM4::Writer& writer) const;

        //void blank();
    };
}

#endif // ESM4_RACE
