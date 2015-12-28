/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (extra.h) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#ifndef OPENMW_COMPONENTS_NIF_EXTRA_HPP
#define OPENMW_COMPONENTS_NIF_EXTRA_HPP

#include "base.hpp"

namespace Nif
{

class NiVertWeightsExtraData : public Extra
{
public:
    void read(NIFStream *nif)
    {
        Extra::read(nif);

        // We should have s*4+2 == i, for some reason. Might simply be the
        // size of the rest of the record, unhelpful as that may be.
        /*int i =*/ nif->getInt();
        int s = nif->getUShort();

        nif->skip(s * sizeof(float)); // vertex weights I guess
    }
};

class NiTextKeyExtraData : public Extra
{
public:
    struct TextKey
    {
        float time;
        std::string text;
    };
    std::vector<TextKey> list;

    void read(NIFStream *nif)
    {
        Extra::read(nif);

        nif->getInt(); // 0

        int keynum = nif->getInt();
        list.resize(keynum);
        for(int i=0; i<keynum; i++)
        {
            list[i].time = nif->getFloat();
            list[i].text = nif->getString();
        }
    }
};

class NiStringExtraData : public Extra
{
public:
    /* Two known meanings:
       "MRK" - marker, only visible in the editor, not rendered in-game
       "NCO" - no collision
    */
    std::string name;
    std::string string;

    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a000100) // from 10.0.1.0
            name = nif->getString();

        if (nifVer <= 0x04020200) // up to 4.2.2.0
        {
            Extra::read(nif);

            nif->getInt(); // size of string + 4. Really useful...
        }
        string = nif->getString();
    }
};

// http://niftools.sourceforge.net/doc/nif/BSXFlags.html
//
// Controls animation and collision. Integer holds flags:
// Bit 0 : enable havok, bAnimated(Skyrim)
// Bit 1 : enable collision, bHavok(Skyrim)
// Bit 2 : is skeleton nif?, bRagdoll(Skyrim)
// Bit 3 : enable animation, bComplex(Skyrim)
// Bit 4 : FlameNodes present, bAddon(Skyrim)
// Bit 5 : EditorMarkers present
// Bit 6 : bDynamic(Skyrim)
// Bit 7 : bArticulated(Skyrim)
// Bit 8 : bIKTarget(Skyrim)
// Bit 9 : Unknown(Skyrim)
class BSXFlags : public Extra
{
public:
    std::string name;

    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a000100) // Name, from 10.0.1.0
            name = nif->getString();

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            Extra::read(nif);

        nif->getInt(); // unsupported for now, see above for details
    }
};

class NiBinaryExtraData : public Extra
{
public:
    std::string string;
    std::vector<char> data;

    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a000100)
            string = nif->getString(); // Name, from 10.0.1.0

        if (nifVer <= 0x04020200) // up to 4.2.2.0
            Extra::read(nif);

        unsigned int size = nif->getUInt();
        data.resize(size);
        for(unsigned int i = 0; i< size; i++)
        {
            data[i] = nif->getChar();
        }
    }
};

} // Namespace
#endif
