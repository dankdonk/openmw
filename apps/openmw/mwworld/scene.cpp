#include "scene.hpp"

#include <stdexcept>
#include <bitset> // FIXME: for testing

#include <OgreSceneNode.h>

#include <extern/esm4/land.hpp>
#include <extern/esm4/cell.hpp>
#include <extern/esm4/wrld.hpp>
#include <extern/esm4/musc.hpp>
#include <extern/esm4/aloc.hpp>
#include <extern/esm4/mset.hpp>

#include <components/nif/niffile.hpp>
#include <components/misc/stringops.hpp>
#include <components/misc/resourcehelpers.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/soundmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "physicssystem.hpp"
#include "player.hpp"
#include "localscripts.hpp"
#include "esmstore.hpp"
#include "class.hpp"
#include "cellfunctors.hpp"
#include "cellstore.hpp"
//#include "foreigncell.hpp"

namespace
{

    void addObject(const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics,
                   MWRender::RenderingManager& rendering)
    {
        std::string model = Misc::ResourceHelpers::correctActorModelPath(ptr.getClass().getModel(ptr));
        std::string id = ptr.getClass().getId(ptr);
        if (id == "prisonmarker" || id == "divinemarker" || id == "templemarker" || id == "northmarker")
            model = ""; // marker objects that have a hardcoded function in the game logic, should be hidden from the player
        rendering.addObject(ptr, model);
        ptr.getClass().insertObject (ptr, model, physics);
    }

    void updateObjectLocalRotation (const MWWorld::Ptr& ptr, MWWorld::PhysicsSystem& physics,
                                    MWRender::RenderingManager& rendering)
    {
        if (ptr.getRefData().getBaseNode() != NULL)
        {
            Ogre::Quaternion worldRotQuat(Ogre::Radian(ptr.getRefData().getPosition().rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z);
            if (!ptr.getClass().isActor())
                worldRotQuat = Ogre::Quaternion(Ogre::Radian(ptr.getRefData().getPosition().rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X)*
                        Ogre::Quaternion(Ogre::Radian(ptr.getRefData().getPosition().rot[1]), Ogre::Vector3::NEGATIVE_UNIT_Y)* worldRotQuat;

            float x = ptr.getRefData().getLocalRotation().rot[0];
            float y = ptr.getRefData().getLocalRotation().rot[1];
            float z = ptr.getRefData().getLocalRotation().rot[2];

            Ogre::Quaternion rot(Ogre::Radian(z), Ogre::Vector3::NEGATIVE_UNIT_Z);
            if (!ptr.getClass().isActor())
                rot = Ogre::Quaternion(Ogre::Radian(x), Ogre::Vector3::NEGATIVE_UNIT_X)*
                Ogre::Quaternion(Ogre::Radian(y), Ogre::Vector3::NEGATIVE_UNIT_Y)*rot;

            ptr.getRefData().getBaseNode()->setOrientation(worldRotQuat*rot);
            physics.rotateObject(ptr);
        }
    }

    struct InsertFunctor
    {
        MWWorld::CellStore& mCell;
        bool mRescale;
        Loading::Listener& mLoadingListener;
        MWWorld::PhysicsSystem& mPhysics;
        MWRender::RenderingManager& mRendering;

        InsertFunctor (MWWorld::CellStore& cell, bool rescale, Loading::Listener& loadingListener,
            MWWorld::PhysicsSystem& physics, MWRender::RenderingManager& rendering);

        bool operator() (const MWWorld::Ptr& ptr);
    };

    InsertFunctor::InsertFunctor (MWWorld::CellStore& cell, bool rescale,
        Loading::Listener& loadingListener, MWWorld::PhysicsSystem& physics,
        MWRender::RenderingManager& rendering)
    : mCell (cell), mRescale (rescale), mLoadingListener (loadingListener),
      mPhysics (physics), mRendering (rendering)
    {}

    bool InsertFunctor::operator() (const MWWorld::Ptr& ptr)
    {
        if (mRescale)
        {
            if (ptr.getCellRef().getScale()<0.5)
                ptr.getCellRef().setScale(0.5);
            else if (ptr.getCellRef().getScale()>2)
                ptr.getCellRef().setScale(2);
        }

        if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
        {
            try
            {
                addObject(ptr, mPhysics, mRendering);
                updateObjectLocalRotation(ptr, mPhysics, mRendering);
                if (ptr.getRefData().getBaseNode())
                {
                    float scale = ptr.getCellRef().getScale();
                    ptr.getClass().adjustScale(ptr, scale);
                    mRendering.scaleObject(ptr, Ogre::Vector3(scale));
                }
                ptr.getClass().adjustPosition (ptr, false);
            }
            catch (const std::exception& e)
            {
                std::string error ("error during rendering: ");
                std::cerr << error + e.what() << std::endl;
            }
        }

        mLoadingListener.increaseProgress (1);

        return true;
    }

    // FIXME: modify InsertFunctor rather than duplicate much of the code? the only change is
    // lack of mPhysics
    struct InsertDummyFunctor
    {
        MWWorld::CellStore& mCell;
        bool mRescale;
        Loading::Listener& mLoadingListener;
        MWWorld::PhysicsSystem *mPhysics;
        MWRender::RenderingManager& mRendering;

        InsertDummyFunctor (MWWorld::CellStore& cell, bool rescale, Loading::Listener& loadingListener,
            MWWorld::PhysicsSystem *physics, MWRender::RenderingManager& rendering);

        bool operator() (const MWWorld::Ptr& ptr);
    };

    InsertDummyFunctor::InsertDummyFunctor (MWWorld::CellStore& cell, bool rescale,
        Loading::Listener& loadingListener, MWWorld::PhysicsSystem *physics,
        MWRender::RenderingManager& rendering)
    : mCell (cell), mRescale (rescale), mLoadingListener (loadingListener),
      mPhysics (physics), mRendering (rendering)
    {}

    bool InsertDummyFunctor::operator() (const MWWorld::Ptr& ptr)
    {
        if (mRescale)
        {
            if (ptr.getCellRef().getScale()<0.5)
                ptr.getCellRef().setScale(0.5);
            else if (ptr.getCellRef().getScale()>2)
                ptr.getCellRef().setScale(2);
        }

        if (!ptr.getRefData().isDeleted() && ptr.getRefData().isEnabled())
        {
            try
            {
                 //if (ptr.getTypeName() == typeid(ESM4::Static).name()) // FIXME
                     //std::cout << "static" << std::endl;

                //addObject(ptr, *mPhysics, mRendering);
                std::string model = ptr.getClass().getModel(ptr);
                mRendering.addObject(ptr, model);
                if (mPhysics)
                    ptr.getClass().insertObject (ptr, model, *mPhysics);

                //updateObjectLocalRotation(ptr, mPhysics, mRendering);
                if (ptr.getRefData().getBaseNode() != NULL)
                {
                    Ogre::Quaternion worldRotQuat(Ogre::Radian(ptr.getRefData().getPosition().rot[2]), Ogre::Vector3::NEGATIVE_UNIT_Z);
                    if (!ptr.getClass().isActor())
                        worldRotQuat = Ogre::Quaternion(Ogre::Radian(ptr.getRefData().getPosition().rot[0]), Ogre::Vector3::NEGATIVE_UNIT_X)*
                                Ogre::Quaternion(Ogre::Radian(ptr.getRefData().getPosition().rot[1]), Ogre::Vector3::NEGATIVE_UNIT_Y)* worldRotQuat;

                    float x = ptr.getRefData().getLocalRotation().rot[0];
                    float y = ptr.getRefData().getLocalRotation().rot[1];
                    float z = ptr.getRefData().getLocalRotation().rot[2];

                    Ogre::Quaternion rot(Ogre::Radian(z), Ogre::Vector3::NEGATIVE_UNIT_Z);
                    if (!ptr.getClass().isActor())
                        rot = Ogre::Quaternion(Ogre::Radian(x), Ogre::Vector3::NEGATIVE_UNIT_X)*
                        Ogre::Quaternion(Ogre::Radian(y), Ogre::Vector3::NEGATIVE_UNIT_Y)*rot;

                    ptr.getRefData().getBaseNode()->setOrientation(worldRotQuat*rot);
                    if (mPhysics)
                        mPhysics->rotateObject(ptr);
                }

                //
                if (ptr.getRefData().getBaseNode())
                {
                    float scale = ptr.getCellRef().getScale();
                    ptr.getClass().adjustScale(ptr, scale);
                    mRendering.scaleObject(ptr, Ogre::Vector3(scale));
                }
                ptr.getClass().adjustPosition(ptr, false);
            }
            catch (const std::exception& e)
            {
                std::string error ("error during rendering: ");
                std::cerr << error + e.what() << std::endl;
            }
        }

        mLoadingListener.increaseProgress (1);

        return true;
    }
}


namespace MWWorld
{

    void Scene::updateObjectLocalRotation (const Ptr& ptr)
    {
        ::updateObjectLocalRotation(ptr, *mPhysics, mRendering);
    }

    void Scene::moveSubObjectLocalPosition (const MWWorld::Ptr& ptr,
                    const std::string& boneName, const Ogre::Vector3& position)
    {
        mPhysics->moveSubObject(ptr, boneName, position);
    }

    void Scene::rotateSubObjectLocalRotation (const MWWorld::Ptr& ptr,
                    const std::string& boneName, const Ogre::Quaternion& rotation)
    {
        mPhysics->rotateSubObject(ptr, boneName, rotation);
    }

    void Scene::updateObjectRotation (const Ptr& ptr)
    {
        if(ptr.getRefData().getBaseNode() != 0)
        {
            mRendering.rotateObject(ptr);
            mPhysics->rotateObject(ptr);
        }
    }

    void Scene::getGridCenter(int &cellX, int &cellY)
    {
        int maxX = std::numeric_limits<int>::min();
        int maxY = std::numeric_limits<int>::min();
        int minX = std::numeric_limits<int>::max();
        int minY = std::numeric_limits<int>::max();
        CellStoreCollection::iterator iter = mActiveCells.begin();
        while (iter!=mActiveCells.end())
        {
            assert ((*iter)->getCell()->isExterior());
            int x = (*iter)->getCell()->getGridX();
            int y = (*iter)->getCell()->getGridY();
            maxX = std::max(x, maxX);
            maxY = std::max(y, maxY);
            minX = std::min(x, minX);
            minY = std::min(y, minY);
            ++iter;
        }
        cellX = (minX + maxX) / 2;
        cellY = (minY + maxY) / 2;
    }

    void Scene::update (float duration, bool paused)
    {
        if (mNeedMapUpdate)
        {
            // Note: exterior cell maps must be updated, even if they were visited before, because the set of surrounding cells might be different
            // (and objects in a different cell can "bleed" into another cells map if they cross the border)
            for (CellStoreCollection::iterator active = mActiveCells.begin(); active!=mActiveCells.end(); ++active)
                mRendering.requestMap(*active);
            mNeedMapUpdate = false;

            if (mCurrentCell->isExterior())
            {
                int cellX, cellY;
                getGridCenter(cellX, cellY);
                MWBase::Environment::get().getWindowManager()->setActiveMap(cellX,cellY,false);
            }
        }

        mRendering.update (duration, paused);
    }

    void Scene::unloadCell (CellStoreCollection::iterator iter)
    {
        std::cout << "Unloading cell\n";
        ListAndResetHandles functor;

        (*iter)->forEach<ListAndResetHandles>(functor);
        {
            // silence annoying g++ warning
            for (std::vector<Ogre::SceneNode*>::const_iterator iter2 (functor.mHandles.begin());
                iter2!=functor.mHandles.end(); ++iter2)
            {
                Ogre::SceneNode* node = *iter2;

                // ragdoll objects have physics objects attached to child SceneNodes
                Ogre::Node::ChildNodeIterator childIter = node->getChildIterator();
                while (childIter.hasMoreElements())
                {
                    mPhysics->removeObject (childIter.current()->first);
                    childIter.getNext();
                }

                mPhysics->removeObject (node->getName());
            }
        }

        if ((*iter)->getCell()->isExterior())
        {
            ESM::Land* land =
                MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                    (*iter)->getCell()->getGridX(),
                    (*iter)->getCell()->getGridY()
                );
            if (land && land->mDataTypes&ESM::Land::DATA_VHGT)
                mPhysics->removeHeightField ((*iter)->getCell()->getGridX(), (*iter)->getCell()->getGridY());
        }

        mRendering.removeCell(*iter);

        MWBase::Environment::get().getWorld()->getLocalScripts().clearCell (*iter);

        MWBase::Environment::get().getMechanicsManager()->drop (*iter);

        MWBase::Environment::get().getSoundManager()->stopSound (*iter);
        mActiveCells.erase(*iter);
    }

    void Scene::loadCell (CellStore *cell, Loading::Listener* loadingListener)
    {
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

        if(result.second)
        {
            std::cout << "loading cell " << cell->getCell()->getDescription() << std::endl;

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;

            // Load terrain physics first...
            if (cell->getCell()->isExterior())
            {
                ESM::Land* land =
                    MWBase::Environment::get().getWorld()->getStore().get<ESM::Land>().search(
                        cell->getCell()->getGridX(),
                        cell->getCell()->getGridY()
                    );
                if (land && land->mDataTypes&ESM::Land::DATA_VHGT) {
                    // Actually only VHGT is needed here, but we'll need the rest for rendering anyway.
                    // Load everything now to reduce IO overhead.
                    const int flags = ESM::Land::DATA_VCLR|ESM::Land::DATA_VHGT|ESM::Land::DATA_VNML|ESM::Land::DATA_VTEX;

                    const ESM::Land::LandData *data = land->getLandData (flags);
                    mPhysics->addHeightField (data->mHeights, cell->getCell()->getGridX(), cell->getCell()->getGridY(),
                        0, worldsize / (verts-1), verts);
                }
            }

            cell->respawn();

            // ... then references. This is important for adjustPosition to work correctly.
            /// \todo rescale depending on the state of a new GMST
            insertCell (*cell, true, loadingListener);

            mRendering.cellAdded (cell);
            bool waterEnabled = cell->getCell()->hasWater() || cell->isExterior();
            mRendering.setWaterEnabled(waterEnabled);
            float waterLevel = cell->isExterior() ? -1.f : cell->getWaterLevel();
            if (waterEnabled)
            {
                mPhysics->enableWater(waterLevel);
                mRendering.setWaterHeight(waterLevel);
            }
            else
                mPhysics->disableWater();

            mRendering.configureAmbient(*cell);
#if 0
            // FIXME: update open (status = 1) door anims
            const MWWorld::Store<ESM4::Door>& doorStore
                = MWBase::Environment::get().getWorld()->getStore().get<ESM4::Door>();
            for (MWWorld::Store<ESM4::Door>::iterator it = doorStore.begin(); it != doorStore.end(); ++it)
            {
                const ESM4::Door& door = *it;
            }
#endif
        }

        // register local scripts
        // ??? Should this go into the above if block ???
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);
    }

    void Scene::loadForeignCell (CellStore *cell, Loading::Listener* loadingListener, uint32_t worldId)
    {
        std::cout << "loading cell " << cell->getCell()->getDescription() << std::endl;

        float verts = ESM4::Land::VERTS_PER_SIDE; // number of vertices per side
        float worldsize = ESM4::Land::REAL_SIZE;  // cell terrain size in world coords

        // FIXME: these shouldn't be Refs and needs to move near LOD landscape below
        if (cell->isDummyCell() || cell->isVisibleDistCell())
        {
            insertForeignCell (*cell, true, loadingListener);
            return;
        }

        // FIXME: some detailed terrain without physics for LOD should be loaded

        // Load terrain physics first...
        if (cell->getCell()->isExterior())
        {
            const ForeignLand *land =
                    MWBase::Environment::get().getWorld()->getStore().getForeign<ForeignLand>().find(cell->getForeignLandId());
            //if (land)
                //std::cout << "heightoffset " << land->mHeightMap.heightOffset << std::endl;

            // check that this cell has land data at all
            if (land && land->getLandData (ESM4::Land::LAND_VHGT))
            {
                float verts = ESM4::Land::VERTS_PER_SIDE;
                float worldsize = ESM4::Land::REAL_SIZE;

                const ESM4Terrain::LandData *data = land->getLandData (ESM4::Land::LAND_VHGT);
                mPhysics->addHeightField(data->mHeights,
                        cell->getCell()->getGridX(), cell->getCell()->getGridY(), 0, worldsize / (verts-1), verts);
            }
            else
                std::cerr << "Heightmap for " << cell->getCell()->getDescription() << " not found" << std::endl;
        }

        cell->respawn(); // FIXME: needs respawnTes4()

        // ... then references. This is important for adjustPosition to work correctly.
        /// \todo rescale depending on the state of a new GMST
        insertForeignCell (*cell, true, loadingListener);

        // NOTE: this also calls Debugging to add active cells for Pathgrids
        mRendering.cellAdded (cell); // calls mTerrain->loadCell()

        bool waterEnabled = cell->getCell()->hasWater() || cell->isExterior();
        mRendering.setWaterEnabled(waterEnabled);
        float waterLevel = /*cell->isExterior() ? -1.f : */cell->getWaterLevel();
        if (waterEnabled)
        {
            mPhysics->enableWater(waterLevel);
            mRendering.setWaterHeight(waterLevel);
        }
        else
            mPhysics->disableWater();

        mRendering.configureAmbient(*cell); // FIXME

        // register local scripts
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell); // FIXME
    }

    void Scene::loadDummyCell (CellStore *cell, int x, int y, Loading::Listener* loadingListener)
    {
#if 0
        // FIXME: do we need x, y ?
        std::size_t range = 2;
        InsertDummyFunctor functor (*cell, true/*rescale*/, *loadingListener, mPhysics, mRendering);
        cell->forEach (functor, x, y, range, 0);

        range = 6;
        InsertDummyFunctor functor2 (*cell, true/*rescale*/, *loadingListener, 0, mRendering);
        cell->forEach (functor2, x, y, range, 2);

        //mDummyStore = cell;
#endif
    }

    void Scene::loadVisibleDist (CellStore *cell, Loading::Listener* loadingListener)
    {
        //std::cout << "loading visible distant " << cell->getCell()->getDescription() << std::endl;

        insertForeignCell (*cell, true, loadingListener);
    }

    void Scene::changeToVoid()
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
            unloadCell (active++);
        assert(mActiveCells.empty());
        mCurrentCell = NULL;
    }

    void Scene::playerMoved(const Ogre::Vector3 &pos)
    {
        if (!mCurrentCell || !mCurrentCell->isExterior())
            return;

        // figure out the center of the current cell grid (*not* necessarily mCurrentCell, which is the cell the player is in)
        int cellX, cellY;
        getGridCenter(cellX, cellY);
        float centerX, centerY;
        if (!mCurrentCell->isForeignCell())
        {
            MWBase::Environment::get().getWorld()->indexToPosition(cellX, cellY, centerX, centerY, true);
            const float maxDistance = 8192/2 + 1024; // 1/2 cell size + threshold
            float distance = std::max(std::abs(centerX-pos.x), std::abs(centerY-pos.y));
            if (distance > maxDistance)
            {
                int newX, newY;
                MWBase::Environment::get().getWorld()->positionToIndex(pos.x, pos.y, newX, newY);
                changeCellGrid(newX, newY);
                mRendering.updateTerrain();
            }
        }
        else // ForeignCell
        {
            // FIXME: indexToPosition needs to be worldspace-aware
            MWBase::Environment::get().getWorld()->indexToWorldPosition("", cellX, cellY, centerX, centerY, true);
            const float maxDistance = (4096/2 + 512)/2; // 1/2 cell size + threshold
            float distance = std::max(std::abs(centerX-pos.x), std::abs(centerY-pos.y));
            if (distance > maxDistance)
            {
                int newX, newY;
                //MWBase::Environment::get().getWorld()->positionToIndex(pos.x, pos.y, newX, newY);
                {
                    const int cellSize = 4096;

                    newX = static_cast<int>(std::floor(pos.x / cellSize));
                    newY = static_cast<int>(std::floor(pos.y / cellSize));
                }

                // If we're still in the same cell, don't change even if gone past max distance?
                //if (mCurrentCell->getCell()->getGridX() == newX && mCurrentCell->getCell()->getGridY() == newY)
                    //return;

                ESM4::FormId worldId
                    = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent;

                updateWorldCellsAtGrid(worldId, newX, newY);
                mRendering.updateTerrain();
            }
        }
    }

    void Scene::changeCellGrid (int X, int Y)
    {
// FIXME: testing only
//#if 0
        if(mCurrentCell && mCurrentCell->isForeignCell())
            throw std::runtime_error("Scene::changeCellGrid used for TES4");
            //std::cout << "Error: changeCellGrid called from a foreign cell" << std::endl;
//#endif

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        mRendering.enableTerrain(true);

        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);

        const int halfGridSize = Settings::Manager::getInt("exterior grid size", "Cells")/2;

        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            if ((*active)->getCell()->isExterior()) // FIXME: should this be an assert instead?
            {
                if (std::abs (X-(*active)->getCell()->getGridX())<=halfGridSize &&
                    std::abs (Y-(*active)->getCell()->getGridY())<=halfGridSize)
                {
                    // keep cells within the new grid
                    ++active;
                    continue;
                }
            }
            unloadCell (active++); // discard cells thare are no longer in the grid (or internal)
        }

        int refsToLoad = 0;
        // get the number of refs to load (for loading bar progress display)
        for (int x=X-halfGridSize; x<=X+halfGridSize; ++x)
        {
            for (int y=Y-halfGridSize; y<=Y+halfGridSize; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x==(*iter)->getCell()->getGridX() &&
                        y==(*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end())
                    refsToLoad += MWBase::Environment::get().getWorld()->getExterior(x, y)->count();
            }
        }

        loadingListener->setProgressRange(refsToLoad);

        // Load cells
        for (int x=X-halfGridSize; x<=X+halfGridSize; ++x)
        {
            for (int y=Y-halfGridSize; y<=Y+halfGridSize; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                // loop through all active cells until x,y matches
                while (iter!=mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x==(*iter)->getCell()->getGridX() &&
                        y==(*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter==mActiveCells.end()) // only load cells that are not already active
                {
                    CellStore *cell = MWBase::Environment::get().getWorld()->getExterior(x, y);

                    loadCell (cell, loadingListener);
                }
            }
        }

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(X,Y);
        MWBase::Environment::get().getWindowManager()->changeCell(current);

        mCellChanged = true;

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;
    }

    void Scene::changePlayerCell(CellStore *cell, const ESM::Position &pos, bool adjustPlayerPos)
    {
        if (mCurrentCell && mCurrentCell == cell) // mCurrentCell can be nullptr (at the start only?)
            return;

        // NOTE: we can detect the change of world space here

        mCurrentCell = cell; // if cell->isForeignCell(), mCell->mParent is the world FormId

        MWBase::World *world = MWBase::Environment::get().getWorld();
        MWWorld::Ptr old = world->getPlayerPtr();
        world->getPlayer().setCell(cell);

        MWWorld::Ptr player = world->getPlayerPtr();
        mRendering.updatePlayerPtr(player);

        if (adjustPlayerPos) {
            world->moveObject(player, pos.pos[0], pos.pos[1], pos.pos[2]);

            float x = Ogre::Radian(pos.rot[0]).valueDegrees();
            float y = Ogre::Radian(pos.rot[1]).valueDegrees();
            float z = Ogre::Radian(pos.rot[2]).valueDegrees();
            world->rotateObject(player, x, y, z);

            player.getClass().adjustPosition(player, true);
        }

        MWBase::MechanicsManager *mechMgr =
            MWBase::Environment::get().getMechanicsManager();

        mechMgr->updateCell(old, player);
        mechMgr->watchActor(player);

        MWBase::Environment::get().getWorld()->adjustSky();
    }

    // loads cells and associated references to mActiveCells as required, based on exterior
    // grid size and player position (cellstore contains cell pointer and refs)
    CellStore *Scene::updateWorldCellsAtGrid (ESM4::FormId worldId, int X, int Y)
    {
        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        // It seems that ICMarketDistrict needs its parent world's LAND
        //const ForeignCell *cell
                //ESM4::FormId parentWorldId
                    //= static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent;
        // TODO: check if the LOD mesh can do the job instead

        ESM4::FormId currentWorldId = 0;
        if (mCurrentCell->isForeignCell())
        {
            if (static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent != 0)
                currentWorldId = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent;
            else
                currentWorldId = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mFormId;
        }
        else
            MWBase::Environment::get().getSoundManager()->initForeign();

        CellStore* cell = MWBase::Environment::get().getWorld()->getWorldCell(worldId, X, Y);
        if (!cell) // FIXME
            return nullptr;

        mRendering.enableTerrain(true, worldId);

        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);

        bool worldChanged = false;

        // TODO: For TES4/5 we should double this value? (cells are smaller)
        const int halfGridSize = Settings::Manager::getInt("exterior grid size", "Cells") / 2;
        if (worldId != currentWorldId)
        {
            worldChanged = true;

            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            bool isFONV = false;

            // FIXME: need to put this in a function or two, maybe in MWClass::ForeignActivator, etc

            // FONV: check if there are any radio activators in this cell
            std::map</*ESM4::Quest**/ESM4::FormId, ESM4::FormId> radioQuests;
            typedef CellRefList<ESM4::Activator>::List ActivatorList;
            {
            const ActivatorList& acti = cell->getForeignReadOnly<ESM4::Activator>().mList;
            for (ActivatorList::const_iterator it = acti.begin(); it != acti.end(); ++it)
            {
                const MWWorld::LiveCellRef<ESM4::Activator>& ref = *it;
                const CellRef& actRef = ref.mRef;
                const ESM4::Activator *acti = ref.mBase;

                bool hasRadio = acti->mRadioStation != 0;

                //std::cout << acti->mEditorId << std::endl;

                if (hasRadio)
                {
                    // look for quests that are "start game enabled" and has a condition
                    // looking for this activator
                    const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().searchCondition(acti->mRadioStation);
                    if (quest)
                    {
                        ESM4::FormId questId = quest->mFormId;
                        radioQuests[questId] = acti->mRadioStation;
                        std::cout << "world has acti radio quest " << quest->mEditorId << std::endl;
                    }
                }
            }
            }
            CellStore *dummy = MWBase::Environment::get().getWorld()->getWorldDummyCell(worldId);
            {
            const ActivatorList& acti = dummy->getForeignReadOnly<ESM4::Activator>().mList;
            for (ActivatorList::const_iterator it = acti.begin(); it != acti.end(); ++it)
            {
                const MWWorld::LiveCellRef<ESM4::Activator>& ref = *it;
                const CellRef& actRef = ref.mRef;
                const ESM4::Activator *acti = ref.mBase;

                bool hasRadio = acti->mRadioStation != 0;

                //std::cout << acti->mEditorId << std::endl;

                if (hasRadio)
                {
                    // look for quests that are "start game enabled" and has a condition
                    // looking for this activator
                    const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().searchCondition(acti->mRadioStation);
                    if (quest)
                    {
                        ESM4::FormId questId = quest->mFormId;
                        radioQuests[questId] = acti->mRadioStation;
                        std::cout << "dummy has acti radio quest " << quest->mEditorId << std::endl;
                    }
                }
            }
            }

            typedef CellRefList<ESM4::TalkingActivator>::List TalkingActivatorList;
            {
            const TalkingActivatorList& tact = cell->getForeignReadOnly<ESM4::TalkingActivator>().mList;
            for (TalkingActivatorList::const_iterator it = tact.begin(); it != tact.end(); ++it)
            {
                const MWWorld::LiveCellRef<ESM4::TalkingActivator>& ref = *it;
                const CellRef& tactRef = ref.mRef;
                const ESM4::TalkingActivator *tact = ref.mBase;

                if ((tact->mFlags & ESM4::TACT_RadioStation) != 0)
                {
                    const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().searchCondition(tact->mFormId);
                    if (quest)
                        std::cout << "has tact radio quest " << quest->mEditorId << std::endl;
                }
            }
            }
            {
            const TalkingActivatorList& tact = dummy->getForeignReadOnly<ESM4::TalkingActivator>().mList;
            for (TalkingActivatorList::const_iterator it = tact.begin(); it != tact.end(); ++it)
            {
                const MWWorld::LiveCellRef<ESM4::TalkingActivator>& ref = *it;
                const CellRef& tactRef = ref.mRef;
                const ESM4::TalkingActivator *tact = ref.mBase;

                if ((tact->mFlags & ESM4::TACT_RadioStation) != 0)
                {
                    const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().searchCondition(tact->mFormId);
                    if (quest)
                        std::cout << "dummy has tact radio quest " << quest->mEditorId << std::endl;
                }
            }
            }

            if (radioQuests.size())
            {
                std::map<ESM4::DialogInfo*, ESM4::FormId> radioInfos;
                const ForeignDialogue *dial = store.getForeign<ForeignDialogue>().search("RadioHello");
                if (dial)
                {
                    const std::map<ESM4::FormId, ESM4::DialogInfo*>& infos =  dial->getInfos();
                    std::map<ESM4::FormId, ESM4::DialogInfo*>::const_iterator it = infos.begin();
                    for (; it != infos.end(); ++it)
                    {
                        if (radioQuests.find(it->second->mQuest) != radioQuests.end())
                        {
                            radioInfos[it->second] = it->second->mResponseData.sound;
                        }
                    }
                }

                if (radioInfos.size())
                {
                    //ESM4::FormId newMusicId = radioInfos.rbegin()->second; // FIXME: for now just pick one
                    std::map<ESM4::DialogInfo*, ESM4::FormId>::iterator it = radioInfos.begin();
                    size_t i = Misc::Rng::rollDice(int(radioInfos.size()));
                    for (size_t j = 0; j < i; ++j)
                        ++it;

                    ESM4::FormId newMusicId = it->second; // FIXME: for now just pick a random one

                    const ESM4::Sound *newMusic = store.getForeign<ESM4::Sound>().search(newMusicId);
                    if (newMusic) // FO3 VaultTecHQ03 doesn't have any cell music or aspc
                    {
                        // stop Morrowind music
                        MWBase::Environment::get().getSoundManager()->playPlaylist("explore");
                        MWBase::Environment::get().getSoundManager()->stopMusic();

                        // play new radio music
#if 0
                        std::string soundFile = Misc::StringUtils::lowerCase(newMusic->mSoundFile);
                        std::cout << "interior cell radio Music " << soundFile << std::endl; // FIXME
                        MWBase::Environment::get().getSoundManager()->streamMusic(soundFile);
#else
                        std::string radioSoundId = ESM4::formIdToString(newMusicId);
                        if (radioSoundId != mCurrentRadioSoundId)
                        {
                            if (mCurrentRadioSoundId != "")
                                MWBase::Environment::get().getSoundManager()->stopSound(mCurrentRadioSound);

                            // FIXME: can radio play at the same time
                            MWBase::Environment::get().getSoundManager()->stopMusic();

                            mCurrentRadioSound =
                                MWBase::Environment::get().getSoundManager()->playSound(
                                    radioSoundId, 1.f, 1.f,
                                    MWBase::SoundManager::Play_TypeMusic,
                                    MWBase::SoundManager::Play_Loop);

                            mCurrentRadioSoundId = radioSoundId;

                            std::cout << "Playing Radio sound " << newMusic->mSoundFile << std::endl; // FIXME
                        }
                        else
                            std::cout << "Radio continuing old sound" << std::endl; // FIXME
#endif

                        isFONV = true;
                    }
                }
            }

            // FIXME: most ESM4::World doesn't seem to have music for FO3
            ESM4::FormId currMusicId = 0;
            if (mCurrentCell && mCurrentCell->isForeignCell())
            {
                const ForeignWorld *currWorld = store.getForeign<ForeignWorld>().find(currentWorldId);
                if (currWorld)
                    currMusicId = currWorld->mMusic;

                if (!currWorld || !currMusicId) // try cell
                    currMusicId = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mMusic;
            }

            const ESM4::World *newWorld = store.getForeign<ForeignWorld>().find(worldId);
            ESM4::FormId newMusicId = 0;
            if (newWorld)
                newMusicId = newWorld->mMusic;

            if (!newWorld || !newMusicId) // try cell
                newMusicId = static_cast<const MWWorld::ForeignCell*>(cell->getCell())->mCell->mMusic;

            bool playNewMusic = false;
            if (!mCurrentCell->isForeignCell() || !mCurrentCell->getCell()->isExterior())
                playNewMusic = true;

            std::string musicFile;
            if (playNewMusic || !(currMusicId && newMusicId && currMusicId == newMusicId))
            {
                const ESM4::Music *newMusic = store.getForeign<ESM4::Music>().search(newMusicId);
                if (newMusic)
                {
                    musicFile = newMusic->mMusicFile;
                    if (isFONV)
                        std::cout << "                   music clash" << std::endl;
                }
                else
                    musicFile = "Explore\\";

                playNewMusic = true;
            }

            if (!currMusicId && !newMusicId) // maybe TES4?
            {
                uint8_t currMusicType = 0;
                if (mCurrentCell->isForeignCell())
                    currMusicType = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mMusicType;

                uint8_t newMusicType = static_cast<const MWWorld::ForeignCell*>(cell->getCell())->mCell->mMusicType;
                if (currMusicType != newMusicType || !mCurrentCell->isForeignCell())
                {
                    if (newMusicType == 2)
                        musicFile = "dungeon\\";
                    if (newMusicType == 1)
                        musicFile = "public\\";
                    else
                    {
                        if ((newWorld->mWorldFlags & ESM4::World::WLD_Oblivion) != 0 || newWorld->mSound == 2)
                            musicFile = "dungeon\\";
                        else if (newWorld->mSound == 1)
                            musicFile = "public\\";
                        else
                            musicFile = "explore\\";
                    }

                    playNewMusic = true;
                }
            }

            if (!isFONV && playNewMusic)
            {
                // FIXME: fade in/out when changing music?
                if (musicFile.find('.') != std::string::npos)
                {
                    std::cout << "world stream Music " << musicFile << std::endl;
                    MWBase::Environment::get().getSoundManager()->streamMusic(musicFile);
                }
                else
                {
                    std::size_t pos = musicFile.find_first_of("\\");
                    if (pos != std::string::npos)
                    {
                        std::cout << "world Music " << musicFile.substr(0, pos) << std::endl;
                        MWBase::Environment::get().getSoundManager()->playPlaylist(musicFile.substr(0, pos));
                    }
                }
            }

            CellStoreCollection::iterator active = mActiveCells.begin();
            while (active != mActiveCells.end())
                unloadCell(active++);

            // FIXME: need to unload dummy and visible distant
        }
        else
        {
            CellStoreCollection::iterator active = mActiveCells.begin();
            while (active != mActiveCells.end())
            {
                if ((*active)->getCell()->isExterior() && // FIXME: should this be an assert instead?
                    (*active)->isForeignCell())
                {
                    if ((*active)->isDummyCell() || (*active)->isVisibleDistCell())
                    {
                        ++active;
                        continue;
                    }

                    if (std::abs(X - (*active)->getCell()->getGridX()) <= halfGridSize &&
                        std::abs(Y - (*active)->getCell()->getGridY()) <= halfGridSize)
                    {
                        // keep cells within the new grid
                        ++active;
                        continue;
                    }
                }
                unloadCell(active++); // discard cells thare are no longer in the grid (or internal)
            }
        }

        int refsToLoad = 0;
        // get the number of refs to load (for loading bar progress display)
        for (int x = X-halfGridSize; x <= X+halfGridSize; ++x)
        {
            for (int y = Y-halfGridSize; y <= Y+halfGridSize; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                // there will be no active cells for the scenario where we chanced worlds
                while (iter != mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x == (*iter)->getCell()->getGridX() &&
                        y == (*iter)->getCell()->getGridY() &&
                        !(*iter)->isDummyCell() &&
                        !(*iter)->isVisibleDistCell())
                        break;

                    ++iter;
                }
                // the counting of refs does not work for TES4, since we don't know how many refs exist
                // until they are loaded - we may need to estimate by looking at the size of
                // the group and dividing by some factor
                if (iter == mActiveCells.end())
                {
                    // this creates the CellStore and associated ForeignCell
                    CellStore* cell = MWBase::Environment::get().getWorld()->getWorldCell(worldId, x, y);

                    if (cell)
                        refsToLoad += cell->getRefrEstimate(ESM4::Grp_CellTemporaryChild);
                }
            }
        }

        CellStore *dummy = MWBase::Environment::get().getWorld()->getWorldDummyCell(worldId);
        if (dummy)
            refsToLoad += dummy->getPersistentRefrCount();

        loadingListener->setProgressRange(refsToLoad);

        // FIXME: load visibly distant REFR here?

        // Load cells
        for (int x = X-halfGridSize; x <= X+halfGridSize; ++x)
        {
            for (int y = Y-halfGridSize; y <= Y+halfGridSize; ++y)
            {
                CellStoreCollection::iterator iter = mActiveCells.begin();

                // loop through all active cells until x,y matches
                while (iter != mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x == (*iter)->getCell()->getGridX() &&
                        y == (*iter)->getCell()->getGridY() &&
                        !(*iter)->isDummyCell() &&
                        !(*iter)->isVisibleDistCell())
                        break;

                    ++iter;
                }

                if (iter == mActiveCells.end()) // only load cells that are not already active
                {
                    CellStore* cell = MWBase::Environment::get().getWorld()->getWorldCell(worldId, x, y);

                    if (cell)
                    {
                        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

                        if (result.second)
                            loadForeignCell(cell, loadingListener, worldId);
                    }
                    else
                    {
                       // FIXME: create dynamic cells instead?  See getWorldCell in cells.c
                        //std::cout << "no cell at " << x << ", " << y << std::endl;
                    }
                }
            }
        }

        // A dummy cell for the world formid store in:
        //     Store<MWWorld::ForeignCell>::preload()
        // A dummy cell is created in:
        //     MWWorld::Cells::getWorldCell(ESM4::FormId worldId, int x, int y)

        // check for dummy cell
        // FIXME: having the dummy cell results in *all* the doors being rendered!  Need to be
        // able to limit rendering based on the ref's position
        if (worldChanged && !mActiveCells.empty())
        {
            CellStore *dummy = MWBase::Environment::get().getWorld()->getWorldDummyCell(worldId);
            if (dummy)
            {
                std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(dummy);
                if (result.second)
                {
//                  std::cout << "dummy " << std::endl;
                    loadForeignCell(dummy, loadingListener);
                }
            }
            else // create a new one
            {
            }
//#if 0
            // FIXME: having one visible distant per world is not going to work, especially
            // with trees
            CellStore *dist = MWBase::Environment::get().getWorld()->getWorldVisibleDistCell(worldId);
            if (dist)
            {
                std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(dist);
                if (result.second)
                {
//                  std::cout << "dist " << std::endl;
                    loadVisibleDist(dist, loadingListener);
                }
            }
//#endif
        }

        MWBase::Environment::get().getWindowManager()->changeCell(cell);

        mCellChanged = true;

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;

        return cell;
    }

    //We need the ogre renderer and a scene node.
    Scene::Scene (MWRender::RenderingManager& rendering, PhysicsSystem *physics)
    : mCurrentCell (0), mCellChanged (false), mPhysics(physics), mRendering(rendering), mNeedMapUpdate(false)
    {
    }

    Scene::~Scene()
    {
    }

    bool Scene::hasCellChanged() const
    {
        return mCellChanged;
    }

    const Scene::CellStoreCollection& Scene::getActiveCells() const
    {
        return mActiveCells;
    }

    void Scene::changeToInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        CellStore *cell = MWBase::Environment::get().getWorld()->getInterior(cellName);
        if (!cell || !cell->getCell())
            return; // probably spelling error from console

        bool loadcell = (mCurrentCell == NULL);
        if(!loadcell)
            loadcell = *mCurrentCell != *cell;

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        std::string loadingInteriorText = "#{sLoadingMessage2}";
        loadingListener->setLabel(loadingInteriorText);
        Loading::ScopedLoad load(loadingListener);

        mRendering.enableTerrain(false);

        if(!loadcell)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->moveObject(world->getPlayerPtr(), position.pos[0], position.pos[1], position.pos[2]);

            float x = Ogre::Radian(position.rot[0]).valueDegrees();
            float y = Ogre::Radian(position.rot[1]).valueDegrees();
            float z = Ogre::Radian(position.rot[2]).valueDegrees();
            world->rotateObject(world->getPlayerPtr(), x, y, z);

            world->getPlayerPtr().getClass().adjustPosition(world->getPlayerPtr(), true);
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
            return;
        }

        std::cout << "Changing to interior\n";

        // unload
        int current = 0;
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
        {
            unloadCell (active++);
            ++current;
        }

        int refsToLoad = cell->count();
        loadingListener->setProgressRange(refsToLoad);

        // Load cell.
        loadCell (cell, loadingListener);

        changePlayerCell(cell, position, true);

        // adjust fog
        mRendering.configureFog(*mCurrentCell);

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mCellChanged = true; MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;
    }

    void Scene::changeToForeignInteriorCell (const std::string& cellName, const ESM::Position& position)
    {
        CellStore *cell = MWBase::Environment::get().getWorld()->getForeignInterior(cellName);
        if (!cell) // FIXME: why null?
            return;

        bool loadcell = (mCurrentCell == NULL);
        if(!loadcell)
            loadcell = *mCurrentCell != *cell;

        MWBase::Environment::get().getWindowManager()->fadeScreenOut(0.5);

        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        std::string loadingInteriorText = "#{sLoadingMessage2}";
        loadingListener->setLabel(loadingInteriorText);
        Loading::ScopedLoad load(loadingListener);

        mRendering.enableTerrain(false);

        if(!loadcell)
        {
            MWBase::World *world = MWBase::Environment::get().getWorld();
            world->moveObject(world->getPlayerPtr(), position.pos[0], position.pos[1], position.pos[2]);

            float x = Ogre::Radian(position.rot[0]).valueDegrees();
            float y = Ogre::Radian(position.rot[1]).valueDegrees();
            float z = Ogre::Radian(position.rot[2]).valueDegrees();
            world->rotateObject(world->getPlayerPtr(), x, y, z);

            world->getPlayerPtr().getClass().adjustPosition(world->getPlayerPtr(), true);
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);
            return;
        }

        std::cout << "Changing to foreign interior\n";

        // Music selection logic (just a guess):
        //
        // * if we are coming from TES3, use new interior music
        // * if we are coming from external, use new interior music
        // * if we are coming from another interior, use new only if different
        //   (unless there is no new music? if so continue prev or use "explore" as default?)
        bool fromAnotherWorld = false;
        bool playNewMusic = false;

        if (!mCurrentCell->isForeignCell()) // coming from TES3
            fromAnotherWorld = true;

        if (mCurrentCell->getCell()->isExterior()) // not coming from another interior
            fromAnotherWorld = true;

        if (fromAnotherWorld)
            MWBase::Environment::get().getSoundManager()->initForeign();

        bool isFONV = false; // FIXME: need better logic
        // FIXME: for an interior cell we don't need audio marker location?
        if (cell->getAudioLocation()) // FONV only
        {
            const MWWorld::ESMStore& store = MWBase::Environment::get().getWorld()->getStore();

            const ESM4::MediaLocationController *aloc
                = store.getForeign<ESM4::MediaLocationController>().search(cell->getAudioLocation());

            if (aloc)
            {
                isFONV = true;
                std::cout << "FONV: location interior music " << aloc->mEditorId << std::endl;

                // FIXME: using loop option 4 for testing
                switch (aloc->mMediaFlags.loopingOptions)
                {
                    case 0: // loop, use mDayStart/mNightStart
                    case 1: // random, use mDayStart/mNightStart
                    case 2: // retrigger, use mDayStart/mNightStart/mRetriggerDelay
                    case 3: // none (play once only?)
                    case 4: // loop, use 6:00/23:54
                    case 5: // random, use 6:00/23:54
                    case 6: // retrigger, use 6:00/23:54, mRetriggerDelay
                    case 7: // none (how is this different to case 3?)
                    default: break;
                }

                // media set to use
                if (aloc->mConditionalFaction)
                {
                    // FIXME: what happens here?
                    // (guess) if the conditional faction is found nearby to the player,
                    // choose between battle/enemy/friend/ally
                    std::cout << "FONV music faction" << std::endl;
                }
                else
                {
                    // NOTE: sometimes the specified sets are empty
                    // e.g. musCtrlAAAntiMusic - "Location"
                    std::uint16_t sets = aloc->mMediaFlags.factionNotFound;
                    //std::cout << "FONV: location sets " << sets << std::endl; // FIXME
                    const std::vector<ESM4::FormId>& mediaSets
                        =   (sets == 0) ? aloc->mNeutralSets :
                           ((sets == 1) ? aloc->mEnemySets   :
                           ((sets == 2) ? aloc->mAllySets    :
                           ((sets == 3) ? aloc->mFriendSets  :
                           /*sets == 4*/  aloc->mLocationSets)));

                    //std::cout << "FONV: location sets size " << mediaSets.size() << std::endl; // FIXME
                    ESM4::FormId mediaSetId = 0;
                    if (mediaSets.size() > 1)
                    {
                        size_t i = Misc::Rng::rollDice(int(mediaSets.size()));

                        mediaSetId = mediaSets[i];
                    }
                    else if (mediaSets.size() == 1)
                        mediaSetId = mediaSets[0];

                    const ESM4::MediaSet *mset = store.getForeign<ESM4::MediaSet>().search(mediaSetId);
                    if (mset)
                    {
                        if (mCurrentRadioSoundId != "") // FIXME: can both be played at the same time?
                            MWBase::Environment::get().getSoundManager()->stopSound(mCurrentRadioSound);

                        std::uint8_t enabled = mset->mEnabled;
                        std::bitset<8> bb(enabled);
                        std::cout << "FONV: location media set " << mset->mEditorId
                            << " type " << mset->mSetType << " enabled " << bb << std::endl;
                        MWBase::Environment::get().getSoundManager()->streamMediaSet(mediaSetId);
                    }
                    else
                        std::cout << "FONV: no location media set music" << std::endl;

                    ESM4::FormId aspcId
                        = static_cast<const MWWorld::ForeignCell*>(cell->getCell())->mCell->mAcousticSpace;
                    const ESM4::AcousticSpace *aspc = store.getForeign<ESM4::AcousticSpace>().search(aspcId);

                    if (aspc && aspc->mIsInterior)
                    {
                        ESM4::FormId soundId = aspc->mAmbientLoopSounds[0];
                        const ESM4::Sound *sound = store.getForeign<ESM4::Sound>().search(soundId);
                        if (sound)
                        {
                            // play new ambient sound
                            std::string ambientSoundId = ESM4::formIdToString(soundId);
                            if (ambientSoundId != mCurrentAmbientSoundId)
                            {
                                if (mCurrentAmbientSoundId != "")
                                    MWBase::Environment::get().getSoundManager()->stopSound(mCurrentAmbientSound);

                                mCurrentAmbientSound =
                                    MWBase::Environment::get().getSoundManager()->playSound(
                                        ambientSoundId, 1.f, 1.f,
                                        MWBase::SoundManager::Play_TypeMusic,
                                        MWBase::SoundManager::Play_Loop);

                                mCurrentAmbientSoundId = ambientSoundId;

                                std::cout << "Playing Acoustic Space ambient loop sound "
                                    << sound->mSoundFile << std::endl; // FIXME
                            }
                            else
                                std::cout << "Acoustic Space continuing old ambient loop sound"
                                    << std::endl; // FIXME
                        }
                    }
                    else
                        std::cout << "FONV: No location Acoustic Space ambient loop sound" << std::endl;
                }
            }
#if 0
            const ForeignDialogue *dial = store.getForeign<ForeignDialogue>().search("RadioHello");

            if (dial)
            {
                isFONV = true;

                const std::map<ESM4::FormId, ESM4::DialogInfo*>& infos =  dial->getInfos();
                std::map<ESM4::FormId, ESM4::DialogInfo*>::const_iterator it = infos.begin();
                for (; it != infos.end(); ++it)
                {
                    if (it->second->mTargetCondition.functionIndex == ESM4::FUN_GetQuestVariable)
                    {
                        // get quest
                        ESM4::FormId questId = it->second->mTargetCondition.param1; // formid, not adjusted

                        const ESM4::Quest* quest = store.getForeign<ESM4::Quest>().search(questId);
                        //std::cout << "RadioHello " << ESM4::formIdToString(questId) << std::endl;
                        if (quest)
                        {
                            if (quest->mTargetConditions[0].functionIndex == ESM4::FUN_GetIsID) // FIXME: can have multiple conditions
                            {
                                //std::cout << ESM4::formIdToString(quest->mTargetCondition.param1) << std::endl;
                                Ptr ptr = cell->search(ESM4::formIdToString(quest->mTargetConditions[0].param1));
                                if (ptr && it->second->mResponseData.sound)
                                {
                                    //std::cout << "RadioHello " << it->second->mResponse << std::endl;

            ESM4::FormId newMusicId = it->second->mResponseData.sound;
            const ESM4::Sound *newMusic
                = MWBase::Environment::get().getWorld()->getStore().getForeign<ESM4::Sound>().search(newMusicId);
            if (newMusic) // FO3 VaultTecHQ03 doesn't have any cell music or aspc
            {
                std::string musicFile = newMusic->mSoundFile;

                size_t pos = musicFile.find_last_of(".");
                std::string soundFile = musicFile.substr(0, pos) + "_mono.ogg";
                soundFile = ESM4::formIdToString(newMusicId);

                //if (!Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup("Meshes\\" + trimmedModel))
                std::cout << "interior cell stream Music " << soundFile << std::endl;
                MWBase::Environment::get().getSoundManager()->playPlaylist("explore", true);
                MWBase::Environment::get().getSoundManager()->stopMusic();
                if (mSound)
                    MWBase::Environment::get().getSoundManager()->stopSound(mSound);
                mSound = MWBase::Environment::get().getSoundManager()->playSound(soundFile, 1.f, 1.f,
                                        MWBase::SoundManager::Play_TypeMusic,
                                        MWBase::SoundManager::Play_Loop);
            }
                                }
                            }
                        }
                    }
                }
            }
#else
            // FONV: check if there are any radio activators in this cell
            typedef CellRefList<ESM4::Activator>::List ActivatorList;

            std::map</*ESM4::Quest**/ESM4::FormId, ESM4::FormId> radioQuests;
            const ActivatorList& acti = cell->getForeignReadOnly<ESM4::Activator>().mList;
            for (ActivatorList::const_iterator it = acti.begin(); it != acti.end(); ++it)
            {
                const MWWorld::LiveCellRef<ESM4::Activator>& ref = *it;
                const CellRef& actRef = ref.mRef;
                const ESM4::Activator *acti = ref.mBase;

                bool hasRadio = acti->mRadioStation != 0;

                //std::cout << acti->mEditorId << std::endl;

                if (hasRadio)
                {
                    // look for quests that are "start game enabled" and has a condition
                    // looking for this talking activator
                    const ESM4::Quest *quest
                        = store.getForeign<ESM4::Quest>().searchCondition(acti->mRadioStation);
                    if (quest)
                    {
                        ESM4::FormId questId = quest->mFormId;
                        radioQuests[questId] = acti->mRadioStation;
                        //std::cout << "FONV: radio int cell has acti radio quest "
                                  //<< quest->mEditorId << std::endl; // FIXME
                    }
                }
            }

            if (radioQuests.size())
            {
                std::map<ESM4::DialogInfo*, ESM4::FormId> radioInfos;
                // FIXME: hard coded "RadioHello"
                const ForeignDialogue *dial = store.getForeign<ForeignDialogue>().search("RadioHello");
                if (dial)
                {
                    const std::map<ESM4::FormId, ESM4::DialogInfo*>& infos =  dial->getInfos();
                    std::map<ESM4::FormId, ESM4::DialogInfo*>::const_iterator it = infos.begin();
                    for (; it != infos.end(); ++it)
                    {
                        if (radioQuests.find(it->second->mQuest) != radioQuests.end())
                        {
                            if (it->second->mResponseData.sound)
                                radioInfos[it->second] = it->second->mResponseData.sound;
                        }
                    }
                }

                if (radioInfos.size())
                {
                    //ESM4::FormId newMusicId = radioInfos.begin()->second; // FIXME: for now just pick one
                    std::map<ESM4::DialogInfo*, ESM4::FormId>::iterator it = radioInfos.begin();
                    size_t i = Misc::Rng::rollDice(int(radioInfos.size()));
                    for (size_t j = 0; j < i; ++j)
                        ++it;

                    ESM4::FormId newMusicId = it->second; // FIXME: for now just pick a random one

                    const ESM4::Sound *newMusic = store.getForeign<ESM4::Sound>().search(newMusicId);
                    if (newMusic) // FO3 VaultTecHQ03 doesn't have any cell music or aspc
                    {
                        // play new radio music
#if 0
                        std::string soundFile = Misc::StringUtils::lowerCase(newMusic->mSoundFile);
                        std::cout << "FONV: interior cell radio Music " << soundFile << std::endl; // FIXME
                        MWBase::Environment::get().getSoundManager()->streamMusic(soundFile);
#else
                        std::string radioSoundId = ESM4::formIdToString(newMusicId);
                        if (radioSoundId != mCurrentRadioSoundId)
                        {
                            if (mCurrentRadioSoundId != "")
                                MWBase::Environment::get().getSoundManager()->stopSound(mCurrentRadioSound);

                            // FIXME: can radio play at the same time
                            MWBase::Environment::get().getSoundManager()->stopMusic();

                            mCurrentRadioSound =
                                MWBase::Environment::get().getSoundManager()->playSound(
                                    radioSoundId, 1.f, 1.f,
                                    MWBase::SoundManager::Play_TypeMusic,
                                    MWBase::SoundManager::Play_Loop);

                            mCurrentRadioSoundId = radioSoundId;

                            std::cout << "Playing Radio sound " << newMusic->mSoundFile << std::endl; // FIXME
                        }
                        else
                            std::cout << "Radio continuing old sound" << std::endl; // FIXME
#endif
                        isFONV = true;
                    }
                }
            }

            // FIXME: not needed?
            typedef CellRefList<ESM4::TalkingActivator>::List TalkingActivatorList;
            {
            const TalkingActivatorList& tact = cell->getForeignReadOnly<ESM4::TalkingActivator>().mList;
            for (TalkingActivatorList::const_iterator it = tact.begin(); it != tact.end(); ++it)
            {
                const MWWorld::LiveCellRef<ESM4::TalkingActivator>& ref = *it;
                const CellRef& tactRef = ref.mRef;
                const ESM4::TalkingActivator *tact = ref.mBase;

                if ((tact->mFlags & ESM4::TACT_RadioStation) != 0)
                {
                    const ESM4::Quest *quest = store.getForeign<ESM4::Quest>().searchCondition(tact->mFormId);
                    //if (quest)
                        //std::cout << "FONV: has tact radio quest " << quest->mEditorId << std::endl; // FIXME
                }
            }
            }
#endif
        }

        ESM4::FormId currMusicId = 0;
        if (mCurrentCell && mCurrentCell->isForeignCell())
            currMusicId = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mMusic;

        ESM4::FormId newMusicId = static_cast<const MWWorld::ForeignCell*>(cell->getCell())->mCell->mMusic;

        std::string musicFile;
        if (fromAnotherWorld ||
                // if coming from another interior don't change if current music is "carry over"
                (currMusicId && newMusicId && currMusicId != newMusicId))
        {
            const ESM4::Music *newMusic
                = MWBase::Environment::get().getWorld()->getStore().getForeign<ESM4::Music>().search(newMusicId);
            if (newMusic) // FO3 VaultTecHQ03 doesn't have any cell music or aspc
            {
                musicFile = newMusic->mMusicFile;

                if (isFONV && musicFile.find('.') != std::string::npos)
                    std::cout << "music clash " << musicFile << std::endl;

                playNewMusic = true;
            }
        }

        if (!currMusicId && !newMusicId) // maybe TES4?
        {
            uint8_t currMusicType = 0;
            if (mCurrentCell->isForeignCell())
                currMusicType = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mMusicType;

            uint8_t newMusicType = static_cast<const MWWorld::ForeignCell*>(cell->getCell())->mCell->mMusicType;
            if (currMusicType != newMusicType || !mCurrentCell->isForeignCell())
            {
                if (newMusicType == 2)
                    musicFile = "dungeon\\";
                if (newMusicType == 1)
                    musicFile = "public\\";
                else
                {
                    std::uint16_t flags
                        = static_cast<const MWWorld::ForeignCell*>(cell->getCell())->mCell->mCellFlags;

                    if ((flags & ESM4::CELL_Public) != 0)
                        musicFile = "public\\";
                    else if ((flags & ESM4::CELL_HideLand) != 0)
                    {
                        std::cout << "Oblivion interior" << std::endl;
                        musicFile = "dungeon\\";
                    }
                    else
                        musicFile = "dungeon\\"; // default for interior is dungeon?
                        //musicFile = "explore\\";
                }

                playNewMusic = true;
            }
        }

        // FIXME: fade in/out when changing music?
        if (!isFONV && (fromAnotherWorld || playNewMusic)) // FIXME: need better logic
        {
            // play new music
            if (musicFile.find('.') != std::string::npos) // TODO: does this ever happen?
            {
                std::cout << "interior cell stream Music " << musicFile << std::endl;
                MWBase::Environment::get().getSoundManager()->streamMusic(musicFile);
            }
            else
            {
                std::size_t pos = musicFile.find_first_of("\\");
                if (pos != std::string::npos)
                {
                    std::cout << "interior cell Music " << musicFile.substr(0, pos) << std::endl;
                    MWBase::Environment::get().getSoundManager()->playPlaylist(musicFile.substr(0, pos));
                }
            }
        }

        // unload
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active!=mActiveCells.end())
            unloadCell (active++);

        int refsToLoad = cell->getRefrEstimate(ESM4::Grp_CellTemporaryChild);
        loadingListener->setProgressRange(refsToLoad);

        // Load cell.
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);
        if (result.second)
            loadForeignCell (cell, loadingListener);

        changePlayerCell(cell, position, true); // cell becomes mCurrentCell here

        MWBase::Environment::get().getSoundManager()->initRegion();

        // adjust fog
        mRendering.configureFog(*mCurrentCell);

        // Sky system
        MWBase::Environment::get().getWorld()->adjustSky();

        mCellChanged = true; MWBase::Environment::get().getWindowManager()->fadeScreenIn(0.5);

        MWBase::Environment::get().getWindowManager()->changeCell(mCurrentCell);

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;
    }

    void Scene::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCellGrid(x, y);

        CellStore* cell = MWBase::Environment::get().getWorld()->getExterior(x, y);
// FIXME: testing
//#if 0
        if (mCurrentCell && mCurrentCell->isForeignCell() && !cell->isForeignCell())
            throw std::runtime_error("Scene::changeToExteriorCell from TES4 to Morrowind");
//#endif
        changePlayerCell(cell, position, adjustPlayerPos);

        mRendering.updateTerrain();
    }

    void Scene::changeToWorldCell (ESM4::FormId worldId, const ESM::Position& position, bool adjustPlayerPos)
    {
        // FIXME: How to handle TES4 style terrain?  Add updateTES4Terrain() method to RenderingManager?
        int x = 0;
        int y = 0;
        const int cellSize = 4096;

        x = static_cast<int>(std::floor(position.pos[0] / cellSize));
        y = static_cast<int>(std::floor(position.pos[1] / cellSize));

        // FIXME: add landscape LOD mesh here?
        // first, get a list of applicable grid positions based on the player's current grid
        // but what if the current grid is at the edge of the landscape LOD mesh?
        // in extreme case right at the corner?  do we then load 4?
        //
        // maybe keep loading at half way point, e.g. -48, -16, 16, 48
        //
        // Looks like it's <formid dec>.<x bottom left><y bottom left><size>.nif
        //
        //0,0 : 32,0 : 0,32 : 32,32
        //mRendering.addLandscape(worldId, x, y, "meshes\\landscape\\lod\\60.00.00.32.nif");
        // 0,32: 32,32: 0,59: 32,59
        //mRendering.addLandscape(worldId, x, y, "meshes\\landscape\\lod\\60.00.32.32.nif");
        // -64,32 : -64:59 : -32:32 : -32,59
        //mRendering.addLandscape(worldId, x, y, "meshes\\landscape\\lod\\60.-64.32.32.nif");

        int xLeft   = int((x - 16 - 32) / 32) * 32; // = int((x - 48) / 32) * 32
        int xRight  = int((x - 16 + 32) / 32) * 32; // = int((x + 16) / 32) * 32
        int yBottom = int((y - 16 - 32) / 32) * 32; // = int((y - 48) / 32) * 32
        int yTop    = int((y - 16 + 32) / 32) * 32; // = int((y + 16) / 32) * 32
#if 0
        //std::stringstream ss;
        //ss << std::setw(2) << std::setfill('0') << i;
        //std::string mesh = ss.str();
        std::vector<std::string> lodFiles;
        std::string mesh = "meshes\\landscape\\lod\\"+std::to_string(worldId)
                            +"."+((xLeft==0)? "0":"")+std::to_string(xLeft)+"."+((yBottom==0)? "0":"")+std::to_string(yBottom)+".32.nif";
        std::cout << mesh << std::endl;
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(mesh))
        {
            lodFiles.push_back(mesh);
            mRendering.addLandscape(worldId, x, y, lodFiles.back());
        }

        mesh = "meshes\\landscape\\lod\\"+std::to_string(worldId)
                            +"."+((xLeft==0)? "0":"")+std::to_string(xLeft)+"."+((yTop==0)? "0":"")+std::to_string(yTop)+".32.nif";
        std::cout << mesh << std::endl;
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(mesh))
        {
            lodFiles.push_back(mesh);
            mRendering.addLandscape(worldId, x, y, lodFiles.back());
        }

        mesh = "meshes\\landscape\\lod\\"+std::to_string(worldId)
                            +"."+((xRight==0)? "0":"")+std::to_string(xRight)+"."+((yBottom==0)? "0":"")+std::to_string(yBottom)+".32.nif";
        std::cout << mesh << std::endl;
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(mesh))
        {
            lodFiles.push_back(mesh);
            mRendering.addLandscape(worldId, x, y, lodFiles.back());
        }
        mesh = "meshes\\landscape\\lod\\"+std::to_string(worldId)
                            +"."+((xRight==0)? "0":"")+std::to_string(xRight)+"."+((yTop==0)? "0":"")+std::to_string(yTop)+".32.nif";
        std::cout << mesh << std::endl;
        if (Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(mesh))
        {
            lodFiles.push_back(mesh);
            mRendering.addLandscape(worldId, x, y, lodFiles.back());
        }
#endif
        // objects
        CellStore *current = updateWorldCellsAtGrid(worldId, x, y);
        if (!current)
            return; // FIXME

        changePlayerCell(current, position, adjustPlayerPos);

        mRendering.updateTerrain();
    }

    CellStore* Scene::getCurrentCell ()
    {
        return mCurrentCell;
    }

    void Scene::markCellAsUnchanged()
    {
        mCellChanged = false;
    }

    void Scene::insertCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener)
    {
        InsertFunctor functor (cell, rescale, *loadingListener, *mPhysics, mRendering);
        cell.forEach (functor);
    }

    void Scene::insertForeignCell (CellStore &cell, bool rescale, Loading::Listener* loadingListener)
    {
        InsertFunctor functor (cell, rescale, *loadingListener, *mPhysics, mRendering);
        cell.forEach (functor/*, 0, 0, 0, 0*/);
    }

    void Scene::addObjectToScene (const Ptr& ptr)
    {
        try
        {
            addObject(ptr, *mPhysics, mRendering);
            MWBase::Environment::get().getWorld()->rotateObject(ptr, 0, 0, 0, true);
            MWBase::Environment::get().getWorld()->scaleObject(ptr, ptr.getCellRef().getScale());
        }
        catch (std::exception& e)
        {
            std::cerr << "error during rendering: " << e.what() << std::endl;
        }
    }

    void Scene::removeObjectFromScene (const Ptr& ptr)
    {
        MWBase::Environment::get().getMechanicsManager()->remove (ptr);
        MWBase::Environment::get().getSoundManager()->stopSound3D (ptr);
        mPhysics->removeObject (ptr.getRefData().getHandle());
        mRendering.removeObject (ptr);
    }

    bool Scene::isCellActive(const CellStore &cell)
    {
        CellStoreCollection::iterator active = mActiveCells.begin();
        while (active != mActiveCells.end()) {
            if (**active == cell) {
                return true;
            }
            ++active;
        }
        return false;
    }

    Ptr Scene::searchPtrViaHandle (const std::string& handle)
    {
        for (CellStoreCollection::const_iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            if (Ptr ptr = (*iter)->searchViaHandle (handle))
                return ptr;

        return Ptr();
    }

    Ptr Scene::searchPtrViaActorId (int actorId)
    {
        for (CellStoreCollection::const_iterator iter (mActiveCells.begin());
            iter!=mActiveCells.end(); ++iter)
            if (Ptr ptr = (*iter)->searchViaActorId (actorId))
                return ptr;

        return Ptr();
    }
}
