#include "foreigninteriorwidget.hpp"

#include <sstream>
#include <iostream> // FIXME

#include <OgreColourValue.h>
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreCamera.h>

#include <QEvent>

#include "../../model/doc/document.hpp"

#include "../../model/world/data.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/pathgridcommands.hpp"

#include "../widget/scenetooltoggle.hpp"
#include "../widget/scenetooltoggle2.hpp"

#include "elements.hpp"

void CSVRender::ForeignInteriorWidget::update()
{
    const CSMWorld::Record<CSMForeign::Cell>& record =
        dynamic_cast<const CSMWorld::Record<CSMForeign::Cell>&> (mCellsModel->getRecord (mCellId));
#if 0

    ESM4::FormId formId = static_cast<ESM4::FormId>(std::stoi(mCellId, nullptr, 16));

    ForeignCell *cell
                = new ForeignCell (mDocument, getSceneManager(), formId, 0, mDocument.getPhysics());

    //connect (cell->getSignalHandler(), SIGNAL(flagAsModified()), this, SLOT(flagAsModSlot()));
    mCell.reset (cell);
#endif
    Ogre::ColourValue colour;
    colour.setAsABGR (record.get().mLighting.ambient);
    setDefaultAmbient (colour);

    /// \todo deal with mSunlight and mFog/mForDensity

    // FIXME: how to detect whether to apply fog?
    if (record.get().mLighting.fogNear != 0) // assume we don't have black fog...
    {
        colour.setAsABGR (record.get().mLighting.fogNear); // variable colour reused
        getSceneManager()->setFog(Ogre::FogMode::FOG_LINEAR, // FIXME: how to detect which mode?
                colour,
                0.001f, // unused for FOG_LINEAR
                record.get().mLighting.unknown1,  // near
                record.get().mLighting.unknown2); // far
    }

    flagAsModified();
}

// Call flow for creation of scene subview:
//
// CSVDoc::SubView::focusId()
// QMetaObject::activate()
// CSVDoc::View::qt_static_metacall()
// CSVDoc::View::addSubView()
// CSVDoc::SubViewFactoryManager::makeSubView()
// CSVDoc::SubViewFactory<CSVWorld::SceneSubView>::makeSubView()
// CSVWorld::SceneSubView::SceneSubView()
// CSVRender::ForeignInteriorWidget::ForeignInteriorWidget()
// /*CSVRender::ForeignInteriorWidget::update()*/


// FIXME: not sure if this use of hint system is the best way to go - needs more thought
//
// CSVWorld::Table::viewRecord()
//     --> emit editRequest(id, hint) where hint is "c:cell"
//                           ^   ^
//                           |   |
//                 CSMWorld::IdTable::view() supplies these based on available "features"
//
//             --> CSVDoc::SubView::focusId(id, hint) <-- not so sure, check
//                 --> slot CSVDoc::View::addSubView(id, hint)
//                     --> scene subview created
//                     --> scene subview useHint()
//                         --> WorldspaceWidget::useViewHint()


// Basically:
//
// IdTable::view() tells View::addSubView to create a SceneSubView by setting the type as Type_Scene,
//                 and SceneSubView to create ForeignInteriorWidget by setting id to sys::foreignInterior
//                 it also sets the hint to be "c:cellname"
//
// View::addSubView() creates a SceneSubView (actuall this class based on the id) then calls useHint()
//                 on SceneSubView, which in turn calls useViewHint()
void CSVRender::ForeignInteriorWidget::useViewHint (const std::string& hint)
{
#if 0
    if (!hint.empty())
    {
        if (hint[0]=='c')
        {
            // syntax: c:00000000
            //           ^       ^
            //           |       |
            //         0123456789
            // FIXME: check length of the string first
            mCellId = hint.substr(2, 8);
        }
        else if (hint[0]=='r')
        {
            /// \todo implement 'r' type hints
        }
        else
            throw std::runtime_error("unknown hint"); // FIXME: better message
    }

    update();
#endif
}

CSVRender::ForeignInteriorWidget::ForeignInteriorWidget (const std::string& cellId,
        QWidget* parent, CSMDoc::Document& document)
: WorldspaceWidget (document, parent), mCellId(cellId), mDocument(document)
{
    mCellsModel = &dynamic_cast<CSMWorld::IdTable&> (
        *document.getData().getTableModel (CSMWorld::UniversalId::Type_ForeignCells));

    //mReferenceablesModel = &dynamic_cast<CSMWorld::IdTable&> (
        //*document.getData().getTableModel (CSMWorld::UniversalId::Type_Referenceables));

    connect (mCellsModel, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellDataChanged (const QModelIndex&, const QModelIndex&)));
    connect (mCellsModel, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellRowsAboutToBeRemoved (const QModelIndex&, int, int)));

    update();

    ESM4::FormId formId = static_cast<ESM4::FormId>(std::stoi(mCellId, nullptr, 16));

    ForeignCell *cell
                = new ForeignCell (mDocument, getSceneManager(), formId, 0, mDocument.getPhysics());

    //connect (cell->getSignalHandler(), SIGNAL(flagAsModified()), this, SLOT(flagAsModSlot()));
    mCell.reset (cell);

    //Ogre::Vector3 corner
        //= cell->getSceneNode()->_getWorldAABB().getCorner(Ogre::AxisAlignedBox::NEAR_RIGHT_TOP);
    //Ogre::Vector3 center
        //= cell->getSceneNode()->_getWorldAABB().getCenter();

    //getCamera()->setPosition(Ogre::Vector3(center.x+300, center.y, center.z));
    //getCamera()->lookAt(center);
    //getCamera()->move(getCamera()->getDirection() * -2000); // FIXME: config setting
    //getCamera()->roll (Ogre::Degree (-90));
}

void CSVRender::ForeignInteriorWidget::cellDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    int index = mCellsModel->findColumnIndex (CSMWorld::Columns::ColumnId_Modification);
    QModelIndex cellIndex = mCellsModel->getModelIndex (mCellId, index);

    if (cellIndex.row()>=topLeft.row() && cellIndex.row()<=bottomRight.row())
    {
        if (mCellsModel->data (cellIndex).toInt()==CSMWorld::RecordBase::State_Deleted)
        {
            emit closeRequest();
        }
        else
        {
            /// \todo possible optimisation: check columns and update only if relevant columns have
            /// changed
            update();
        }
    }
}

void CSVRender::ForeignInteriorWidget::cellRowsAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    QModelIndex cellIndex = mCellsModel->getModelIndex (mCellId, 0);

    if (cellIndex.row()>=start && cellIndex.row()<=end)
        emit closeRequest();
}

bool CSVRender::ForeignInteriorWidget::handleDrop (const std::vector<CSMWorld::UniversalId>& dropData, DropType type)
{
    if (WorldspaceWidget::handleDrop (dropData, type))
        return true;

    if (type != Type_CellsForeign)
        return false;

    const CSMForeign::CellCollection& cells = mDocument.getData().getForeignCells();

    mCellId = dropData.begin()->getId();
    ESM4::FormId formId = static_cast<ESM4::FormId>(std::stoi(mCellId, nullptr, 16));
    int index = cells.searchId(formId);
    if (index == -1)
        return false;

    const CSMForeign::Cell& cellrec = cells.getRecord(cells.searchId(formId)).get();
    if (!cellrec.isInterior)
        return false;

    ForeignCell *cell
        = new ForeignCell (getDocument(), getSceneManager(), formId, 0, getDocument().getPhysics());

    //connect (cell->getSignalHandler(), SIGNAL(flagAsModified()), this, SLOT(flagAsModSlot()));
    mCell.reset (cell);

    update();
    emit cellChanged(*dropData.begin());

    return true;
}

void CSVRender::ForeignInteriorWidget::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mCell.get())
        if (mCell.get()->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::ForeignInteriorWidget::referenceableAboutToBeRemoved (
    const QModelIndex& parent, int start, int end)
{
    if (mCell.get())
        if (mCell.get()->referenceableAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::ForeignInteriorWidget::referenceableAdded (const QModelIndex& parent,
    int start, int end)
{
#if 0
    if (mCell.get())
    {
        QModelIndex topLeft = mReferenceablesModel->index (start, 0);
        QModelIndex bottomRight =
            mReferenceablesModel->index (end, mReferenceablesModel->columnCount());

        if (mCell.get()->referenceableDataChanged (topLeft, bottomRight))
            flagAsModified();
    }
#endif
}

void CSVRender::ForeignInteriorWidget::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mCell.get())
        if (mCell.get()->referenceDataChanged (topLeft, bottomRight))
            flagAsModified();
}

void CSVRender::ForeignInteriorWidget::referenceAboutToBeRemoved (const QModelIndex& parent,
    int start, int end)
{
    if (mCell.get())
        if (mCell.get()->referenceAboutToBeRemoved (parent, start, end))
            flagAsModified();
}

void CSVRender::ForeignInteriorWidget::referenceAdded (const QModelIndex& parent, int start,
    int end)
{
    if (mCell.get())
        if (mCell.get()->referenceAdded (parent, start, end))
            flagAsModified();
}

void CSVRender::ForeignInteriorWidget::addVisibilitySelectorButtons (
    CSVWidget::SceneToolToggle2 *tool)
{
    WorldspaceWidget::addVisibilitySelectorButtons (tool);
    tool->addButton (Element_Terrain, "Terrain", "", true);
    tool->addButton (Element_Fog, "Fog");
}

void CSVRender::ForeignInteriorWidget::pathgridDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    // FIXME:
}

std::string CSVRender::ForeignInteriorWidget::getStartupInstruction()
{
    Ogre::Vector3 position = getCamera()->getPosition();

    std::ostringstream stream;

    stream
        << "player->positionCell "
        << position.x << ", " << position.y << ", " << position.z
        << ", 0, \"" << mCellId << "\"";

    return stream.str();
}

CSVRender::WorldspaceWidget::dropRequirments CSVRender::ForeignInteriorWidget::getDropRequirements (CSVRender::WorldspaceWidget::DropType type) const
{
    dropRequirments requirements = WorldspaceWidget::getDropRequirements (type);

    if (requirements!=ignored)
        return requirements;

    switch(type)
    {
        case Type_CellsForeign:
            return canHandle;

        case Type_CellsInterior:
            return needUnpaged;

        case Type_CellsExterior:
            return needPaged;

        default:
            return ignored;
    }
}

void CSVRender::ForeignInteriorWidget::flagAsModSlot ()
{
    flagAsModified();
}