#include "scene.hpp"

#include <OgreSceneNode.h>

#include <extern/esm4/land.hpp>
#include <extern/esm4/cell.hpp>

#include <components/nif/niffile.hpp>
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
}


namespace MWWorld
{

    void Scene::updateObjectLocalRotation (const Ptr& ptr)
    {
        ::updateObjectLocalRotation(ptr, *mPhysics, mRendering);
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
        }

        // register local scripts
        // ??? Should this go into the above if block ???
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell);
    }

    void Scene::loadForeignCell (CellStore *cell, Loading::Listener* loadingListener)
    {
        std::cout << "loading cell " << cell->getCell()->getDescription() << std::endl;

        float verts = ESM4::Land::VERTS_PER_SIDE; // number of vertices per side
        float worldsize = ESM4::Land::REAL_SIZE;  // cell terrain size in world coords

        // Load terrain physics first...
        if (cell->getCell()->isExterior())
        {
            const ForeignLand *land =
                    MWBase::Environment::get().getWorld()->getStore().get<ForeignLand>().find(cell->getForeignLandId());

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

        cell->respawn();

        // ... then references. This is important for adjustPosition to work correctly.
        /// \todo rescale depending on the state of a new GMST
        insertCell (*cell, true, loadingListener);

        mRendering.cellAdded (cell); // calls mTerrain->loadCell()

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

        mRendering.configureAmbient(*cell); // FIXME

        // register local scripts
        MWBase::Environment::get().getWorld()->getLocalScripts().addCell (cell); // FIXME
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
            const float maxDistance = (8192/2 + 1024)/2; // 1/2 cell size + threshold
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

                changeWorldCellGrid(worldId, newX, newY);
                mRendering.updateTerrain();
            }
        }
    }

    void Scene::changeCellGrid (int X, int Y)
    {
        // FIXME: testing only
        if(mCurrentCell && mCurrentCell->isForeignCell())
            std::cout << "Error: changeCellGrid called from a foreign cell" << std::endl;

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
        mCurrentCell = cell; // FIXME: maybe CellStore can keep the worldspace formId?

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
    void Scene::changeWorldCellGrid (ESM4::FormId worldId, int X, int Y)
    {
        Loading::Listener* loadingListener = MWBase::Environment::get().getWindowManager()->getLoadingScreen();
        Loading::ScopedLoad load(loadingListener);

        // It seems that ICMarketDistrict needs its parent world's LAND
        //const ForeignCell *cell
                //ESM4::FormId parentWorldId
                    //= static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent;

        ESM4::FormId currentWorldId = 0;
        if (mCurrentCell->isForeignCell())
        {
            if (static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent != 0)
                currentWorldId = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mParent;
            else
                currentWorldId = static_cast<const MWWorld::ForeignCell*>(mCurrentCell->getCell())->mCell->mFormId;
        }

        CellStore* current = MWBase::Environment::get().getWorld()->getForeignWorld(worldId, X, Y);
        if (!current) // FIXME
            return;

        mRendering.enableTerrain(true, worldId);

        std::string loadingExteriorText = "#{sLoadingMessage3}";
        loadingListener->setLabel(loadingExteriorText);

        // TODO: For TES4/5 we should double this value? (cells are smaller)
        const int halfGridSize = Settings::Manager::getInt("exterior grid size", "Cells") / 1.5;
        if (worldId != currentWorldId)
        {
            int current = 0;
            CellStoreCollection::iterator active = mActiveCells.begin();
            while (active != mActiveCells.end())
            {
                unloadCell(active++);
                ++current;
            }
        }
        else
        {
            CellStoreCollection::iterator active = mActiveCells.begin();
            while (active != mActiveCells.end())
            {
                if ((*active)->getCell()->isExterior() && // FIXME: should this be an assert instead?
                    (*active)->isForeignCell())
                {
                    if ((*active)->isDummyCell())
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

                while (iter != mActiveCells.end())
                {
                    assert ((*iter)->getCell()->isExterior());

                    if (x == (*iter)->getCell()->getGridX() &&
                        y == (*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                // FIXME: add a worldspace param to getExterior?
                if (iter == mActiveCells.end())
                    refsToLoad += MWBase::Environment::get().getWorld()->getExterior(x, y)->count();
            }
        }

        loadingListener->setProgressRange(refsToLoad);

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
                        y == (*iter)->getCell()->getGridY())
                        break;

                    ++iter;
                }

                if (iter == mActiveCells.end()) // only load cells that are not already active
                {
                    CellStore* cell = MWBase::Environment::get().getWorld()->getForeignWorld(worldId, x, y);

                    if (cell)
                    {
                        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);

                        if (result.second)
                            loadForeignCell(cell, loadingListener);
                    }
                    else
                    {
                       // FIXME: create dynamic cells instead?  See getForeignWorld in cells.c
                        //std::cout << "no cell at " << x << ", " << y << std::endl;
                    }
                }
            }
        }

        // check for dummy cell
        // FIXME: having the dummy cell results in *all* the doors being rendered!  Need to be
        // able to limit rendering based on the ref's position
        if (!mActiveCells.empty())
        {
            CellStore *dummy = MWBase::Environment::get().getWorld()->getForeignWorldDummy(worldId);
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
        }

        MWBase::Environment::get().getWindowManager()->changeCell(current);

        mCellChanged = true;

        // Delay the map update until scripts have been given a chance to run.
        // If we don't do this, objects that should be disabled will still appear on the map.
        mNeedMapUpdate = true;
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
        std::pair<CellStoreCollection::iterator, bool> result = mActiveCells.insert(cell);
        if (result.second)
            loadForeignCell (cell, loadingListener);

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

    void Scene::changeToExteriorCell (const ESM::Position& position, bool adjustPlayerPos)
    {
        int x = 0;
        int y = 0;

        MWBase::Environment::get().getWorld()->positionToIndex (position.pos[0], position.pos[1], x, y);

        changeCellGrid(x, y);

        CellStore* current = MWBase::Environment::get().getWorld()->getExterior(x, y);
        changePlayerCell(current, position, adjustPlayerPos);

        mRendering.updateTerrain();
    }

    void Scene::changeToForeignWorldCell (ESM4::FormId worldId, const ESM::Position& position, bool adjustPlayerPos)
    {
        // FIXME: CellStore needs to support TES4 and worldspace
        // FIXME: How to handle TES4 style terrain?  Add updateTES4Terrain() method to RenderingManager?
        int x = 0;
        int y = 0;
        const int cellSize = 4096;

        x = static_cast<int>(std::floor(position.pos[0] / cellSize));
        y = static_cast<int>(std::floor(position.pos[1] / cellSize));

        changeWorldCellGrid(worldId, x, y);

        CellStore* current = MWBase::Environment::get().getWorld()->getForeignWorld(worldId, x, y); // FIXME
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
