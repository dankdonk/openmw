#include "foreigncell.hpp"

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreManualObject.h>
#include <OgreMaterialManager.h>
#include <OgreTechnique.h>

#include <components/misc/stringops.hpp>
#include <components/esm/loadland.hpp>

#include "../../model/doc/document.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/refcollection.hpp"
#include "../../model/world/pathgrid.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/pathgridcommands.hpp"
#include "../../model/world/pathgridpointswrap.hpp"
#include "../../model/world/nestedtableproxymodel.hpp"
#include "../world/physicssystem.hpp"

#include "elements.hpp"
#include "../foreign/terrainstorage.hpp"
#include "pathgridpoint.hpp"

namespace CSVRender
{
    // PLEASE NOTE: pathgrid edge code copied and adapted from mwrender/debugging
    static const std::string PG_LINE_MATERIAL = "pathgridLineMaterial";
    static const int POINT_MESH_BASE = 35;
    static const std::string DEBUGGING_GROUP = "debugging";
}

void CSVRender::ForeignCell::createGridMaterials()
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

void CSVRender::ForeignCell::destroyGridMaterials()
{
    if(Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(DEBUGGING_GROUP))
    {
        if(!Ogre::MaterialManager::getSingleton().getByName(PG_LINE_MATERIAL, DEBUGGING_GROUP).isNull())
            Ogre::MaterialManager::getSingleton().remove(PG_LINE_MATERIAL);

        Ogre::ResourceGroupManager::getSingleton().destroyResourceGroup(DEBUGGING_GROUP);
    }
}

Ogre::ManualObject *CSVRender::ForeignCell::createPathgridEdge(const std::string &name,
        const Ogre::Vector3 &start, const Ogre::Vector3 &end)
{
    Ogre::ManualObject *result = mSceneMgr->createManualObject(name);

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

bool CSVRender::ForeignCell::removeObject (const std::string& id) // assume formId string
{
    std::map<ESM4::FormId, ForeignObject *>::iterator iter =
        mObjects.find (static_cast<ESM4::FormId>(std::stoi(id, nullptr, 16)));

    if (iter == mObjects.end())
        return false;

    delete iter->second;
    mObjects.erase (iter);
    return true;
}

bool CSVRender::ForeignCell::addObjects (const std::vector<ESM4::FormId>& objects)
{
    bool modified = false;

    const CSMForeign::RefCollection& refs = mDocument.getData().getForeignReferences();

    for (unsigned int i = 0; i < objects.size(); ++i)
    {
        const CSMWorld::Record<CSMForeign::CellRef>& record = refs.getRecord(refs.searchId(objects[i]));

        if (record.mState != CSMWorld::RecordBase::State_Deleted)
        {
            ESM4::FormId id = objects[i];
            //std::cout << "Adding object: " << ESM4::formIdToString(id) << std::endl;
            mObjects.insert(std::make_pair(id,
                        new ForeignObject(mDocument.getData(), mCellNode, id, false, mPhysics)));
            modified = true;
        }
    }

    return modified;
}

CSVRender::ForeignCell::ForeignCell (CSMDoc::Document& document, Ogre::SceneManager *sceneManager,
    ESM4::FormId id, boost::shared_ptr<CSVWorld::PhysicsSystem> physics, const Ogre::Vector3& origin)
: mDocument (document), mFormId (id)
, mProxyModel(0), mModel(0), mPgIndex(-1)/*, mHandler(new CSMWorld::SignalHandler(this))*/
, mPhysics(physics), mSceneMgr(sceneManager), mX(0), mY(0)
{
    mCellNode = sceneManager->getRootSceneNode()->createChildSceneNode();
    mCellNode->setPosition (origin);

    const CSMForeign::CellCollection& cells = mDocument.getData().getForeignCells();
    const CSMForeign::Cell& cell = cells.getRecord(cells.searchId(id)).get();

    const CSMForeign::LandCollection& lands = mDocument.getData().getForeignLands();
    int landIndex = lands.searchId(cell.mLandTemporary);

    addObjects(cell.mRefTemporary); // FIXME: ignore visible distant and persistent children for now
    addObjects(cell.mRefPersistent); // FIXME: ignore visible distant children for now
    addObjects(cell.mRefVisibleDistant);

    if (landIndex != -1)
    {
        std::istringstream stream (cell.mCellId.c_str());
        char ignore; // '#'
        int x = 0;
        int y = 0;
        stream >> ignore >> x >> y;

        const CSMForeign::Land& esmLand = lands.getRecord(landIndex).get();

        if (esmLand.getLandData (ESM4::Land::LAND_VHGT))
        {
            mTerrain.reset(new Terrain::TerrainGrid(sceneManager,
                                                    new CSVForeign::TerrainStorage(mDocument.getData()),
                                                    Element_Terrain,
                                                    true,
                                                    Terrain::Align_XY));
            mTerrain->loadCell(/*esmLand.mX*/x,
                               /*esmLand.mY*/y);

            float verts = ESM4::Land::VERTS_SIDE;
            float worldsize = ESM4::Land::REAL_SIZE;
            mX = /*esmLand.mX*/x;
            mY = /*esmLand.mY*/y;

            mPhysics->addHeightField(sceneManager,
                esmLand.getLandData(ESM4::Land::LAND_VHGT)->mHeights, mX, mY, 0, worldsize / (verts-1), verts);
        }
    }

    createGridMaterials();

    setupPathgrid();
    buildPathgrid();

    //setupNavMesh();
    //buildNavMesh();
}

CSVRender::ForeignCell::~ForeignCell()
{
    //clearNavMesh();

    clearPathgrid();

    destroyGridMaterials();

    delete mProxyModel;
    //delete mHandler;

    if (mTerrain.get())
        mPhysics->removeHeightField(mSceneMgr, mX, mY);

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
        delete iter->second;

    mCellNode->getCreator()->destroySceneNode (mCellNode);
}

bool CSVRender::ForeignCell::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    bool modified = false;

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
        if (iter->second->referenceableDataChanged (topLeft, bottomRight))
            modified = true;

    return modified;
}

bool CSVRender::ForeignCell::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    bool modified = false;

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
        if (iter->second->referenceableAboutToBeRemoved (parent, start, end))
            modified = true;

    return modified;
}

bool CSVRender::ForeignCell::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
#if 0
    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);
    int cellColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Cell);
    int stateColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Modification);

    // list IDs in cell
    std::map<ESM4::FormId, bool> ids; // id, deleted state

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

    for (std::map<ESM4::FormId, ForeignObject *>::iterator iter (mObjects.begin());
        iter != mObjects.end(); ++iter)
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
            iter->first, new ForeignObject (mDocument.getData(), mCellNode, iter->first, false, mPhysics)));

        modified = true;
    }

    return modified;
#endif
    return false;
}

bool CSVRender::ForeignCell::referenceAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    if (parent.isValid())
        return false;

    CSMWorld::IdTable& references = dynamic_cast<CSMWorld::IdTable&> (
        *mDocument.getData().getTableModel (CSMWorld::UniversalId::Type_References));

    int idColumn = references.findColumnIndex (CSMWorld::Columns::ColumnId_Id);

    bool modified = false;

    for (int row = start; row<=end; ++row)
        if (removeObject (references.data (
            references.index (row, idColumn)).toString().toUtf8().constData()))
            modified = true;

    return modified;
}

bool CSVRender::ForeignCell::referenceAdded (const QModelIndex& parent, int start, int end)
{
    if (parent.isValid())
        return false;

    return false;// addObjects(start, end); FIXME
}

float CSVRender::ForeignCell::getTerrainHeightAt(const Ogre::Vector3 &pos) const
{
    if(mTerrain.get() != NULL)
        return mTerrain->getHeightAt(pos);
    else
        return -std::numeric_limits<float>::max();
}

void CSVRender::ForeignCell::pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    CSMWorld::IdTree *pathgrids = dynamic_cast<CSMWorld::IdTree *>(
                mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_Pathgrid));

    int idColumn = pathgrids->findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    int colPaths = pathgrids->findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);
    //int colEdges = pathgrids->findColumnIndex(CSMWorld::Columns::ColumnId_PathgridEdges);

    // FIXME: how to detect adds/deletes/modifies?

    for (int i=topLeft.row(); i<=bottomRight.row(); ++i)
    {
        std::string cell = Misc::StringUtils::lowerCase (pathgrids->data (
            pathgrids->index (i, idColumn)).toString().toUtf8().constData());

        if (/*cell==mId && */colPaths >= topLeft.column() && colPaths <= bottomRight.column())
        {
            if (!mModel)
                setupPathgrid();

            //mHandler->rebuildPathgrid();
        }
    }
}

// FIXME:
//  - adding edges (need the ability to select a pathgrid and highlight)
//  - repainting edges while moving
void CSVRender::ForeignCell::setupPathgrid()
{
    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    int index = -1;// pathgrids.searchId(mId);
    if(index != -1)
    {
        int col = pathgrids.findColumnIndex(CSMWorld::Columns::ColumnId_PathgridPoints);

        mPgIndex = index; // keep a copy to save from searching mId all the time

        mModel = dynamic_cast<CSMWorld::IdTree *>(
                mDocument.getData().getTableModel(CSMWorld::UniversalId::Type_Pathgrid));

        mProxyModel = new CSMWorld::NestedTableProxyModel (mModel->index(mPgIndex, col),
                CSMWorld::ColumnBase::Display_NestedHeader, mModel);

    }
}

void CSVRender::ForeignCell::clearPathgrid()
{
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
}

// NOTE: getName() generates a string representation of mId+index to uniquely identify a
// pathgrid point.  The trouble is that the index can change when a pathgrid point is deleted.
// Need a new way of uniquely identifying a pathgrid point.
//
// A workaround is to re-generate the pathgrids and edges each time a point is deleted or
// undo() is called (probably via a signal)
void CSVRender::ForeignCell::buildPathgrid()
{
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
}

// NOTE: pos is in world coordinates
void CSVRender::ForeignCell::pathgridPointAdded(const Ogre::Vector3 &pos, bool interior)
{
    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    CSMWorld::Pathgrid pathgrid = pathgrids.getRecord(mPgIndex).get();

    std::string name = "";// PathgridPoint::getName(mId, pathgrid.mPoints.size()); // generate a new name

    mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));

    // store to document
    int worldsize = ESM::Land::REAL_SIZE;

    int x = pos.x;
    int y = pos.y;
    if(!interior)
    {
        x = x - (worldsize * mX);
        y = y - (worldsize * mY);
    }

    ESM::Pathgrid::Point point(x, y, (int)pos.z);
    point.mConnectionNum = 0;
    pathgrid.mPoints.push_back(point);
    // FIXME: update other scene managers

    pathgrid.mData.mS2 += 1; // increment the number of points

    // TODO: check for possible issue if this cell is deleted and undo() is actioned afterwards
    CSMWorld::ModifyPathgridCommand *cmd = new CSMWorld::ModifyPathgridCommand(*mModel,
            mProxyModel->getParentId(), mProxyModel->getParentColumn(),
            new CSMWorld::PathgridPointsWrap(pathgrid));
    //mHandler->connectToCommand(cmd);
    mDocument.getUndoStack().push(cmd);
    // emit signal here?
}

void CSVRender::ForeignCell::pathgridPointRemoved(const std::string &name)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(name);
    if(result.first == "")
        return;

    std::string pathgridId = result.first;
    int index = result.second;

    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    CSMWorld::Pathgrid pathgrid = pathgrids.getRecord(mPgIndex).get();

    // check if the point exists
    if(index < 0 || (unsigned int)index >= pathgrid.mPoints.size())
        return;

    int numToDelete = pathgrid.mPoints[index].mConnectionNum * 2; // for sanity check later
    int deletedEdgeCount = 0;

    // update edge indicies to account for the deleted pathgrid point
    std::vector<ESM::Pathgrid::Edge>::iterator iter = pathgrid.mEdges.begin();
    for (; iter != pathgrid.mEdges.end();)
    {
        if (((*iter).mV0 == index) || ((*iter).mV1 == index))
        {
            iter = pathgrid.mEdges.erase(iter);
            pathgrid.mPoints[index].mConnectionNum -= 1;
            deletedEdgeCount++; // for sanity check later
        }
        else
        {
            if ((*iter).mV0 > index)
                (*iter).mV0--;

            if ((*iter).mV1 > index)
                (*iter).mV1--;

            ++iter;
        }
    }
    pathgrid.mPoints.erase(pathgrid.mPoints.begin()+index);
    pathgrid.mData.mS2 -= 1; // decrement the number of points

    if(deletedEdgeCount != numToDelete)
    {
        // WARNING: continue anyway?  Or should this be an exception?
        std::cerr << "The no of edges del does not match the no of conn for: "
            << pathgridId + "_" + QString::number(index).toStdString() << std::endl;
    }

    // TODO: check for possible issue if this cell is deleted and undo() is actioned afterwards
    CSMWorld::ModifyPathgridCommand *cmd = new CSMWorld::ModifyPathgridCommand(*mModel,
            mProxyModel->getParentId(), mProxyModel->getParentColumn(),
            new CSMWorld::PathgridPointsWrap(pathgrid));
    //mHandler->connectToCommand(cmd);
    mDocument.getUndoStack().push(cmd);

    clearPathgrid();
    buildPathgrid();
}

// NOTE: newPos is in world coordinates
void CSVRender::ForeignCell::pathgridPointMoved(const std::string &name,
        const Ogre::Vector3 &newPos, bool interior)
{
    std::pair<std::string, int> result = PathgridPoint::getIdAndIndex(name);
    if(result.first == "")
        return;

    std::string pathgridId = result.first;
    int index = result.second;

    const CSMWorld::SubCellCollection<CSMWorld::Pathgrid>& pathgrids = mDocument.getData().getPathgrids();
    CSMWorld::Pathgrid pathgrid = pathgrids.getRecord(mPgIndex).get();

    // check if the point exists
    if(index < 0 || (unsigned int)index >= pathgrid.mPoints.size())
        return;

    int worldsize = ESM::Land::REAL_SIZE;

    int x = newPos.x;
    int y = newPos.y;
    if(!interior)
    {
        x = x - (worldsize * mX);
        y = y - (worldsize * mY);
    }

    pathgrid.mPoints[index].mX = x;
    pathgrid.mPoints[index].mY = y;
    pathgrid.mPoints[index].mZ = newPos.z;

    // TODO: check for possible issue if this cell is deleted and undo() is actioned afterwards
    CSMWorld::ModifyPathgridCommand *cmd = new CSMWorld::ModifyPathgridCommand(*mModel,
            mProxyModel->getParentId(), mProxyModel->getParentColumn(),
            new CSMWorld::PathgridPointsWrap(pathgrid));
    //mHandler->connectToCommand(cmd);
    mDocument.getUndoStack().push(cmd);

    clearPathgrid();
    buildPathgrid();
}

// FIXME: save to the document
void CSVRender::ForeignCell::addPathgridEdge()
{
    // check if the points exist
    // update the edges
    // store to document
    // FIXME: update other scene managers
}

// FIXME: save to the document
void CSVRender::ForeignCell::removePathgridEdge()
{
}

CSMWorld::SignalHandler *CSVRender::ForeignCell::getSignalHandler()
{
    return NULL;// mHandler;
}
void CSVRender::ForeignCell::setupNavMesh()
{
    // FIXME: index throws exception
    const CSMForeign::NavMeshCollection& navmeshes = mDocument.getData().getNavMeshes();
    int index = -1;// navmeshes.searchId(mId);
    if(index != -1)
    {
        std::cout << "CSVRender::ForeignCell: found navmesh" << std::endl;
    }
}

void CSVRender::ForeignCell::clearNavMesh()
{
    // FIXME
}

void CSVRender::ForeignCell::buildNavMesh()
{
    const CSMForeign::NavigationCollection& navmeshes = mDocument.getData().getNavigation();

    const std::map<std::string, std::vector<std::string> >& cellmap = navmeshes.cellToFormIds();

    if (cellmap.find(""/*mId*/) == cellmap.end())
        return;

    const std::vector<std::string>& formids = cellmap.find(/*mId*/"")->second;

    for (std::vector<std::string>::const_iterator iter = formids.begin(); iter != formids.end(); ++iter)
    {
        int index = navmeshes.searchId(*iter);
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
                std::string name = PathgridPoint::getName(""/*mId*/, index);

                Ogre::Vector3 pos = Ogre::Vector3((*i).x, (*i).y, (*i).z);

                mPgPoints.insert(std::make_pair(name, new PathgridPoint(name, mCellNode, pos, mPhysics)));
            }
        }
    }
}