#ifndef OPENCS_VIEW_FOREIGNOBJECT_H
#define OPENCS_VIEW_FOREIGNOBJECT_H

#include <cstdint>

#include <boost/shared_ptr.hpp>

#ifndef Q_MOC_RUN
#include <components/nifogre/ogrenifloader.hpp>
#endif

class QModelIndex;

namespace Ogre
{
    class SceneNode;
}

namespace ESM4
{
    typedef std::uint32_t FormId;
}

namespace CSMWorld
{
    class Data;
}

namespace CSMForeign
{
    struct CellRef;
}

namespace CSVWorld
{
    class PhysicsSystem;
}

namespace CSVRender
{
    class ForeignObject
    {
            const CSMWorld::Data& mData;
            ESM4::FormId mReferenceId;
            std::string mReferenceableId;
            Ogre::SceneNode *mBase;
            NifOgre::ObjectScenePtr mObject;
            bool mForceBaseToZero;
            boost::shared_ptr<CSVWorld::PhysicsSystem> mPhysics;
            std::string mPhysicsObject;

            /// Not implemented
            ForeignObject (const ForeignObject&);

            /// Not implemented
            ForeignObject& operator= (const ForeignObject&);

            /// Destroy all scene nodes and movable objects attached to node.
            static void clearSceneNode (Ogre::SceneNode *node);

            /// Remove object from node (includes deleting)
            void clear();

            /// Update model
            void update();

            /// Adjust position, orientation and scale
            void adjust();

            /// Throws an exception if *this was constructed with referenceable
            const CSMForeign::CellRef& getReference() const;

        public:

            ForeignObject (const CSMWorld::Data& data, Ogre::SceneNode *cellNode,
                ESM4::FormId id, bool referenceable,
                boost::shared_ptr<CSVWorld::PhysicsSystem> physics = boost::shared_ptr<CSVWorld::PhysicsSystem> (),
                bool forceBaseToZero = false);
            /// \param forceBaseToZero If this is a reference ignore the coordinates and place
            /// it at 0, 0, 0 instead.

            ~ForeignObject();

            /// \return Did this call result in a modification of the visual representation of
            /// this object?
            bool referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this object?
            bool referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this object?
            bool referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            /// Returns an empty string if this is a refereceable-type object.
            std::string getReferenceId() const;

            std::string getReferenceableId() const;

            NifOgre::ObjectScenePtr getObject() const;

            Ogre::SceneNode *getSceneNode() const;
    };
}

#endif
