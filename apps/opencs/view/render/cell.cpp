
#include "cell.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreManualObject.h>

#include <components/misc/stringops.hpp>
#include <components/esm/loadland.hpp>

#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"
#include "terrainstorage.hpp"
#include "pathgridpoint.hpp"

namespace CSVRender
{
    // PLEASE NOTE: pathgrid edge code copied and adapted from mwrender/debugging
    static const std::string PG_LINE_MATERIAL = "pathgridLineMaterial";
    static const int POINT_MESH_BASE = 35;
    static const std::string DEBUGGING_GROUP = "debugging";
}

void CSVRender::Cell::createGridMaterials()
{
    if(!Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
        Ogre::ResourceGroupManager::getSingleton().createResourceGroup(DEBUGGING_GROUP);

    if(Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP).isNull())
    {
        Ogre::MaterialPtr lineMatPtr =
            Ogre::MaterialManager::getSingleton().create(PG_LINE_MATERIAL, DEBUGGING_GROUP);
        lineMatPtr->setReceiveShadows(false);
        lineMatPtr->getTechnique(0)->setLightingEnabled(true);
        lineMatPtr->getTechnique(0)->getPass(0)->setDiffuse(1,1,0,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setAmbient(1,1,0);
        lineMatPtr->getTechnique(0)->getPass(0)->setSelfIllumination(1,1,0);
    }
}

void CSVRender::Cell::destroyGridMaterials()
{
    if(Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
    {
        if(!Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP).isNull())
            Ogre::MaterialManager::getSingleton().remove(PG_LINE_MATERIAL);

        Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(DEBUGGING_GROUP);
    }
}

Ogre::ManualObject *CSVRender::Cell::createPathgridEdge(const std::string &name,
        const Ogre::Vector3 &start, const Ogre::Vector3 &end)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject(name);

    //createGridMaterials();
    result->begin(PG_LINE_MATERIAL, Ogre::RenderOperation::OT_LINE_LIST);

    Ogre::Vector3 direction = (end - start);
    Ogre::Vector3 lineDisplacement = direction.crossProduct(Ogre::Vector3::UNIT_Z).normalisedCopy();
    // move lines up a little, so they will be less covered by meshes/landscape
    lineDisplacement = lineDisplacement * POINT_MESH_BASE + Ogre::Vector3(0, 0, 10);
    result->position(start + lineDisplacement);
    result->position(end + lineDisplacement);

    result->end();

    return result;
}

bool CSVRender::Cell::removeObject (const std::string& id)
{
    std::map<std::string, Object *>::iterator iter =
        mObjects.find (Misc::StringUtils::lowerCase (id));

    if (iter==mObjects.end())
        return false;

    delete iter->second;
    mObjects.erase (iter);
    return true;
}

bool CSVRender::Cell::addObjects (int start, int end)
{
    bool modified = false;

    const CSMWorld::RefCollection& collection = mData.getReferences();

    for (int i=start; i<=end; ++i)
    {
        std::string cell = Misc::StringUtils::lowerCase (collection.getRecord (i).get().mCell);

        CSMWorld::RecordBase::State state = collection.getRecord (i).mState;

        if (cell==mId && state!=CSMWorld::RecordBase::State_Deleted)
        {
            std::string id = Misc::StringUtils::lowerCase (collection.getRecord (i).get().mId);

            mObjects.insert (std::make_pair (id, new Object (mData, mCellNode, id, false, mPhysics)));
            modified = true;
        }
    }

    return modified;
}

CSVRender::Cell::Cell (CSMWorld::Data& data, Ogre::SceneManager *sceneManager,
    const std::string& id, boost::shared_ptr<CSVWorld::PhysicsSystem> physics, const Ogre::Vector3& origin)
: mData (data), mId (Misc::StringUtils::lowerCase (id)), mPhysics(physics), mSceneMgr(sceneManager), mX(0), mY(0)
{
    mCellNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    mCellNode->setPosition (origin);

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int rows = references.rowCount();

    addObjects (0, rows-1);

    const CSMWorld::IdCollection<CSMWorld::Land>& land = mData.getLand();
    int landIndex = land.searchId(mId);
    if (landIndex != -1)
    {
        const ESM::Land* esmLand = land.getRecord(mId).get().mLand.get();
        if(esmLand && esmLand->mDataTypes&ESM::Land::DATA_VHGT)
        {
            mTerrain.reset(new Terrain::TerrainGrid(sceneManager, new TerrainStorage(mData), Element_Terrain, true,
                                                    Terrain::Align_XY));
            mTerrain->loadCell(esmLand->mX,
                               esmLand->mY);

            float verts = ESM::Land::LAND_SIZE;
            float worldsize = ESM::Land::REAL_SIZE;
            mX = esmLand->mX;
            mY = esmLand->mY;
            mPhysics->addHeightField(sceneManager,
                    esmLand->mLandData->mHeights, mX, mY, 0, worldsize / (verts-1), verts);
        }
    }

    createGridMaterials();
    setupNavMesh();
    buildNavMesh();
}

CSVRender::Cell::~Cell()
{
    clearNavMesh();
    destroyGridMaterials();
    if (mTerrain.get())
        mPhysics->removeHeightField(mSceneMgr, mX, mY);

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        delete iter->second;

    mCellNode->getCreator()->destroySceneNode (mCellNode);
}

bool CSVRender::Cell::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    bool modified = false;

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            modified = true;

    return modified;
}

bool CSVRender::Cell::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    bool modified = false;

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            modified = true;

    return modified;
}

bool CSVRender::Cell::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int cellColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
    int stateColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Modification);

    // list IDs in cell
    std::map<std::string, bool> ids; // id, deleted state

    for (int i=topLeft.row(); i<=bottomRight.row(); ++i)
    {
        std::string cell = Misc::StringUtils::lowerCase (references.data (
            references.index (i, cellColumn)).toString().toUtf8().constData());

        if (cell==mId)
        {
            std::string id = Misc::StringUtils::lowerCase (references.data (
                references.index (i, idColumn)).toString().toUtf8().constData());

            int state = references.data (references.index (i, stateColumn)).toInt();

            ids.insert (std::make_pair (id, state==CSMWorld::RecordBase::State_Deleted));
        }
    }

    // perform update and remove where needed
    bool modified = false;

    for (std::map<std::string, Object *>::iterator iter (mObjects.begin());
        iter!=mObjects.end(); ++iter)
    {
        if (iter->second->referenceDataChanged (topLeft, bottomRight))
            modified = true;

        std::map<std::string, bool>::iterator iter2 = ids.find (iter->first);

        if (iter2!=ids.end())
        {
            if (iter2->second)
            {
                removeObject (iter->first);
                modified = true;
            }

            ids.erase (iter2);
        }
    }

    // add new objects
    for (std::map<std::string, bool>::iterator iter (ids.begin()); iter!=ids.end(); ++iter)
    {
        mObjects.insert (std::make_pair (
            iter->first, new Object (mData, mCellNode, iter->first, false, mPhysics)));

        modified = true;
    }

    return modified;
}

bool CSVRender::Cell::referenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mData.getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    bool modified = false;

    for (int row = start; row<=end; ++row)
        if (removeObject (references.data (
            references.index (row, idColumn)).toString().toUtf8().constData()))
            modified = true;

    return modified;
}

bool CSVRender::Cell::referenceAdded (const QModelIndex& parent, int start, int end)
{
    if (parent.isValid())
        return false;

    return addObjects (start, end);
}

float CSVRender::Cell::getTerrainHeightAt(const Ogre::Vector3 &pos) const
{
    if(mTerrain.get() != NULL)
        return mTerrain->getHeightAt(pos);
    else
        return -std::numeric_limits<float>::max();
}
void CSVRender::Cell::setupNavMesh()
{
    const CSMForeign::NavMeshCollection& navmeshes = mData.getNavMeshes();
    int index = navmeshes.searchId(mId);
    if(index != -1)
    {
        std::cout << "CSVRender::Cell: found navmesh" << std::endl;
    }
#if 0
    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    int index = pathgrids.searchId(mId);
    if(index != -1)
    {
        int col = pathgrids.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);

        mPgIndex = index; // keep a copy to save from searching mId all the time

        mModel = dynamic_cast<CSMWorld::IdTree *>(
                mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_Pathgrid));

        mProxyModel = new CSMWorld::NestedTableProxyModel (mModel->index(mPgIndex, col),
                CSMWorld::ColumnBase::Display_NestedHeader, mModel);

    }
#endif
}

void CSVRender::Cell::clearNavMesh()
{
#if 0
    // destroy manual objects (edges)
    for(std::map<std::pair<int, int>, std::string>::iterator iter = mPgEdges.begin();
        iter != mPgEdges.end(); ++iter)
    {
        if(mSceneMgr->hasManualObject((*iter).second))
        {
            Ogre::ManualObject *manual = mSceneMgr->getManualObject((*iter).second);
            Ogre::SceneNode *node = manual->getParentSceneNode();
            mSceneMgr->destroyManualObject((*iter).second);
            if(mSceneMgr->hasSceneNode(node->getName()))
                mSceneMgr->destroySceneNode(node);
        }
    }
    mPgEdges.clear();

    // destroy points
    for(std::map<std::string, PathgridPoint *>::iterator iter (mPgPoints.begin());
        iter!=mPgPoints.end(); ++iter)
    {
        delete iter->second;
    }
    mPgPoints.clear();
#endif
}

void CSVRender::Cell::buildNavMesh()
{
    const CSMForeign::NavigationCollection& navmeshes = mData.getNavigation();

    const std::map<std::string, std::vector<std::string> >& cellmap = navmeshes.cellToFormIds();

    if (cellmap.find(mId) == cellmap.end())
        return;

    const std::vector<std::string>& formids = cellmap.find(mId)->second;

    for (std::vector<std::string>::const_iterator it = formids.begin(); it != formids.end(); ++it)
    {
        int index = navmeshes.searchId(*it);
        const CSMForeign::NavMeshInfo &navmesh = navmeshes.getRecord(index).get();

        if (navmesh.islandInfo.empty())
            continue;

        for (std::vector<ESM4::Navigation::IslandInfo>::const_iterator it = navmesh.islandInfo.begin();
                it != navmesh.islandInfo.end(); ++it)
        {
            //std::cout << "CSVRender MW: X "
                //<< std::dec << (*it).cellGrid.grid.x << ", Y " << (*it).cellGrid.grid.y << std::endl;

            for (std::vector<ESM4::Vertex>::const_iterator i = (*it).verticies.begin();
                    i != (*it).verticies.end(); ++i)
            {
                std::string name = PathgridPoint::getName(mId, index);

                Ogre::Vector3 pos = Ogre::Vector3((*i).x, (*i).y, (*i).z);

                mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
            }
        }
    }
#if 0
    const CSMForeign::NavMeshCollection& navmeshes = mData.getNavMeshes();
    int index = navmeshes.searchId(mId);
    const CSMForeign::NavMesh &navmesh = navmeshes.getRecord(index).get();

    //int worldsize = ESM::Land::REAL_SIZE;
    for (std::vector<ESM4::NavMesh::NVNMstruct>::const_iterator it = navmesh.mData.begin();
            it != navmesh.mData.end(); ++it)
    {
        std::cout << "CSVRender MW: X "
            << std::dec << (*it).cellGrid.grid.x << ", Y " << (*it).cellGrid.grid.y << std::endl;

        for (std::vector<ESM4::Vertex>::const_iterator i = (*it).verticies.begin();
                i != (*it).verticies.end(); ++i)
        {
            std::string name = PathgridPoint::getName(mId, index);

            Ogre::Vector3 pos = Ogre::Vector3((*i).x, (*i).y, (*i).z);

            mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
        }
    }
#endif
#if 0
    if (!mModel)
        return;

    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    const CSMWorld::Pathgrid &pathgrid = pathgrids.getRecord(mPgIndex).get();

    int worldsize = ESM::Land::REAL_SIZE;

    std::vector<ESM::Pathgrid::Point>::const_iterator iter = pathgrid.mPoints.begin();
    for(int index = 0; iter != pathgrid.mPoints.end(); ++iter, ++index)
    {
        std::string name = PathgridPoint::getName(pathgrid.mId, index);

        Ogre::Vector3 pos =
            Ogre::Vector3(worldsize*mX+(*iter).mX, worldsize*mY+(*iter).mY, (*iter).mZ);

        mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
    }

    for(ESM::Pathgrid::EdgeList::const_iterator it = pathgrid.mEdges.begin();
        it != pathgrid.mEdges.end();
        ++it)
    {
        Ogre::SceneNode *node = mCellNode->createChildSceneNode();
        const ESM::Pathgrid::Edge &edge = *it;
        const ESM::Pathgrid::Point &p0 = pathgrid.mPoints[edge.mV0];
        const ESM::Pathgrid::Point &p1 = pathgrid.mPoints[edge.mV1];

        std::ostringstream stream;
        stream << pathgrid.mId << "_" << edge.mV0 << " " << edge.mV1;
        std::string name = stream.str();

        Ogre::ManualObject *line = createPathgridEdge(name,
            Ogre::Vector3(worldsize*mX+p0.mX, worldsize*mY+p0.mY, p0.mZ),
            Ogre::Vector3(worldsize*mX+p1.mX, worldsize*mY+p1.mY, p1.mZ));
        line->setVisibilityFlags(Element_Pathgrid);
        node->attachObject(line);

        mPgEdges.insert(std::make_pair(std::make_pair(edge.mV0, edge.mV1), name));
    }
#endif
}
