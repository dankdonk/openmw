/*
  Copyright (C) 2019, 2020 cc9cii

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

  Below code is based on Ogre::SkeletonManager and Ogre::MeshManager.

*/
#include "nimodelmanager.hpp"

#include <vector>
#include <cassert>

#include <boost/algorithm/string.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreTextureManager.h>
#include <OgreTexture.h>
#include <OgreVector3.h>
#include <OgreSkeleton.h>

#include <extern/esm4/npc_.hpp>
#include <extern/esm4/race.hpp>
#include <extern/fglib/fgsam.hpp>

#include "nimodel.hpp"

template<> NiBtOgre::NiModelManager* Ogre::Singleton<NiBtOgre::NiModelManager>::msSingleton = 0;

namespace NiBtOgre
{
    NiModelManager* NiModelManager::getSingletonPtr(void)
    {
        return msSingleton;
    }

    NiModelManager& NiModelManager::getSingleton(void)
    {
        assert( msSingleton ); return ( *msSingleton );
    }

    NiModelManager::NiModelManager()
    {
        // FIXME: how to choose an appropriate value?
        // OgreMain/src/OgreGpuProgramManager.cpp           mLoadOrder =  50.0f;
        // OgreMain/src/OgreHighLevelGpuProgramManager.cpp  mLoadOrder =  50.0f;
        // OgreMain/src/Deprecated/OgreTextureManager.cpp   mLoadOrder =  75.0f;
        // OgreMain/src/OgreMaterialManager.cpp             mLoadOrder = 100.0f;
        // Components/Overlay/src/OgreFontManager.cpp       mLoadOrder = 200.0f;
        // OgreMain/src/OgreMeshManager2.cpp                mLoadOrder = 300.0f;
        // OgreMain/src/OgreOldSkeletonManager.cpp          mLoadOrder = 300.0f;
        // OgreMain/src/OgreMeshManager.cpp                 mLoadOrder = 350.0f;
        mLoadOrder = 400.0f;
        mResourceType = "NiModel";

        Ogre::ResourceGroupManager::getSingleton()._registerResourceManager(mResourceType, this);
    }

    NiModelManager::~NiModelManager()
    {
        Ogre::ResourceGroupManager::getSingleton()._unregisterResourceManager(mResourceType);
    }

    NiModelPtr NiModelManager::getByName(const Ogre::String& name, const Ogre::String& group)
    {
        return Ogre::static_pointer_cast<NiModel>(getResourceByName(name, group));
    }

    NiModelPtr NiModelManager::create(const Ogre::String& name, const Ogre::String& group,
                                       bool isManual, Ogre::ManualResourceLoader* loader,
                                       const Ogre::NameValuePairList* createParams)
    {
        return Ogre::static_pointer_cast<NiModel>(createResource(name, group, isManual, loader, createParams));
    }

    NiModelPtr NiModelManager::createManual(const Ogre::String& name, const Ogre::String& group,
                                            const Ogre::String& nif, Ogre::ManualResourceLoader* loader)
    {
        // Don't try to get existing, create should fail if already exists
        if( this->getResourceByName( name, group ) )
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_DUPLICATE_ITEM,
                         "NiModel with name '" + name + "' already exists.",
                         "NiModelManager::createManual" );
        }

        Ogre::NameValuePairList params;
        params.insert( std::make_pair(Ogre::String("nif"), nif) );

        return create(name, group, true/*isManual*/, loader, &params);
    }

    Ogre::Resource* NiModelManager::createImpl(const Ogre::String& name,
            Ogre::ResourceHandle handle, const Ogre::String& group, bool isManual,
            Ogre::ManualResourceLoader* loader, const Ogre::NameValuePairList* createParams)
    {
        if (createParams)
        {
            Ogre::String nif("nif");

            std::map<Ogre::String, Ogre::String>::const_iterator it = createParams->find(nif);
            if (it != createParams->end())
            {
                return OGRE_NEW NiModel(this, name, handle, group, isManual, loader, it->second);
            }
        }

        OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                     "Cannot find build parameters for " + name,
                     "NiModelManager::createImpl");
    }
#if 0
    // this method may create a skinned model instead if NiSkinInstance is found
    NiModelPtr NiModelManager::createModel(const Ogre::String& nif, const Ogre::String& group,
            NiModel *skeleton)
    {
        // check if existing - we may be attempting to auto-detect whether the model is skinned
        NiModelPtr pModel = getByName(nif, group);
        if (!pModel)
            pModel = createManual(nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Object;
        bInfo.baseNif = nif;
        bInfo.skel = skeleton;
        if (skeleton)
        {
            bInfo.skelNif = skeleton->getModelName();
            bInfo.skelGroup = skeleton->getOgreGroup();
        }
        else
        {
            bInfo.skelNif.clear();
            bInfo.skelGroup.clear();
        }
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }
#endif
    NiModelPtr NiModelManager::createSkinnedModel(const Ogre::String& nif, const Ogre::String& group,
            NiModel *skeleton)
    {
        std::string skelName = boost::to_lower_copy(skeleton->getModelName());
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(skelName+"_"+nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Skinned;
        bInfo.baseNif = nif;
        bInfo.skel = skeleton; // TODO: throw exeption if no skeleton?
        bInfo.skelNif = skeleton->getModelName();
        bInfo.skelGroup = skeleton->getOgreGroup();
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createSkeletonModel(const Ogre::String& nif, const Ogre::String& group)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Skeleton;
        bInfo.baseNif = nif;
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createMorphedModel(const Ogre::String& nif, const Ogre::String& group,
            const ESM4::Npc *npc, const ESM4::Race *race, const Ogre::String& texture, NiModel *skeleton)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(npc->mEditorId + "_" + nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Morphed;
        bInfo.npc = npc;
        bInfo.race = race;
        bInfo.baseNif = nif;
        bInfo.baseTexture = texture;
        bInfo.skel = skeleton;
        if (skeleton)
        {
            bInfo.skelNif = skeleton->getModelName();
            bInfo.skelGroup = skeleton->getOgreGroup();
        }
        else
        {
            bInfo.skelNif.clear();
            bInfo.skelGroup.clear();
        }
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::createAnimModel(const Ogre::String& nif, const Ogre::String& group,
            NiModel *skeleton)
    {
        // Create manual model which calls back self to load
        NiModelPtr pModel = createManual(nif, group, nif, this);

        // store parameters
        ModelBuildInfo bInfo;
        bInfo.type = MBT_Anim;
        bInfo.baseNif = nif;
        mModelBuildInfoMap[pModel.get()] = bInfo;

        pModel->load(); // load immediately

        return pModel;
    }

    NiModelPtr NiModelManager::getOrLoadByName(const Ogre::String& name, const Ogre::String& group)
    {
        Ogre::ResourcePtr res = getResourceByName(name, group);

        // load() calls ResourceManager::createOrRetrieve() then Resource::load()
        // which results in Resource::loadImpl() being called
        if (!res)
        {
            Ogre::NameValuePairList params;
            params.insert( std::make_pair(Ogre::String("nif"), name) );

            res = load(name, group, false/*isManual*/, nullptr/*ManualResourceLoader**/, &params);
        }

        return Ogre::static_pointer_cast<NiModel>(res);
    }

    void NiModelManager::loadResource(Ogre::Resource* res)
    {
        NiModel* model = static_cast<NiModel*>(res);
        model->createNiObjects(); // read the NIF file and create the objects

        // Find build parameters
        std::map<Ogre::Resource*, ModelBuildInfo>::iterator it = mModelBuildInfoMap.find(res);
        if (it == mModelBuildInfoMap.end())
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Cannot find build parameters for " + res->getName(),
                         "NiModelManager::loadResource");
        }

        ModelBuildInfo& bInfo = it->second;
        switch(bInfo.type)
        {
            case MBT_Object:
#if 0
            {
                bool isSkinned = model->buildData().mIsSkinned; // indicates the presence of NiSkinInstance

                if (!isSkinned || bInfo.skel == nullptr)
                {
                    model->findBoneNodes(true);
                    model->createMesh();
                    model->buildModel();
                }
                else // skinned
                {
                    Ogre::SkeletonPtr skel;
                    try
                    {
                        skel = bInfo.skel->getSkeleton();
                    }
                    catch (...)
                    {
                        NiModelPtr skelModel = getByName(bInfo.skelNif, bInfo.skelGroup);
                        skel = skelModel->getSkeleton();
                    }

                    Ogre::String modelName = bInfo.skelNif+"_"+res->getName();
                    NiModelPtr newModel = createManual(modelName, res->getGroup(), res->getName(), this);

                    ModelBuildInfo newInfo = bInfo;
                    newInfo.type = MBT_Skinned;
                    mModelBuildInfoMap[newModel.get()] = newInfo;

                    mModelBuildInfoMap.erase(it); // not sure if we need to erase this

                    newModel->load();
                }

                break;
            }
#else
                break;
#endif
            case MBT_Skinned:
                loadManualSkinnedModel(model, bInfo);
                break;
            case MBT_Skeleton:
                loadManualSkeletonModel(model, bInfo);
                break;
            case MBT_Morphed:
                loadManualMorphedModel(model, bInfo);
                break;
            case MBT_Anim:
                loadManualAnimModel(model, bInfo);
                break;
            default:
                OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                             "Unknown build parameters for " + res->getName(),
                             "NiModelManager::loadResource");
        }
    }

    void NiModelManager::loadManualSkinnedModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        Ogre::SkeletonPtr skel;

        try
        {
            skel = bInfo.skel->getSkeleton();
        }
        catch (...)
        {
            NiModelPtr skelModel = getByName(bInfo.skelNif, bInfo.skelGroup);
            skel = skelModel->getSkeleton();
        }

        pModel->createMesh(false, skel);
        pModel->buildSkinnedModel(skel);
    }

    void NiModelManager::loadManualSkeletonModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        pModel->findBoneNodes(true/*buildObjectPalette*/);
        pModel->buildSkeleton(true/*load*/);
        pModel->createDummyMesh();
    }

    void NiModelManager::loadManualMorphedModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
        // FIXME: needs a try/catch block here

        const std::vector<float>& sRaceCoeff = bInfo.race->mSymShapeModeCoefficients;
        const std::vector<float>& aRaceCoeff = bInfo.race->mAsymShapeModeCoefficients;
        const std::vector<float>& sRaceTCoeff = bInfo.race->mSymTextureModeCoefficients;
        const std::vector<float>& sCoeff = bInfo.npc->mSymShapeModeCoefficients;
        const std::vector<float>& aCoeff = bInfo.npc->mAsymShapeModeCoefficients;
        const std::vector<float>& sTCoeff = bInfo.npc->mSymTextureModeCoefficients;

        // FIXME: no morphed vertices for upper body
        // if EGM and TRI both found, build morphed vertices
        FgLib::FgSam sam;
        sam.buildMorphedVertices(pModel, bInfo.baseNif, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);

        // special material for headhuman.dds
        if (bInfo.bodyPart == 0/*head*/ && bInfo.baseNif.find("headhuman.nif") != std::string::npos)
        {
            bool isFemale = (bInfo.npc->mBaseConfig.flags & 0x1) != 0;

            std::string detailTexture
                = sam.getHeadHumanDetailTexture(sam.getAge(sRaceCoeff, sCoeff), isFemale);

            size_t pos = detailTexture.find_last_of(".");
            if (pos == std::string::npos)
                return; // FIXME: should throw

            std::string normalTexture = detailTexture.substr(0, pos) + "_n.dds";

            // FIXME: do stuff here including morphed textue

            // FIXME: need to pass to shader FaceGen maps textures\faces\oblivion.esm\<formid>_[01].dds

        }
        else if (bInfo.bodyPart == 10/*hair*/) // special material for hair
        {
            size_t pos = bInfo.baseTexture.find_last_of(".");
            if (pos == std::string::npos)
                return; // FIXME: should throw

            std::string texture = bInfo.baseTexture.substr(0, pos);
            std::string normalTexture = texture + "_n.dds";
            std::string heightMap = texture + "_hh.dds";  // NOTE: apparently only the R channel needed?
            std::string layerMap = texture + "_hl.dds";

            // FIXME: how to extract the anisoMap from normal texture's alpha channel?

            // FIXME: do other stuff here (but no texture morphing since there aren't any EGT for hair)

        }
        else if (bInfo.bodyPart <= 2 || bInfo.bodyPart >= 9) // ears, upper body or other heads
        {
            // e.g. argonian, orc, khajiit
            // FIXME: morphed texture

        }
        else
        {
            // no special materials for eyes, mouth, teeth and tongue, just the base texture

        }


#if 0
        // build the morphed texture and use it as the base texture
        Ogre::TexturePtr fgTexture
            = Ogre::TextureManager::getSingleton().getByName(bInfo.baseTexture, pModel->getGroup());
        if (!fgTexture)
        {
            OGRE_EXCEPT( Ogre::Exception::ERR_ITEM_NOT_FOUND,
                         "Base texture not found for " + pModel->getName(),
                         "NiModelManager::loadManualMorphedModel");
        }

        sam.getMorphedTexture(fgTexture,
                bInfo.baseNif, bInfo.npc->mEditorId, sRaceCoeff, aRaceCoeff, sCoeff, aCoeff);
#endif
        // build the material

        // build the mesh ??? maybe not since circular?  just create?

        // build the rest of the model / misc odds and ends

        //if (pModel->targetBone() != "") // this should also work
        if (!pModel->buildData().mIsSkinned)
        {
            pModel->createMesh(true/*isMorphed*/);
            pModel->buildModel();
        }
        else // skinned, need skeleton
        {
            Ogre::SkeletonPtr skel;

            try
            {
                skel = bInfo.skel->getSkeleton();
            }
            catch (...)
            {
                NiModelPtr skelModel = getByName(bInfo.skelNif, bInfo.skelGroup);
                skel = skelModel->getSkeleton();
            }

            pModel->createMesh(true/*isMorphed*/, skel);
            pModel->buildSkinnedModel(skel);
        }
    }

    void NiModelManager::loadManualAnimModel(NiModel* pModel, const ModelBuildInfo& bInfo)
    {
#if 0
        pModel->buildAnimation(
                Ogre::Entity *skelBase,
                NiModelPtr anim,
                std::multimap<float, std::string>& textKeys,
                std::vector<Ogre::Controller<Ogre::Real> >& controllers,
                NiModel *skeleton,
                NiModel *bow)
#endif
    }
}
