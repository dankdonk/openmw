/*
  Copyright (C) 2017 cc9cii

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
#ifndef NIBTOGRE_NIOBJECT_H
#define NIBTOGRE_NIOBJECT_H

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

#include <boost/current_function.hpp>

#include <components/nifogre/objectscene.hpp> // FIXME

#include "factory.hpp"

namespace Ogre
{
    class SceneNode;
}

namespace NiBtOgre
{
    class NiStream;
    class Header;
    class NiModel;


    // based on https://stackoverflow.com/questions/1832003/instantiating-classes-by-name-with-factory-pattern
    class NiObject
    {
    public:
        virtual ~NiObject() {}

        typedef std::vector<std::unique_ptr<NiObject> > RecordBlocks;
        virtual void build(const RecordBlocks& objects, const Header& header,
                           Ogre::SceneNode* sceneNode, NifOgre::ObjectScenePtr scene) {} //= 0; // FIXME

        typedef NiBtOgre::Factory<NiObject> Factory;
        //static Factory::Type create(Factory::Key const& name, NiStream& stream, const RecordBlocks& objects)
        static Factory::Type create(Factory::Key const& name, NiStream& stream, const NiModel& model)
        {
            //return mFactory.create(name, stream, objects);
            return mFactory.create(name, stream, model);
        }

        template<class Derived>
        static void define(Factory::Key const& name)
        {
            if (!mFactory.define(name, &Factory::template create_func<NiObject, Derived>))
                throw std::logic_error(std::string(BOOST_CURRENT_FUNCTION) + ": name already registered");
        }

        //NiObject(NiStream& stream, const RecordBlocks& objects);
        NiObject(NiStream& stream, const NiModel& model) : mModel(model) {}

    protected:
        NiObject() = default; // disallow the default constructor in derived classes
        const NiModel& mModel; // a little akward, but need a way to access NiObject Ptrs/Refs and TES5 strings

    private:
        static Factory mFactory;
    };
#if 0
    template<typename Type> NiObject* create() { return new Type; }
#endif

    // BSEffectShaderProperty
    // BSLightingShaderProperty
    // BSSkyShaderProperty
    // BSWaterShaderProperty
    // NiGeometryData
    // TexDesc
    struct TexCoord
    {
        float u;
        float v;
    };

    typedef std::int32_t AbstractAdditionalGeometryDataRef;
    typedef std::int32_t BSAnimNotesRef;
    typedef std::int32_t BSMultiBoundDataRef;
    typedef std::int32_t BSMultiBoundRef;
    typedef std::int32_t BSShaderTextureSetRef;
    typedef std::int32_t NiAVObjectRef;
    typedef std::int32_t NiBSplineBasisDataRef;
    typedef std::int32_t NiBSplineDataRef;
    typedef std::int32_t NiBoolDataRef;
    typedef std::int32_t NiCollisionObjectRef;
    typedef std::int32_t NiColorDataRef;
    typedef std::int32_t NiControllerSequenceRef;
    typedef std::int32_t NiDefaultAvObjectPaletteRef;
    typedef std::int32_t NiDynamicEffectRef;
    typedef std::int32_t NiExtraDataRef;
    typedef std::int32_t NiFloatDataRef;
    typedef std::int32_t NiFloatInterpolatorRef;
    typedef std::int32_t NiGeometryDataRef;
    typedef std::int32_t NiInterpolatorRef;
    typedef std::int32_t NiKeyframeDataRef;
    typedef std::int32_t NiMorphDataRef;
    typedef std::int32_t NiNodeRef;
    typedef std::int32_t NiObjectRef;
    typedef std::int32_t NiPSysColliderRef;
    typedef std::int32_t NiPSysModifierRef;
    typedef std::int32_t NiPSysSpawnModifierRef;
    typedef std::int32_t NiPaletteRef;
    typedef std::int32_t NiParticleModifierRef;
    typedef std::int32_t NiPixelDataRef;
    typedef std::int32_t NiPoint3InterpolatorRef;
    typedef std::int32_t NiPosDataRef;
    typedef std::int32_t NiPropertyRef;
    typedef std::int32_t NiSkinDataRef;
    typedef std::int32_t NiSkinInstanceRef;
    typedef std::int32_t NiSkinPartitionRef;
    typedef std::int32_t NiSourceTextureRef;
    typedef std::int32_t NiStringPaletteRef;
    typedef std::int32_t NiTextKeyExtraDataRef;
    typedef std::int32_t NiTimeControllerRef;
    typedef std::int32_t NiTransformDataRef;
    typedef std::int32_t NiTriBasedGeomRef;
    typedef std::int32_t NiTriStripsDataRef;
    typedef std::int32_t NiUVDataRef;
    typedef std::int32_t NiVisDataRef;
    typedef std::int32_t bhkCompressedMeshShapeDataRef;
    typedef std::int32_t bhkSerializableRef;
    typedef std::int32_t bhkShapeRef;
    typedef std::int32_t hkPackedNiTriStripsDataRef;
}

#endif // NIBTOGRE_NIOBJECT_H
