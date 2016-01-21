#ifndef OPENCS_VIEW_FOREIGNINTERIORWIDGET_H
#define OPENCS_VIEW_FOREIGNINTERIORWIDGET_H

#include <string>
#include <memory>

#include "worldspacewidget.hpp"
#include "foreigncell.hpp"

class QModelIndex;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class IdTable;
}

namespace CSVRender
{
    class ForeignInteriorWidget : public WorldspaceWidget
    {
            Q_OBJECT

            std::string mCellId;
            CSMWorld::IdTable *mCellsModel;
            CSMWorld::IdTable *mReferenceablesModel;
            std::auto_ptr<ForeignCell> mCell;
            CSMDoc::Document& mDocument;

            void update();

        public:

            ForeignInteriorWidget (const std::string& cellId, QWidget *parent, CSMDoc::Document& document);

            virtual dropRequirments getDropRequirements(DropType type) const;

            /// \return Drop handled?
            virtual bool handleDrop (const std::vector<CSMWorld::UniversalId>& data,
                DropType type);

            void useViewHint (const std::string& hint);

        private:

            virtual void referenceableDataChanged (const QModelIndex& topLeft,
                const QModelIndex& bottomRight);

            virtual void referenceableAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void referenceableAdded (const QModelIndex& index, int start, int end);

            virtual void referenceDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            virtual void referenceAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void referenceAdded (const QModelIndex& index, int start, int end);

            virtual std::string getStartupInstruction();

        protected:

            virtual void addVisibilitySelectorButtons (CSVWidget::SceneToolToggle2 *tool);

        private slots:

            void cellDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void cellRowsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            virtual void flagAsModSlot();

            virtual void pathgridDataChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

        signals:

            void cellChanged(const CSMWorld::UniversalId& id);
    };
}

#endif
