/*
  Copyright (C) 2015-2019 cc9cii

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

  Much of the information on the NIF file structures are based on the NifSkope
  documenation.  See http://niftools.sourceforge.net/wiki/NifSkope for details.

*/
#ifndef NIBTOGRE_NIPSYSCOLLIDER_H
#define NIBTOGRE_NIPSYSCOLLIDER_H

#include <OgreVector3.h>

#include "niobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiPSysCollider
//     NiPSysPlanarCollider
//     NiPSysSphericalCollider
namespace NiBtOgre
{
    class NiStream;
    class Header;

    class NiNode;

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysCollider : public NiObject
    {
    public:
        float mBounce;
        bool mSpawnOnCollide;
        bool mDieOnCollide;
        NiPSysSpawnModifierRef mSpawnModifierRef;
        NiObject   *mParent; // Ptr
        NiObjectRef mNextColliderRef;
        // architecture\pentagon\crane01.nif (FO3) shows that some of the Ptr refer to
        // objects not yet loaded. Change to Ref instead.
        //NiNode     *mColliderObject; // Ptr
        NiAVObjectRef mColliderObjectRef;

        NiPSysCollider(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF ver 20.0.0.4, 20.0.0.5
    class NiPSysPlanarCollider : public NiPSysCollider
    {
    public:
        float mWidth;
        float mHeight;
        Ogre::Vector3 mXAxis;
        Ogre::Vector3 mYAxis;

        NiPSysPlanarCollider(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
    };

    // Seen in NIF version 20.2.0.7
    class NiPSysSphericalCollider : public NiPSysCollider
    {
    public:
        float mRadius;

        NiPSysSphericalCollider(uint32_t index, NiStream& stream, const NiModel& model, BuildData& data);
    };
}

#endif // NIBTOGRE_NIPSYSCOLLIDER_H
