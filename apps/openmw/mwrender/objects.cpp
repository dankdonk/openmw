#include "objects.hpp"

#include <cmath>
//#include <iostream> // FIXME: testing only

#include <OgreSceneNode.h>
#include <OgreSceneManager.h>
#include <OgreEntity.h>
#include <OgreNode.h>
#include <OgreLight.h>
#include <OgreSubEntity.h>
#include <OgreParticleSystem.h>
#include <OgreParticleEmitter.h>
#include <OgreStaticGeometry.h>

#include <extern/esm4/formid.hpp> // mainly for debugging
#include <extern/nibtogre/btogreinst.hpp>
#include <extern/nibtogre/nimodelmanager.hpp>

#include <components/esm/loadligh.hpp>
#include <components/esm/loadstat.hpp>

#include <components/nifogre/ogrenifloader.hpp>
#include <components/settings/settings.hpp>

#include "../mwworld/ptr.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "renderconst.hpp"
#include "animation.hpp"
#include "foreignactivatoranimation.hpp"

using namespace MWRender;

int Objects::uniqueID = 0;

Objects::~Objects()
{
    // FIXME: delete landscape
    for (std::size_t i = 0; i < mLandscapes.size(); ++i)
    {
        delete mLandscapes[i];
    }
}

void Objects::setRootNode(Ogre::SceneNode* root)
{
    mRootNode = root;
}

void Objects::insertBegin(const MWWorld::Ptr& ptr)
{
    Ogre::SceneNode* root = mRootNode;
    Ogre::SceneNode* cellnode;
    if(mCellSceneNodes.find(ptr.getCell()) == mCellSceneNodes.end())
    {
        //Create the scenenode and put it in the map
        cellnode = root->createChildSceneNode();
        mCellSceneNodes[ptr.getCell()] = cellnode;
    }
    else
    {
        cellnode = mCellSceneNodes[ptr.getCell()];
    }

    Ogre::SceneNode* insert = cellnode->createChildSceneNode();
    const float *f = ptr.getRefData().getPosition().pos;

    insert->setPosition(f[0], f[1], f[2]);
    insert->setScale(ptr.getCellRef().getScale(), ptr.getCellRef().getScale(), ptr.getCellRef().getScale());


    // Convert MW rotation to a quaternion:
    f = ptr.getCellRef().getPosition().rot;

    // Rotate around X axis
    Ogre::Quaternion xr(Ogre::Radian(-f[0]), Ogre::Vector3::UNIT_X);

    // Rotate around Y axis
    Ogre::Quaternion yr(Ogre::Radian(-f[1]), Ogre::Vector3::UNIT_Y);

    // Rotate around Z axis
    Ogre::Quaternion zr(Ogre::Radian(-f[2]), Ogre::Vector3::UNIT_Z);

    // Rotates first around z, then y, then x
    insert->setOrientation(xr*yr*zr);

    ptr.getRefData().setBaseNode(insert);
}

void Objects::insertLight(const MWWorld::Ptr &ptr)
{
    insertBegin(ptr);

    std::auto_ptr<ObjectAnimation> anim = std::auto_ptr<ObjectAnimation>(new ObjectAnimation(ptr, ""));

    if(anim.get() != NULL)
    {
        anim->addLight();
        mObjects.insert(std::make_pair(ptr, anim.release()));
    }
}

const std::map<std::int32_t, Ogre::SceneNode*> *Objects::insertModel(const MWWorld::Ptr &ptr, const std::string &mesh, bool batch)
{
    insertBegin(ptr);
    //if (mesh.find("TorchTall01") != std::string::npos)
        //std::cout << "stop" << std::endl;

    std::auto_ptr<ObjectAnimation> anim;
    if(ptr.getTypeName() == typeid(ESM4::Activator).name())
        anim = std::auto_ptr<ObjectAnimation>(new ForeignActivatorAnimation(ptr, mesh));
    else
        anim = std::auto_ptr<ObjectAnimation>(new ObjectAnimation(ptr, mesh));

    if(ptr.getTypeName() == typeid(ESM4::Light).name())
        anim->addLight();

    if (!mesh.empty())
    {
        Ogre::AxisAlignedBox bounds = anim->getWorldBounds();
        Ogre::Vector3 extents = bounds.getSize();
        extents *= ptr.getRefData().getBaseNode()->getScale();
        float size = std::max(std::max(extents.x, extents.y), extents.z);

        bool small = (size < Settings::Manager::getInt("small object size", "Viewing distance")) &&
                     Settings::Manager::getBool("limit small object distance", "Viewing distance");
        // do not fade out doors. that will cause holes and look stupid
        if(ptr.getTypeName().find("Door") != std::string::npos)
            small = false;

        if (mBounds.find(ptr.getCell()) == mBounds.end())
            mBounds[ptr.getCell()] = Ogre::AxisAlignedBox::BOX_NULL;
        mBounds[ptr.getCell()].merge(bounds);

        if(batch &&
           Settings::Manager::getBool("use static geometry", "Objects") &&
           anim->canBatch())
        {
            Ogre::StaticGeometry* sg = 0;

            if (small)
            {
                if(mStaticGeometrySmall.find(ptr.getCell()) == mStaticGeometrySmall.end())
                {
                    uniqueID = uniqueID+1;
                    sg = mRenderer.getScene()->createStaticGeometry("sg" + Ogre::StringConverter::toString(uniqueID));
                    sg->setOrigin(ptr.getRefData().getBaseNode()->getPosition());
                    mStaticGeometrySmall[ptr.getCell()] = sg;

                    sg->setRenderingDistance(static_cast<Ogre::Real>(Settings::Manager::getInt("small object distance", "Viewing distance")));
                }
                else
                    sg = mStaticGeometrySmall[ptr.getCell()];
            }
            else
            {
                if(mStaticGeometry.find(ptr.getCell()) == mStaticGeometry.end())
                {
                    uniqueID = uniqueID+1;
                    sg = mRenderer.getScene()->createStaticGeometry("sg" + Ogre::StringConverter::toString(uniqueID));
                    sg->setOrigin(ptr.getRefData().getBaseNode()->getPosition());
                    mStaticGeometry[ptr.getCell()] = sg;
                }
                else
                    sg = mStaticGeometry[ptr.getCell()];
            }

            // This specifies the size of a single batch region.
            // If it is set too high:
            //  - there will be problems choosing the correct lights
            //  - the culling will be more inefficient
            // If it is set too low:
            //  - there will be too many batches.
            if(ptr.getCell()->isExterior())
                sg->setRegionDimensions(Ogre::Vector3(2048,2048,2048));
            else
                sg->setRegionDimensions(Ogre::Vector3(1024,1024,1024));

            sg->setVisibilityFlags(small ? RV_StaticsSmall : RV_Statics);

            sg->setCastShadows(true);

            sg->setRenderQueueGroup(RQG_Main);

            anim->fillBatch(sg);
            /* TODO: We could hold on to this and just detach it from the scene graph, so if the Ptr
             * ever needs to modify we can reattach it and rebuild the StaticGeometry object without
             * it. Would require associating the Ptr with the StaticGeometry. */
            anim.reset();
        }
    }

    if(anim.get() != NULL)
    {
        const std::map<std::int32_t, Ogre::SceneNode*> *res = &anim->getPhysicsNodeMap();
        mObjects.insert(std::make_pair(ptr, anim.release()));
        return res;
    }
    else
        return nullptr;
}

void Objects::insertLandscapeModel(ESM4::FormId worldId, int x, int y, const std::string &mesh)
{
    // FIXME: need something similar to mObjects to keep track of SceneNodes, etc
    Ogre::SceneNode* root = mRootNode;
    Ogre::SceneNode* cellnode = root->createChildSceneNode(); // child for cell level

    Ogre::SceneNode* insert = cellnode->createChildSceneNode(); // grandchild for object level

    //insert->setPosition();
    //insert->setOrientation();
    //insert->setScale();

    NiBtOgre::NiModelManager& modelManager = NiBtOgre::NiModelManager::getSingleton();
    NiModelPtr landscape = modelManager.getOrLoadByName(mesh, "General");
    //NiModelPtr landscape = modelManager.getByName(mesh, "General");
    //if (!landscape)
        //landscape = modelManager.createLandscapeModel(mesh, "General");

    // Ogre::SceneManager needed to destroy created Ogre::Entity
    //if (!mLandscape)
        mLandscapes.push_back(new NiBtOgre::BtOgreInst(landscape, insert->createChildSceneNode())); // great grandchild for sub-object
    mLandscapes.back()->instantiate();
    std::map<int32_t, Ogre::Entity*>::iterator it(mLandscapes.back()->mEntities.begin());
    for (; it != mLandscapes.back()->mEntities.end(); ++it)
    {
#if 0
        insert->attachObject(it->second);
#else
        uniqueID = uniqueID+1;
        Ogre::StaticGeometry *sg = mRenderer.getScene()->createStaticGeometry("sg" + Ogre::StringConverter::toString(uniqueID));
        sg->setOrigin(Ogre::Vector3::ZERO);
        std::map<std::pair<int, int>, Ogre::StaticGeometry*> lodMap;
        lodMap[std::pair<int,int>(x,y)] = sg;
        mStaticGeometryLandscape[worldId] = lodMap;

        sg->setRegionDimensions(Ogre::Vector3(2048,2048,2048));
        sg->setVisibilityFlags(RV_Statics);
        sg->setCastShadows(true);
        sg->setRenderQueueGroup(RQG_Main);

        Ogre::Node *node = (it->second)->getParentNode();
        //if ((it->second)->isVisible())
            //sg->addEntity(it->second, node->_getDerivedPosition(), node->_getDerivedOrientation(), node->_getDerivedScale());
            sg->addEntity(it->second, Ogre::Vector3::ZERO);
        sg->build();

        //insert->getCreator()->destroyEntity(it->second);
#endif
    }

    // add to Ogre::StaticGeometry here
}

// IDEA: put world in a FIFO and keep the last one (so that if one pops into a store to offload
// some loot and come back out we don't have to reload all the landscape for that world)
void Objects::deleteLandscapeModel(ESM4::FormId worldId, int x, int y, const std::string &mesh)
{
    // FIXME: need something other than mesh name?
}

bool Objects::deleteObject (const MWWorld::Ptr& ptr)
{
    if(!ptr.getRefData().getBaseNode())
        return true;

    PtrAnimationMap::iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
    {
        delete iter->second;
        mObjects.erase(iter);

        mRenderer.getScene()->destroySceneNode(ptr.getRefData().getBaseNode());
        ptr.getRefData().setBaseNode(0);
        return true;
    }

    return false;
}

// FIXME: can't remove landscape LOD each time a cell is removed, but then when do we remove it?
//        maybe when world changes?  how about when someone enters an interior cell then comes
//        back out?
void Objects::removeCell(MWWorld::CellStore* store)
{
    for(PtrAnimationMap::iterator iter = mObjects.begin();iter != mObjects.end();)
    {
        if(iter->first.getCell() == store)
        {
            delete iter->second;
            mObjects.erase(iter++);
        }
        else
            ++iter;
    }

    std::map<MWWorld::CellStore*,Ogre::StaticGeometry*>::iterator geom = mStaticGeometry.find(store);
    if(geom != mStaticGeometry.end())
    {
        Ogre::StaticGeometry *sg = geom->second;
        mStaticGeometry.erase(geom);
        mRenderer.getScene()->destroyStaticGeometry(sg);
    }

    geom = mStaticGeometrySmall.find(store);
    if(geom != mStaticGeometrySmall.end())
    {
        Ogre::StaticGeometry *sg = geom->second;
        mStaticGeometrySmall.erase(store);
        mRenderer.getScene()->destroyStaticGeometry(sg);
    }

    mBounds.erase(store);

    std::map<MWWorld::CellStore*,Ogre::SceneNode*>::iterator cell = mCellSceneNodes.find(store);
    if(cell != mCellSceneNodes.end())
    {
        cell->second->removeAndDestroyAllChildren();
        mRenderer.getScene()->destroySceneNode(cell->second);
        mCellSceneNodes.erase(cell);
    }
}

void Objects::buildStaticGeometry(MWWorld::CellStore& cell)
{
    if(mStaticGeometry.find(&cell) != mStaticGeometry.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometry[&cell];
        sg->build();
    }
    if(mStaticGeometrySmall.find(&cell) != mStaticGeometrySmall.end())
    {
        Ogre::StaticGeometry* sg = mStaticGeometrySmall[&cell];
        sg->build();
    }
}

Ogre::AxisAlignedBox Objects::getDimensions(MWWorld::CellStore* cell)
{
    return mBounds[cell];
}

void Objects::update(float dt, Ogre::Camera* camera)
{
    PtrAnimationMap::const_iterator it = mObjects.begin();
    for(;it != mObjects.end();++it)
        it->second->runAnimation(dt);

    it = mObjects.begin();
    for(;it != mObjects.end();++it)
        it->second->preRender(camera);

}

void Objects::rebuildStaticGeometry()
{
    for (std::map<MWWorld::CellStore *, Ogre::StaticGeometry*>::iterator it = mStaticGeometry.begin(); it != mStaticGeometry.end(); ++it)
    {
        it->second->destroy();
        it->second->build();
    }

    for (std::map<MWWorld::CellStore *, Ogre::StaticGeometry*>::iterator it = mStaticGeometrySmall.begin(); it != mStaticGeometrySmall.end(); ++it)
    {
        it->second->destroy();
        it->second->build();
    }
}

void Objects::updateObjectCell(const MWWorld::Ptr &old, const MWWorld::Ptr &cur)
{
    Ogre::SceneNode *node;
    MWWorld::CellStore *newCell = cur.getCell();

    if(mCellSceneNodes.find(newCell) == mCellSceneNodes.end()) {
        node = mRootNode->createChildSceneNode();
        mCellSceneNodes[newCell] = node;
    } else {
        node = mCellSceneNodes[newCell];
    }

    node->addChild(cur.getRefData().getBaseNode());

    PtrAnimationMap::iterator iter = mObjects.find(old);
    if(iter != mObjects.end())
    {
        ObjectAnimation *anim = iter->second;
        mObjects.erase(iter);
        anim->updatePtr(cur);
        mObjects[cur] = anim;
    }
}

ObjectAnimation* Objects::getAnimation(const MWWorld::Ptr &ptr)
{
    PtrAnimationMap::const_iterator iter = mObjects.find(ptr);
    if(iter != mObjects.end())
        return iter->second;
    return NULL;
}

