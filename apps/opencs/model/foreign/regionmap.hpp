#ifndef CSM_FOREIGN_REGIONMAP_H
#define CSM_FOREIGN_REGIONMAP_H

#include <map>
#include <string>
#include <vector>
#include <cstdint>

#include <QAbstractTableModel>

#include "../world/record.hpp"
#include "cell.hpp"
#include "../world/cellcoordinates.hpp"

namespace CSMWorld
{
    class Data;
}

namespace CSMForeign
{
    /// \brief Model for the region map
    ///
    /// This class does not holds any record data (other than for the purpose of buffering).
    class RegionMap : public QAbstractTableModel
    {
            Q_OBJECT

        public:
#if 0
            enum Role
            {
                Role_Region = Qt::UserRole,
                Role_CellId = Qt::UserRole+1
            };
#endif
        private:

            struct CellDescription
            {
                bool mDeleted;
                std::string mRegion;
                std::string mName;
                std::string mFullName;

                CellDescription();

                CellDescription (const CSMWorld::Record<Cell>& cell);
            };

            CSMWorld::Data& mData;
            std::map<CSMWorld::CellCoordinates, CellDescription> mMap;
            CSMWorld::CellCoordinates mMin; ///< inclusive
            CSMWorld::CellCoordinates mMax; ///< exclusive
            std::map<std::string, std::uint32_t> mColours; ///< region ID, colour (RGBA)

            CSMWorld::CellCoordinates getIndex (const QModelIndex& index) const;
            ///< Translates a Qt model index into a cell index (which can contain negative components)

            QModelIndex getIndex (const CSMWorld::CellCoordinates& index) const;

            CSMWorld::CellCoordinates getIndex (const Cell& cell) const;

            void buildRegions();

            void buildMap();

            void addCell (const CSMWorld::CellCoordinates& index, const CellDescription& description);
            ///< May be called on a cell that is already in the map (in which case an update is
            // performed)

            void addCells (int start, int end);

            void removeCell (const CSMWorld::CellCoordinates& index);
            ///< May be called on a cell that is not in the map (in which case the call is ignored)

            void addRegion (const std::string& region, unsigned int colour);
            ///< May be called on a region that is already listed (in which case an update is
            /// performed)
            ///
            /// \note This function does not update the region map.

            void removeRegion (const std::string& region);
            ///< May be called on a region that is not listed (in which case the call is ignored)
            ///
            /// \note This function does not update the region map.

            void updateRegions (const std::vector<std::string>& regions);
            ///< Update cells affected by the listed regions

            void updateSize();

            std::pair<CSMWorld::CellCoordinates, CSMWorld::CellCoordinates> getSize() const;

        public:

            RegionMap (CSMWorld::Data& data);

            virtual int rowCount (const QModelIndex& parent = QModelIndex()) const;

            virtual int columnCount (const QModelIndex& parent = QModelIndex()) const;

            virtual QVariant data (const QModelIndex& index, int role = Qt::DisplayRole) const;
            ///< \note Calling this function with role==Role_CellId may return the ID of a cell
            /// that does not exist.

            virtual Qt::ItemFlags flags (const QModelIndex& index) const;

        private slots:

            void regionsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void regionsInserted (const QModelIndex& parent, int start, int end);

            void regionsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void cellsAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void cellsInserted (const QModelIndex& parent, int start, int end);

            void cellsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);
    };
}

#endif // CSM_FOREIGN_REGIONMAP_H
