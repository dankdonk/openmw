/*
  Copyright (C) 2015-2018 cc9cii

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
#ifndef NIBTOGRE_NIGEOMETRY_H
#define NIBTOGRE_NIGEOMETRY_H

#include "niavobject.hpp"

// Based on NifTools/NifSkope/doc/index.html
//
// NiObjectNET
//     NiAVObject
//         NiGeometry
//             NiTriBasedGeom <------------- /* typedef NiGeometry */
//                 BSLODTriShape
//                 NiTriShape <------------- /* typedef NiTriBasedGeom */
//                 NiTriStrips <------------ /* typedef NiTriBasedGeom */
namespace NiBtOgre
{
    class NiStream;

    // Seen in NIF version 20.0.0.4, 20.0.0.5
    struct NiGeometry : public NiAVObject
    {
        // FIXME: probably not grouped together, need to check real examples
        struct MaterialExtraData
        {
            StringIndex  materialName;
            std::int32_t materialExtraData;
        };

        NiGeometryDataRef mDataIndex; // subclass of NiGeometryData includes NiTriShapeData
        NiSkinInstanceRef mSkinInstanceIndex;

        std::vector<StringIndex> mMaterialName;
        std::vector<std::int32_t> mMaterialExtraData;
        //std::vector<MaterialExtraData> mMaterials;

        bool mHasShader;
        StringIndex mShaderName;

        bool mDirtyFlag;
        std::vector<NiPropertyRef> mBSProperties;

        NiGeometry(uint32_t index, NiStream& stream, const NiModel& model);

        virtual void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);

        // FIXME: will need additional methods here to return an updated SubMesh to BtOgreInst
        // NiTriStrips builds differently to NiTriShapes only in that the data are different
        void createSubMesh(Ogre::Mesh *mesh);

    private:
        void buildTES3(Ogre::SceneNode *sceneNode, BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };

    typedef NiGeometry NiTriBasedGeom;
    typedef NiTriBasedGeom NiTriShape;
    typedef NiTriBasedGeom NiTriStrips; // Seen in NIF version 20.0.0.4, 20.0.0.5


    // Seen in NIF version 20.2.0.7
    struct BSLODTriShape : public NiGeometry
    {
        std::uint32_t mLevel0Size;
        std::uint32_t mLevel1Size;
        std::uint32_t mLevel2Size;

        BSLODTriShape(uint32_t index, NiStream& stream, const NiModel& model);

        void build(BtOgreInst *inst, NiObject *parentNiNode = nullptr);
    };
}

#endif // NIBTOGRE_NIGEOMETRY_H
