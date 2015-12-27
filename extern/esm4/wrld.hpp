/*
  Copyright (C) 2015 cc9cii

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
#ifndef ESM4_WRLD_H
#define ESM4_WRLD_H

#include <vector>

#include "common.hpp"

namespace ESM4
{
    class Reader;
    class Writer;

    struct World
    {
        enum WorldFlags                 // TES4                 TES5
        {                               // -------------------- -----------------
            WLD_Small          = 0x01,  // Small World          Small World
            WLD_NoFastTravel   = 0x02,  // Can't Fast Travel    Can't Fast Travel
            WLD_Oblivion       = 0x04,  // Oblivion worldspace
            WLD_NoLODWater     = 0x08,  //                      No LOD Water
            WLD_NoLandscpe     = 0x10,  // No LOD Water         No Landscape
            WLD_NoSky          = 0x20,  //                      No Sky
            wLD_FixedDimension = 0x40,  //                      Fixed Dimensions
            WLD_NoGrass        = 0x80   //                      No Grass
        };

        struct REFRcoord
        {
            FormId       formId;
            std::int16_t unknown1;
            std::int16_t unknown2;
       };

        struct RNAMstruct
        {
            std::int16_t unknown1;
            std::int16_t unknown2;
            std::vector<REFRcoord> refrs;
       };

        FormId mFormId;       // from the header
        std::uint32_t mFlags; // from the header, see enum type RecordFlag for details

        std::string mEditorId;
        std::string mFullName;
        FormId mParent;       // parent worldspace formid
        std::uint8_t mWorldFlags;

        // ------ TES4 only -----

        std::int32_t mSound;   // 0 = no record, 1 = Public, 2 = Dungeon
        std::string mMapFile;

        // ------ TES5 only -----

        Grid mCenterCell;
        RNAMstruct mData;

        // ----------------------

        // TODO consider caching children formId's (e.g. CELL, ROAD)

        World();
        ~World();

        void load(ESM4::Reader& reader);
        //void save(ESM4::Writer& writer) const;

        void blank();
    };
}

#endif // ESM4_WRLD_H
