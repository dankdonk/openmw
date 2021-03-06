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
#include "race.hpp"

#include <stdexcept>
#include <iostream> // FIXME: for debugging only
#include <iomanip>  // FIXME: for debugging only

#include "formid.hpp"
#include "reader.hpp"
//#include "writer.hpp"

ESM4::Race::Race() : mFormId(0), mFlags(0)
                   , mHeightMale(1.f), mHeightFemale(1.f), mWeightMale(1.f), mWeightFemale(1.f)
                   , mRaceFlags(0), mFaceGenMainClamp(0.f), mFaceGenFaceClamp(0.f), mNumKeywords(0)
{
    mEditorId.clear();
    mFullName.clear();
    mDesc.clear();
    mModelMale.clear();
    mModelFemale.clear();

    std::memset(&mAttribMale, 0, sizeof(AttributeValues));
    std::memset(&mAttribFemale, 0, sizeof(AttributeValues));

    mVNAM.resize(2);
    mDefaultHair.resize(2);
}

ESM4::Race::~Race()
{
}

void ESM4::Race::load(ESM4::Reader& reader)
{
    mFormId = reader.hdr().record.id;
    reader.adjustFormId(mFormId);
    mFlags  = reader.hdr().record.flags;

    bool isMale = false;
    bool headpart = true;
    std::uint32_t currentIndex = 0xffffffff;

    while (reader.getSubRecordHeader())
    {
        const ESM4::SubRecordHeader& subHdr = reader.subRecordHeader();
        //std::cout << "RACE " << ESM4::printName(subHdr.typeId) << std::endl;
        switch (subHdr.typeId)
        {
            case ESM4::SUB_EDID:
            {
                reader.getZString(mEditorId);
                // TES4
                // Sheogorath  0x0005308E
                // GoldenSaint 0x0001208F
                // DarkSeducer 0x0001208E
                // VampireRace 0x00000019
                // Dremora     0x00038010
                // Argonian    0x00023FE9
                // Nord        0x000224FD
                // Breton      0x000224FC
                // WoodElf     0x000223C8
                // Khajiit     0x000223C7
                // DarkElf     0x000191C1
                // Orc         0x000191C0
                // HighElf     0x00019204
                // Redguard    0x00000D43
                // Imperial    0x00000907
                break;
            }
            case ESM4::SUB_FULL:
            {
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mFullName);
                else if (!reader.getZString(mFullName))
                    throw std::runtime_error ("RACE FULL data read error");

                break;
            }
            case ESM4::SUB_DESC:
            {
                if (subHdr.dataSize == 1) // FO3?
                {
                    reader.skipSubRecordData();
                    break;
                }
                if (reader.hasLocalizedStrings())
                    reader.getLocalizedString(mDesc); // TODO check if formid is null
                else if (!reader.getZString(mDesc))
                    throw std::runtime_error ("RACE DESC data read error");

                break;
            }
            case ESM4::SUB_SPLO: // bonus spell formid (TES5 may have SPCT and multiple SPLO)
            {
                FormId magic;
                reader.get(magic);
                mBonusSpells.push_back(magic);
//              std::cout << "RACE " << printName(subHdr.typeId) << " " << formIdToString(magic) << std::endl;

                break;
            }
            case ESM4::SUB_DATA: // ?? different length for TES5
            {
// DATA:size 128
// 0f 0f ff 00 ff 00 ff 00 ff 00 ff 00 ff 00 00 00
// 9a 99 99 3f 00 00 80 3f 00 00 80 3f 00 00 80 3f
// 48 89 10 00 00 00 40 41 00 00 00 00 00 00 48 43
// 00 00 48 43 00 00 80 3f 9a 99 19 3f 00 00 00 40
// 01 00 00 00 ff ff ff ff ff ff ff ff 00 00 00 00
// ff ff ff ff 00 00 00 00 00 00 20 41 00 00 a0 40
// 00 00 a0 40 00 00 80 42 ff ff ff ff 00 00 00 00
// 00 00 00 00 9a 99 99 3e 00 00 a0 40 02 00 00 00
#if 0
                unsigned char mDataBuf[256/*bufSize*/];
                reader.get(&mDataBuf[0], subHdr.dataSize);

                std::ostringstream ss;
                ss << ESM4::printName(subHdr.typeId) << ":size " << subHdr.dataSize << "\n";
                for (unsigned int i = 0; i < subHdr.dataSize; ++i)
                {
                    //if (mDataBuf[i] > 64 && mDataBuf[i] < 91)
                        //ss << (char)(mDataBuf[i]) << " ";
                    //else
                        ss << std::setfill('0') << std::setw(2) << std::hex << (int)(mDataBuf[i]);
                    if ((i & 0x000f) == 0xf)
                        ss << "\n";
                    else if (i < 256/*bufSize*/-1)
                        ss << " ";
                }
                std::cout << ss.str() << std::endl;
#else
                if (subHdr.dataSize == 36) // TES4
                {
                    std::uint8_t skill;
                    std::uint8_t bonus;
                    for (unsigned int i = 0; i < 8; ++i)
                    {
                        reader.get(skill);
                        reader.get(bonus);
                        mSkillBonus[static_cast<SkillIndex>(skill)] = bonus;
                    }
                    reader.get(mHeightMale);
                    reader.get(mHeightFemale);
                    reader.get(mWeightMale);
                    reader.get(mWeightFemale);
                    reader.get(mRaceFlags);
                }
                else if (subHdr.dataSize >= 128 && subHdr.dataSize <= 164) // TES5
                {
                    std::uint8_t skill;
                    std::uint8_t bonus;
                    for (unsigned int i = 0; i < 7; ++i)
                    {
                        reader.get(skill);
                        reader.get(bonus);
                        mSkillBonus[static_cast<SkillIndex>(skill)] = bonus;
                    }
                    std::uint16_t unknown;
                    reader.get(unknown);

                    reader.get(mHeightMale);
                    reader.get(mHeightFemale);
                    reader.get(mWeightMale);
                    reader.get(mWeightFemale);
                    reader.get(mRaceFlags);

                    // FIXME
                    float dummy;
                    reader.get(dummy); // starting health
                    reader.get(dummy); // starting magicka
                    reader.get(dummy); // starting stamina
                    reader.get(dummy); // base carry weight
                    reader.get(dummy); // base mass
                    reader.get(dummy); // accleration rate
                    reader.get(dummy); // decleration rate

                    uint32_t dummy2;
                    reader.get(dummy2); // size
                    reader.get(dummy2); // head biped object
                    reader.get(dummy2); // hair biped object
                    reader.get(dummy);  // injured health % (0.f..1.f)
                    reader.get(dummy2); // shield biped object
                    reader.get(dummy);  // health regen
                    reader.get(dummy);  // magicka regen
                    reader.get(dummy);  // stamina regen
                    reader.get(dummy);  // unarmed damage
                    reader.get(dummy);  // unarmed reach
                    reader.get(dummy2); // body biped object
                    reader.get(dummy);  // aim angle tolerance
                    reader.get(dummy2); // unknown
                    reader.get(dummy);  // angular accleration rate
                    reader.get(dummy);  // angular tolerance
                    reader.get(dummy2); // flags

                    if (subHdr.dataSize > 128)
                    {
                        reader.get(dummy2); // unknown 1
                        reader.get(dummy2); // unknown 2
                        reader.get(dummy2); // unknown 3
                        reader.get(dummy2); // unknown 4
                        reader.get(dummy2); // unknown 5
                        reader.get(dummy2); // unknown 6
                        reader.get(dummy2); // unknown 7
                        reader.get(dummy2); // unknown 8
                        reader.get(dummy2); // unknown 9
                    }
                }
                else
                {
                    reader.skipSubRecordData();
                    std::cout << "RACE " << ESM4::printName(subHdr.typeId) << " skipping..."
                        << subHdr.dataSize << std::endl;
                }
#endif
                break;
            }
            case ESM4::SUB_DNAM:
            {
                reader.get(mDefaultHair[0]); // male
                reader.get(mDefaultHair[1]); // female

                break;
            }
            case ESM4::SUB_CNAM: // Only in TES4?
            //              CNAM       SNAM                     VNAM
            // Sheogorath   0x0  0000  98 2b  10011000 00101011
            // Golden Saint 0x3  0011  26 46  00100110 01000110
            // Dark Seducer 0xC  1100  df 55  11011111 01010101
            // Vampire Race 0x0  0000  77 44  01110111 10001000
            // Dremora      0x7  0111  bf 32  10111111 00110010
            // Argonian     0x0  0000  dc 3c  11011100 00111100
            // Nord         0x5  0101  b6 03  10110110 00000011
            // Breton       0x5  0101  48 1d  01001000 00011101 00000000 00000907 (Imperial)
            // Wood Elf     0xD  1101  2e 4a  00101110 01001010 00019204 00019204 (HighElf)
            // khajiit      0x5  0101  54 5b  01010100 01011011 00023FE9 00023FE9 (Argonian)
            // Dark Elf     0x0  0000  72 54  01110010 01010100 00019204 00019204 (HighElf)
            // Orc          0xC  1100  74 09  01110100 00001001 000224FD 000224FD (Nord)
            // High Elf     0xF  1111  e6 21  11100110 00100001
            // Redguard     0xD  1101  a9 61  10101001 01100001
            // Imperial     0xD  1101  8e 35  10001110 00110101
            {
                reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_PNAM: reader.get(mFaceGenMainClamp); break; // 0x40A00000 = 5.f
            case ESM4::SUB_UNAM: reader.get(mFaceGenFaceClamp); break; // 0x40400000 = 3.f
            case ESM4::SUB_ATTR: // Only in TES4?
            {
                if (subHdr.dataSize == 2) // FO3?
                {
                    reader.skipSubRecordData();
                    break;
                }
                reader.get(mAttribMale.strength);
                reader.get(mAttribMale.intelligence);
                reader.get(mAttribMale.willpower);
                reader.get(mAttribMale.agility);
                reader.get(mAttribMale.speed);
                reader.get(mAttribMale.endurance);
                reader.get(mAttribMale.personality);
                reader.get(mAttribMale.luck);
                reader.get(mAttribFemale.strength);
                reader.get(mAttribFemale.intelligence);
                reader.get(mAttribFemale.willpower);
                reader.get(mAttribFemale.agility);
                reader.get(mAttribFemale.speed);
                reader.get(mAttribFemale.endurance);
                reader.get(mAttribFemale.personality);
                reader.get(mAttribFemale.luck);

                break;
            }
            //        [0..9]-> ICON
            // NAM0 -> INDX -> MODL --+
            //          ^   -> MODB   |
            //          |             |
            //          +-------------+
            //
            case ESM4::SUB_NAM0: // start marker head data
            {
                headpart = true;
                mHeadParts.resize(9); // assumed based on Construction Set
                currentIndex = 0xffffffff;
                break;
            }
            case ESM4::SUB_INDX:
            {
                reader.get(currentIndex);
                // FIXME: below check is rather useless
                //if (headpart)
                //{
                //    if (currentIndex > 8)
                //        throw std::runtime_error("ESM4::RACE::load - too many head part " + currentIndex);
                //}
                //else // bodypart
                //{
                //    if (currentIndex > 4)
                //        throw std::runtime_error("ESM4::RACE::load - too many body part " + currentIndex);
                //}

                break;
            }
            case ESM4::SUB_MODL:
            {
                if (headpart)
                    reader.getZString(mHeadParts[currentIndex].mesh);
                else if (isMale)
                    reader.getZString(mBodyPartsMale[currentIndex].mesh);
                else
                    reader.getZString(mBodyPartsFemale[currentIndex].mesh);

                break;
            }
            case ESM4::SUB_MODB: reader.skipSubRecordData(); break; // always 0x0000?
            case ESM4::SUB_ICON: // Only in TES4?
            {
                if (headpart)
                    reader.getZString(mHeadParts[currentIndex].texture);
                else if (isMale)
                    reader.getZString(mBodyPartsMale[currentIndex].texture);
                else
                    reader.getZString(mBodyPartsFemale[currentIndex].texture);

                break;
            }
            //
            case ESM4::SUB_NAM1: // start marker body data
            {
                headpart = false; // body part
                mBodyPartsMale.resize(5);   // 0 = upper body, 1 = legs, 2 = hands, 3 = feet, 4 = tail
                mBodyPartsFemale.resize(5); // 0 = upper body, 1 = legs, 2 = hands, 3 = feet, 4 = tail
                currentIndex = 4; // FIXME: argonian tail mesh without preceeding INDX
                break;
            }
            case ESM4::SUB_MNAM: isMale = true; break;
            case ESM4::SUB_FNAM: isMale = false; break;
            //
            case ESM4::SUB_HNAM:
            {
                std::size_t numHairChoices = subHdr.dataSize / sizeof(FormId);
                mHairChoices.resize(numHairChoices);
                for (unsigned int i = 0; i < numHairChoices; ++i)
                    reader.get(mHairChoices.at(i));

                break;
            }
            case ESM4::SUB_ENAM:
            {
                std::size_t numEyeChoices = subHdr.dataSize / sizeof(FormId);
                mEyeChoices.resize(numEyeChoices);
                for (unsigned int i = 0; i < numEyeChoices; ++i)
                    reader.get(mEyeChoices.at(i));

                break;
            }
            case ESM4::SUB_FGGS:
            {
                mSymShapeModeCoefficients.resize(50);
                for (std::size_t i = 0; i < 50; ++i)
                    reader.get(mSymShapeModeCoefficients.at(i));

                break;
            }
            case ESM4::SUB_FGGA:
            {
                mAsymShapeModeCoefficients.resize(30);
                for (std::size_t i = 0; i < 30; ++i)
                    reader.get(mAsymShapeModeCoefficients.at(i));

                break;
            }
            case ESM4::SUB_FGTS:
            {
                mSymTextureModeCoefficients.resize(50);
                for (std::size_t i = 0; i < 50; ++i)
                    reader.get(mSymTextureModeCoefficients.at(i));

                break;
            }
            //
            case ESM4::SUB_SNAM: //skipping...2 // only in TES4?
            {
                reader.skipSubRecordData();
                break;
            }
            case ESM4::SUB_XNAM:
            {
                FormId race;
                std::int32_t adjustment;
                reader.get(race);
                reader.get(adjustment);
                mDisposition[race] = adjustment;

                break;
            }
            case ESM4::SUB_VNAM:
            {
                if (subHdr.dataSize == 8) // TES4
                {
                    reader.get(mVNAM[0]); // For TES4 seems to be 2 race formids
                    reader.get(mVNAM[1]);
                }
                else if (subHdr.dataSize == 4) // TES5
                {
                    // equipment type flags meant to be uint32 ???  GLOB reference? shows up in
                    // SCRO in sript records and CTDA in INFO records
                    uint32_t dummy;
                    reader.get(dummy);
                }
                else
                {
                    reader.skipSubRecordData();
                    std::cout << "RACE " << ESM4::printName(subHdr.typeId) << " skipping..."
                        << subHdr.dataSize << std::endl;
                }

                break;
            }
            //
            case ESM4::SUB_ANAM: // TES5
            {
                if (isMale)
                    reader.getZString(mModelMale);
                else
                    reader.getZString(mModelFemale);
                break;
            }
            case ESM4::SUB_KSIZ: reader.get(mNumKeywords); break;
            case ESM4::SUB_KWDA:
            {
                std::uint32_t formid;
                for (unsigned int i = 0; i < mNumKeywords; ++i)
                    reader.get(formid);
                break;
            }
            //
            case ESM4::SUB_WNAM: // ARMO FormId
            case ESM4::SUB_BODT: // body template
            case ESM4::SUB_MTNM: // movement type
            case ESM4::SUB_ATKD: // attack data
            case ESM4::SUB_ATKE: // attach event
            case ESM4::SUB_GNAM: // body part data
            case ESM4::SUB_NAM3: // start of hkx model
            case ESM4::SUB_NAM4: // material type
            case ESM4::SUB_NAM5: // unarmed impact?
            case ESM4::SUB_LNAM: // close loot sound
            case ESM4::SUB_NAME: // biped object names (x32)
            case ESM4::SUB_QNAM: // equipment slot formid
            case ESM4::SUB_HCLF: // default hair colour
            case ESM4::SUB_UNES: // unarmed equipment slot formid
            case ESM4::SUB_TINC:
            case ESM4::SUB_TIND:
            case ESM4::SUB_TINI:
            case ESM4::SUB_TINL:
            case ESM4::SUB_TINP:
            case ESM4::SUB_TINT:
            case ESM4::SUB_TINV:
            case ESM4::SUB_TIRS:
            case ESM4::SUB_PHWT:
            case ESM4::SUB_AHCF:
            case ESM4::SUB_AHCM:
            case ESM4::SUB_HEAD:
            case ESM4::SUB_MPAI:
            case ESM4::SUB_MPAV:
            case ESM4::SUB_DFTF:
            case ESM4::SUB_DFTM:
            case ESM4::SUB_FLMV:
            case ESM4::SUB_FTSF:
            case ESM4::SUB_FTSM:
            case ESM4::SUB_MTYP:
            case ESM4::SUB_NAM7:
            case ESM4::SUB_NAM8:
            case ESM4::SUB_PHTN:
            case ESM4::SUB_RNAM:
            case ESM4::SUB_RNMV:
            case ESM4::SUB_RPRF:
            case ESM4::SUB_RPRM:
            case ESM4::SUB_SNMV:
            case ESM4::SUB_SPCT:
            case ESM4::SUB_SPED:
            case ESM4::SUB_SWMV:
            case ESM4::SUB_WKMV:
            //
            case ESM4::SUB_YNAM: // FO3
            case ESM4::SUB_NAM2: // FO3
            case ESM4::SUB_VTCK: // FO3
            case ESM4::SUB_MODT: // FO3
            case ESM4::SUB_MODD: // FO3
            case ESM4::SUB_ONAM: // FO3
            {

                //std::cout << "RACE " << ESM4::printName(subHdr.typeId) << " skipping..." << subHdr.dataSize << std::endl;
                reader.skipSubRecordData();
                break;
            }
            default:
                throw std::runtime_error("ESM4::RACE::load - Unknown subrecord " + ESM4::printName(subHdr.typeId));
        }
    }
}

//void ESM4::Race::save(ESM4::Writer& writer) const
//{
//}

//void ESM4::Race::blank()
//{
//}
