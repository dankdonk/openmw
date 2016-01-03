#include "foreignobject.hpp"

#include "iostream" // FIXME

#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <OgreEntity.h>

#include "../../model/world/data.hpp"
#include "../../model/world/ref.hpp"
#include "../../model/world/refidcollection.hpp"

#include "../world/physicssystem.hpp"

#include "elements.hpp"

void CSVRender::ForeignObject::clearSceneNode (Ogre::SceneNode *node)
{
    for (Ogre::SceneNode::ObjectIterator iter = node->getAttachedObjectIterator();
        iter.hasMoreElements(); )
    {
        Ogre::MovableObject* object = dynamic_cast<Ogre::MovableObject*> (iter.getNext());
        node->getCreator()->destroyMovableObject (object);
    }

    for (Ogre::SceneNode::ChildNodeIterator iter = node->getChildIterator();
        iter.hasMoreElements(); )
    {
        Ogre::SceneNode* childNode = dynamic_cast<Ogre::SceneNode*> (iter.getNext());
        clearSceneNode (childNode);
        node->getCreator()->destroySceneNode (childNode);
   }
}

void CSVRender::ForeignObject::clear()
{
    mObject.setNull();

    if (mBase)
        clearSceneNode (mBase);
}

void CSVRender::ForeignObject::update()
{
    if(!mObject.isNull())
        mPhysics->removePhysicsObject(mBase->getName());

    clear();

    std::string model;
    int error = 0; // 1 referenceable does not exist, 2 referenceable does not specify a mesh

    //const CSMForeign::RefIdCollection& referenceables = mData.getReferenceables();
    const CSMForeign::IdCollection<CSMForeign::Container>& container = mData.getForeignContainers();
    const CSMForeign::IdCollection<CSMForeign::AnimObject>& anio = mData.getForeignAnimObjs();
    const CSMForeign::IdCollection<CSMForeign::MiscItem>& misc = mData.getForeignMiscItems();
    const CSMForeign::IdCollection<CSMForeign::Activator>& acti = mData.getForeignActivators();
    const CSMForeign::StaticCollection& referenceables = mData.getForeignStatics(); // FIXME: use statics only for now

    //int index = referenceables.searchId (mReferenceableId);
    // get the formId of the base object
    const CSMForeign::RefCollection& refs = mData.getForeignReferences();
    ESM4::FormId baseObj = refs.getRecord(refs.searchId(mReferenceId)).get().mBaseObj;
    int index = referenceables.searchId (ESM4::formIdToString(baseObj)); // FIXME: double conversion to string

    // FIXME: this is a massive hack to get around the lack of a referenceable table
    int extraIndex = -1;
    if (index==-1)
    {
        error = 1;

        int contIndex = container.searchId(ESM4::formIdToString(baseObj));
        if (contIndex != -1)
        {
            error = 0;

            model = container.getData (contIndex,
                   container.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).toString().toUtf8().constData();

            if (model.empty())
                error = 2;
        }
        else if (anio.searchId(ESM4::formIdToString(baseObj)) != -1)
            std::cout << "obj is an anim obj" << std::endl;
        else if (extraIndex = misc.searchId(ESM4::formIdToString(baseObj)) != -1)
        {
            //std::cout << "obj is an misc obj" << std::endl;
            model = misc.getData (extraIndex,
                   misc.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).toString().toUtf8().constData();

            if (model.empty())
                error = 2;
        }
        else if (extraIndex = acti.searchId(ESM4::formIdToString(baseObj)) != -1)
        {
            //std::cout << "obj is an acti obj" << std::endl;
            model = acti.getData (extraIndex,
                   acti.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).toString().toUtf8().constData();

            if (model.empty())
                error = 2;
        }
        //else
            //std::cout << "obj not static/anio/misc/acti/container " << ESM4::formIdToString(baseObj) << std::endl;
    }
    else
    {
        /// \todo check for Deleted state (error 1)

        model = referenceables.getData (index,
                referenceables.findColumnIndex (CSMWorld::Columns::ColumnId_Model)).toString().toUtf8().constData();

        if (model.empty())
            error = 2;

#if 0
        // sanity check position
        const CSMForeign::CellRef& reference = getReference();
        std::cout << "object cell " << floor(reference.mPos.pos[0]/4096) << ", "
                                    << floor(reference.mPos.pos[1]/4096) << ", "
                                    <<       reference.mPos.pos[2]       << std::endl;
#endif
    }

    if (error)
    {
        Ogre::Entity* entity = mBase->getCreator()->createEntity (Ogre::SceneManager::PT_CUBE);
        entity->setMaterialName("BaseWhite"); /// \todo adjust material according to error
        entity->setVisibilityFlags (Element_Reference);

        mBase->attachObject (entity);
    }
    else
    {
        // Oblivion.eam contains some paths with leading space but can't use below since
        // some file names have spaces
        //   model.erase(std::remove_if(model.begin(), model.end(), ::isspace), model.end());

        // first find the beginning of the filename
        std::string::size_type separator = model.find_last_of('\\');
        std::string filename;
        if (separator != std::string::npos)
            filename = model.substr(separator+1);
        else
            filename = model;

        // then trim the front
        std::string::size_type start = filename.find_first_not_of(' ');

        if (start == std::string::npos)
            throw std::runtime_error("empty model " + model); // huh? just checked above

        // now put it back together
        std::string trimmedModel = model.substr(0, separator+1) + filename.substr(start);

        //std::cout << "Using model: " << model << std::endl;
        mObject = NifOgre::Loader::createObjects (mBase, "Meshes\\" + trimmedModel);
        mObject->setVisibilityFlags (Element_Reference);

        if (mPhysics && !(mReferenceId == 0))
        {
            const CSMForeign::CellRef& reference = getReference();

            // position
            Ogre::Vector3 position;
            if (!mForceBaseToZero)
                position = Ogre::Vector3(reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]);

            // orientation
            Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);
            Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);
            Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

            // NOTE: physics system and mouse picking assume reference naming convension (i.e."^ref#")
            // FIXME: sometimes wrong model name is passed to Ogre
            mPhysics->addObject("meshes\\" + trimmedModel, mBase->getName(), "ref#"+ESM4::formIdToString(mReferenceId), reference.mScale, position, xr*yr*zr);
        }
    }
}

void CSVRender::ForeignObject::adjust()
{
    if (mReferenceId == 0)
        return;

    const CSMForeign::CellRef& reference = getReference();

    // position
    if (!mForceBaseToZero)
        mBase->setPosition (Ogre::Vector3 (
            reference.mPos.pos[0], reference.mPos.pos[1], reference.mPos.pos[2]));

    // orientation
    Ogre::Quaternion xr (Ogre::Radian (-reference.mPos.rot[0]), Ogre::Vector3::UNIT_X);

    Ogre::Quaternion yr (Ogre::Radian (-reference.mPos.rot[1]), Ogre::Vector3::UNIT_Y);

    Ogre::Quaternion zr (Ogre::Radian (-reference.mPos.rot[2]), Ogre::Vector3::UNIT_Z);

    mBase->setOrientation (xr*yr*zr);

    // scale
    mBase->setScale (reference.mScale, reference.mScale, reference.mScale);
}

const CSMForeign::CellRef& CSVRender::ForeignObject::getReference() const
{
    if (mReferenceId == 0)
        throw std::logic_error ("object does not represent a reference");

    const CSMForeign::RefCollection& refs = mData.getForeignReferences();
    return refs.getRecord (refs.searchId(mReferenceId)).get();
}

CSVRender::ForeignObject::ForeignObject (const CSMWorld::Data& data, Ogre::SceneNode *cellNode,
    ESM4::FormId id, bool referenceable, boost::shared_ptr<CSVWorld::PhysicsSystem> physics,
    bool forceBaseToZero)
: mData (data), mBase (0), mForceBaseToZero (forceBaseToZero), mPhysics(physics)
{
    mBase = cellNode->createChildSceneNode();

    mReferenceId = id;
#if 0
    if (referenceable)
    {
        mReferenceableId = id;
    }
    else
    {
        mReferenceId = id;
        mReferenceableId = getReference().mRefID;
    }
#endif

    update();
    adjust();
}

CSVRender::ForeignObject::~ForeignObject()
{
    clear();

    if (mBase)
    {
        if(mPhysics) // preview may not have physics enabled
            mPhysics->removeObject(mBase->getName());

        mBase->getCreator()->destroySceneNode (mBase);
    }
}

bool CSVRender::ForeignObject::referenceableDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        update();
        adjust();
        return true;
    }

    return false;
}

bool CSVRender::ForeignObject::referenceableAboutToBeRemoved (const QModelIndex& parent, int start,
    int end)
{
    const CSMWorld::RefIdCollection& referenceables = mData.getReferenceables();

    int index = referenceables.searchId (mReferenceableId);

    if (index!=-1 && index>=start && index<=end)
    {
        // Deletion of referenceable-type objects is handled outside of Object.
        if (!(mReferenceId == 0))
        {
            update();
            adjust();
            return true;
        }
    }

    return false;
}

bool CSVRender::ForeignObject::referenceDataChanged (const QModelIndex& topLeft,
    const QModelIndex& bottomRight)
{
    if (mReferenceId == 0)
        return false;

    const CSMForeign::RefCollection& references = mData.getForeignReferences();

    int index = references.searchId(mReferenceId);

    if (index!=-1 && index>=topLeft.row() && index<=bottomRight.row())
    {
        int columnIndex =
            references.findColumnIndex (CSMWorld::Columns::ColumnId_ReferenceableId);

        if (columnIndex>=topLeft.column() && columnIndex<=bottomRight.row())
        {
            mReferenceableId =
                references.getData (index, columnIndex).toString().toUtf8().constData();
        }

        update();
        adjust();
        return true;
    }

    return false;
}

std::string CSVRender::ForeignObject::getReferenceId() const
{
    return "";// mReferenceId;
}

std::string CSVRender::ForeignObject::getReferenceableId() const
{
    return mReferenceableId;
}
