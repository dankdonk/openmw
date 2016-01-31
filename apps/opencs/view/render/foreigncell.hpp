#ifndef OPENCS_VIEW_FOREIGNCELL_H
#define OPENCS_VIEW_FOREIGNCELL_H

#include <string>
#include <map>
#include <memory>
#include <cstdint>

#include <boost/shared_ptr.hpp>

#include <OgreVector3.h>

#include <components/nifogre/objectscene.hpp> // FIXME
//#ifndef Q_MOC_RUN
#include <components/esm4terrain/terraingrid.hpp>
//#endif

#include "foreignobject.hpp"

class QModelIndex;

namespace Ogre
{
    class SceneManager;
    class SceneNode;
    class ManualObject;
}

namespace ESM4
{
    typedef std::uint32_t FormId;
}

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class Pathgrid;
    class NestedTableProxyModel;
    class IdTree;
    class SignalHandler;
}

namespace CSVWorld
{
    class PhysicsSystem;
}

namespace CSVRender
{
    class PathgridPoint;

    class ForeignCell
    {
            CSMDoc::Document& mDocument;
            ESM4::FormId mFormId;
            ESM4::FormId mWorld;
            Ogre::SceneNode *mCellNode;
            std::map<ESM4::FormId, ForeignObject *> mObjects;
            std::map<std::string, PathgridPoint *> mPgPoints;
            std::map<std::pair<int, int>, std::string> mPgEdges;

            CSMWorld::NestedTableProxyModel *mProxyModel;
            CSMWorld::IdTree *mModel;
            int mPgIndex;
            CSMWorld::SignalHandler *mHandler;

            std::auto_ptr<ESM4Terrain::TerrainGrid> mTerrain;
            boost::shared_ptr<CSVWorld::PhysicsSystem> mPhysics;
            Ogre::SceneManager *mSceneMgr;
            int mX;
            int mY;

            /// Ignored if cell does not have an object with the given ID.
            ///
            /// \return Was the object deleted?
            bool removeObject (ESM4::FormId id);

            /// Add objects from reference table that are within this cell.
            ///
            /// \return Have any objects been added?
            bool addObjects (const std::vector<ESM4::FormId>& objects);

            std::vector<NifOgre::ObjectScenePtr> mObjectParts; // FIXME: experiment

        public:

            ForeignCell (CSMDoc::Document& document, Ogre::SceneManager *sceneManager, ESM4::FormId id,
                         ESM4::FormId world,
                         boost::shared_ptr<CSVWorld::PhysicsSystem> physics,
                         const Ogre::Vector3& origin = Ogre::Vector3 (0, 0, 0));

            ~ForeignCell();

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            /// \return Did this call result in a modification of the visual representation of
            /// this cell?
            bool referenceAdded (const QModelIndex& parent, int start, int end);

            float getTerrainHeightAt(const Ogre::Vector3 &pos) const;

            Ogre::SceneNode *getSceneNode() { return mCellNode; } // for camera position

            void pathgridPointAdded(const Ogre::Vector3 &pos, bool interior = false);
            void pathgridPointMoved(const std::string &name,
                    const Ogre::Vector3 &newPos, bool interior = false);
            void pathgridPointRemoved(const std::string &name);

            void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void clearPathgrid();
            void buildPathgrid();
            CSMWorld::SignalHandler *getSignalHandler();

            void clearNavMesh();
            void buildNavMesh();

        private:

            // for drawing pathgrid points & lines
            void createGridMaterials();
            void destroyGridMaterials();
            void setupPathgrid();
            Ogre::ManualObject *createPathgridEdge(const std::string &name,
                    const Ogre::Vector3 &start, const Ogre::Vector3 &end);

            void addPathgridEdge();
            void removePathgridEdge();

            void setupNavMesh();    };
}

#endif
