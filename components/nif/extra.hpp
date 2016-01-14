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
    std::string objectName;
    NodePtr next;
    std::vector<TextKey> list;

    void read(NIFStream *nif)
    {
        if (nifVer >= 0x0a000100) // from 10.0.1.0
            objectName = nif->getString();

        if (nifVer <= 0x04020200) // up to 4.2.2.0
        {
            next.read(nif);
            nif->getInt(); // 0
        }

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

        // unsupported for now
        nif->getInt(); // http://niftools.sourceforge.net/doc/nif/BSXFlags.html
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
