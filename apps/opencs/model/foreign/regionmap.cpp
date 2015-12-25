#include "regionmap.hpp"

#include <cmath>
#include <algorithm>

#include <QBrush>

#include <components/misc/stringops.hpp>

#include "../world/data.hpp"
#include "../world/universalid.hpp"
#include "../world/regionmap.hpp"

CSMForeign::RegionMap::CellDescription::CellDescription() : mDeleted (false) {}

CSMForeign::RegionMap::CellDescription::CellDescription (const CSMWorld::Record<Cell>& cell)
{
    const Cell& cell2 = cell.get();

    if (cell2.mWorld != "Tamriel") // FIXME: make this configurable
        throw std::logic_error ("Interior cell in region map");

    mDeleted = cell.isDeleted();

    mRegion = cell2.mRegion;
    mName = cell2.mName;
}

CSMWorld::CellCoordinates CSMForeign::RegionMap::getIndex (const QModelIndex& index) const
{
    return mMin.move (index.column(), mMax.getY()-mMin.getY() - index.row()-1);
}

QModelIndex CSMForeign::RegionMap::getIndex (const CSMWorld::CellCoordinates& index) const
{
    // I hate you, Qt API naming scheme!
    return QAbstractTableModel::index (mMax.getY()-mMin.getY() - (index.getY()-mMin.getY())-1,
        index.getX()-mMin.getX());
}

CSMWorld::CellCoordinates CSMForeign::RegionMap::getIndex (const Cell& cell) const
{
    std::istringstream stream (cell.mName); // cells in Tamriel should hav names in the form of "#x y"

    char ignore;
    int x = 0;
    int y = 0;
    stream >> ignore >> x >> y;

    return CSMWorld::CellCoordinates (x, y);
}

void CSMForeign::RegionMap::buildRegions()
{
    const RegionCollection& regions = mData.getForeignRegions();

    int size = regions.getSize();

    for (int i=0; i<size; ++i)
    {
        const CSMWorld::Record<Region>& region = regions.getRecord (i);

        if (!region.isDeleted())
            mColours.insert (std::make_pair (Misc::StringUtils::lowerCase (region.get().mId),
                region.get().mMapColor));
    }
}

void CSMForeign::RegionMap::buildMap()
{
    const CellCollection& cells = mData.getForeignCells();
    const RegionCollection& regions = mData.getForeignRegions();

    int size = cells.getSize();

    for (int i=0; i < size; ++i)
    {
        const CSMWorld::Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.mWorld == "Tamriel") // FIXME: make this configurable
        {
            CellDescription description (cell);

            // update mRegion here (the last one was used in CellCollections)
            if (!cell2.mRegions.empty())
            {
                // strategy:
                //   if the only one then use it
                //   if map name exists (type 0x04 or RDAT_Map) for the region use the highest priority
                //   else make no change
                unsigned int size = cell2.mRegions.size();
                if (size == 1)
                {
                    char buf[8+1];
                    int res = 0;
                    res = snprintf(buf, 8+1, "%08x", cell2.mRegions.back());

                    if (res > 0 && res < 8+1)
                        description.mRegion.assign(buf);
                    else
                        throw std::runtime_error("possible buffer overflow on formId");
                }
                else
                {
                    std::string regionString;
                    std::string mapRegionString;
                    int priority = 0;
                    int mapPriority = -1;
                    //int priority = record.mRegions.at(0).mData.pri;
                    for (unsigned int i = 1; i < size; ++i)
                    {
                        std::uint32_t regionId = cell2.mRegions[i];
                        // does this one have a map name?
                        char bufR[8+1];
                        int resR = snprintf(bufR, 8+1, "%08x", regionId);
                        if (resR > 0 && resR < 8+1)
                            regionString.assign(bufR);
                        else
                            throw std::runtime_error("possible buffer overflow on formId");

                        const Region& region = regions.getRecord(regionString).get();
                        if (!region.mMapName.empty())
                        {
                            if (mapPriority == -1)
                            {
                                mapRegionString = regionString;
                                mapPriority = region.mData[0x04].priority;
                            }
                            else if (region.mData[0x04].priority > mapPriority)
                            {
                                mapRegionString = regionString;
                            }
                        }
                        //else
                    }
                    if (!mapRegionString.empty())
                        description.mRegion = mapRegionString;
                }

            }

            CSMWorld::CellCoordinates index = getIndex (cell2);

            mMap.insert (std::make_pair (index, description));
        }
    }

    std::pair<CSMWorld::CellCoordinates, CSMWorld::CellCoordinates> mapSize = getSize();

    mMin = mapSize.first;
    mMax = mapSize.second;
}

void CSMForeign::RegionMap::addCell (const CSMWorld::CellCoordinates& index, const CellDescription& description)
{
    std::map<CSMWorld::CellCoordinates, CellDescription>::iterator cell = mMap.find (index);

    if (cell!=mMap.end())
    {
        cell->second = description;
    }
    else
    {
        updateSize();

        mMap.insert (std::make_pair (index, description));
    }

    QModelIndex index2 = getIndex (index); // FIXME

    dataChanged (index2, index2);
}

void CSMForeign::RegionMap::addCells (int start, int end)
{
    const CellCollection& cells = mData.getForeignCells();

    for (int i=start; i<=end; ++i)
    {
        const CSMWorld::Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.mWorld == "Tamriel") // FIXME: make this configurable
        {
            CSMWorld::CellCoordinates index = getIndex (cell2);

            CellDescription description (cell);

            addCell (index, description);
        }
    }
}

void CSMForeign::RegionMap::removeCell (const CSMWorld::CellCoordinates& index)
{
    std::map<CSMWorld::CellCoordinates, CellDescription>::iterator cell = mMap.find (index);

    if (cell!=mMap.end())
    {
        mMap.erase (cell);

        QModelIndex index2 = getIndex (index); // FIXME

        dataChanged (index2, index2);

        updateSize();
    }
}

void CSMForeign::RegionMap::addRegion (const std::string& region, unsigned int colour)
{
    mColours[Misc::StringUtils::lowerCase (region)] = colour;
}

void CSMForeign::RegionMap::removeRegion (const std::string& region)
{
    std::map<std::string, unsigned int>::iterator iter (
        mColours.find (Misc::StringUtils::lowerCase (region)));

    if (iter!=mColours.end())
        mColours.erase (iter);
}

void CSMForeign::RegionMap::updateRegions (const std::vector<std::string>& regions)
{
    std::vector<std::string> regions2 (regions);

    std::for_each (regions2.begin(), regions2.end(), &Misc::StringUtils::lowerCase);
    std::sort (regions2.begin(), regions2.end());

    for (std::map<CSMWorld::CellCoordinates, CellDescription>::const_iterator iter (mMap.begin());
         iter!=mMap.end(); ++iter)
    {
        if (!iter->second.mRegion.empty() &&
            std::find (regions2.begin(), regions2.end(),
            Misc::StringUtils::lowerCase (iter->second.mRegion))!=regions2.end())
        {
            QModelIndex index = getIndex (iter->first); // FIXME

            dataChanged (index, index);
        }
    }
}

void CSMForeign::RegionMap::updateSize()
{
    std::pair<CSMWorld::CellCoordinates, CSMWorld::CellCoordinates> size = getSize();

    if (int diff = size.first.getX() - mMin.getX())
    {
        beginInsertColumns (QModelIndex(), 0, std::abs (diff)-1);
        mMin = CSMWorld::CellCoordinates (size.first.getX(), mMin.getY());
        endInsertColumns();
    }

    if (int diff = size.first.getY() - mMin.getY())
    {
        beginInsertRows (QModelIndex(), 0, std::abs (diff)-1);
        mMin = CSMWorld::CellCoordinates (mMin.getX(), size.first.getY());
        endInsertRows();
    }

    if (int diff = size.second.getX() - mMax.getX())
    {
        int columns = columnCount();

        if (diff>0)
            beginInsertColumns (QModelIndex(), columns, columns+diff-1);
        else
            beginRemoveColumns (QModelIndex(), columns+diff, columns-1);

        mMax = CSMWorld::CellCoordinates (size.second.getX(), mMax.getY());
        endInsertColumns();
    }

    if (int diff = size.second.getY() - mMax.getY())
    {
        int rows = rowCount();

        if (diff>0)
            beginInsertRows (QModelIndex(), rows, rows+diff-1);
        else
            beginRemoveRows (QModelIndex(), rows+diff, rows-1);

        mMax = CSMWorld::CellCoordinates (mMax.getX(), size.second.getY());
        endInsertRows();
    }
}

std::pair<CSMWorld::CellCoordinates, CSMWorld::CellCoordinates> CSMForeign::RegionMap::getSize() const
{
    const CellCollection& cells = mData.getForeignCells();

    int size = cells.getSize();

    CSMWorld::CellCoordinates min (0, 0);
    CSMWorld::CellCoordinates max (0, 0);

    for (int i=0; i<size; ++i)
    {
        const CSMWorld::Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.mWorld == "Tamriel") // FIXME: make this configurable
        {
            CSMWorld::CellCoordinates index = getIndex (cell2);

            if (min==max)
            {
                min = index;
                max = min.move (1, 1);
            }
            else
            {
                if (index.getX()<min.getX())
                    min = CSMWorld::CellCoordinates (index.getX(), min.getY());
                else if (index.getX()>=max.getX())
                    max = CSMWorld::CellCoordinates (index.getX()+1, max.getY());

                if (index.getY() < min.getY())
                    min = CSMWorld::CellCoordinates(min.getX(), index.getY());
                else if (index.getY() >= max.getY())
                    max = CSMWorld::CellCoordinates(max.getX(), index.getY() + 1);
            }
        }
    }

    return std::make_pair (min, max);
}

CSMForeign::RegionMap::RegionMap (CSMWorld::Data& data) : mData (data)
{
    buildRegions();
    buildMap();

    QAbstractItemModel *regions = data.getTableModel (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Regions));

    connect (regions, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (regionsAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (regions, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (regionsInserted (const QModelIndex&, int, int)));
    connect (regions, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (regionsChanged (const QModelIndex&, const QModelIndex&)));

    QAbstractItemModel *cells = data.getTableModel (CSMWorld::UniversalId (CSMWorld::UniversalId::Type_Cells));

    connect (cells, SIGNAL (rowsAboutToBeRemoved (const QModelIndex&, int, int)),
        this, SLOT (cellsAboutToBeRemoved (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (rowsInserted (const QModelIndex&, int, int)),
        this, SLOT (cellsInserted (const QModelIndex&, int, int)));
    connect (cells, SIGNAL (dataChanged (const QModelIndex&, const QModelIndex&)),
        this, SLOT (cellsChanged (const QModelIndex&, const QModelIndex&)));
}

int CSMForeign::RegionMap::rowCount (const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mMax.getY()-mMin.getY();
}

int CSMForeign::RegionMap::columnCount (const QModelIndex& parent) const
{
    if (parent.isValid())
        return 0;

    return mMax.getX()-mMin.getX();
}

QVariant CSMForeign::RegionMap::data (const QModelIndex& index, int role) const
{
    if (role==Qt::SizeHintRole)
        return QSize (16, 16);

    if (role==Qt::BackgroundRole)
    {
        /// \todo GUI class in non-GUI code. Needs to be addressed eventually.

        std::map<CSMWorld::CellCoordinates, CellDescription>::const_iterator cell =
            mMap.find (getIndex (index));

        if (cell!=mMap.end())
        {
            if (cell->second.mDeleted)
                return QBrush (Qt::red, Qt::DiagCrossPattern);

            std::map<std::string, unsigned int>::const_iterator iter =
                mColours.find (Misc::StringUtils::lowerCase (cell->second.mRegion));

            if (iter!=mColours.end())
                return QBrush (QColor (iter->second & 0xff,
                                       (iter->second >> 8) & 0xff,
                                       (iter->second >> 16) & 0xff));

            if (cell->second.mRegion.empty())
                return QBrush (Qt::Dense6Pattern); // no region

            return QBrush (Qt::red, Qt::Dense6Pattern); // invalid region
        }

        return QBrush (Qt::DiagCrossPattern);
    }

    if (role==Qt::ToolTipRole)
    {
        CSMWorld::CellCoordinates cellIndex = getIndex (index);

        std::ostringstream stream;

        stream << cellIndex;

        std::map<CSMWorld::CellCoordinates, CellDescription>::const_iterator cell =
            mMap.find (cellIndex);

        if (cell!=mMap.end())
        {
            if (!cell->second.mName.empty())
                stream << " " << cell->second.mName;

            if (cell->second.mDeleted)
                stream << " (deleted)";

            if (!cell->second.mRegion.empty())
            {
                stream << "<br>";

                std::map<std::string, unsigned int>::const_iterator iter =
                    mColours.find (Misc::StringUtils::lowerCase (cell->second.mRegion));

                if (iter!=mColours.end())
                {
                    // FIXME: inefficient hack
                    const RegionCollection& regions = mData.getForeignRegions();
                    const Region& region = regions.getRecord(cell->second.mRegion).get();

                    if (!region.mMapName.empty())
                        stream << region.mMapName;
                    else if (!region.mEditorId.empty())
                        stream << region.mEditorId;
                    else
                        stream << cell->second.mRegion;
                }
                else
                    stream << "<font color=red>" << cell->second.mRegion << "</font>";
            }
        }
        else
            stream << " (no cell)";

        return QString::fromUtf8 (stream.str().c_str());
    }

    if (role==CSMWorld::RegionMap::Role_Region)
    {
        CSMWorld::CellCoordinates cellIndex = getIndex (index);

        std::map<CSMWorld::CellCoordinates, CellDescription>::const_iterator cell =
            mMap.find (cellIndex);

        if (cell!=mMap.end() && !cell->second.mRegion.empty())
            return QString::fromUtf8 (Misc::StringUtils::lowerCase (cell->second.mRegion).c_str());
    }

    if (role==CSMWorld::RegionMap::Role_CellId)
    {
        CSMWorld::CellCoordinates cellIndex = getIndex (index);

        std::ostringstream stream;
        stream << "#" << cellIndex.getX() << " " << cellIndex.getY();

        return QString::fromUtf8 (stream.str().c_str());
    }

    return QVariant();
}

Qt::ItemFlags CSMForeign::RegionMap::flags (const QModelIndex& index) const
{
    return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

void CSMForeign::RegionMap::regionsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{
    std::vector<std::string> update;

    const RegionCollection& regions = mData.getForeignRegions();

    for (int i=start; i<=end; ++i)
    {
        const CSMWorld::Record<Region>& region = regions.getRecord (i);

        update.push_back (region.get().mId);

        removeRegion (region.get().mId);
    }

    updateRegions (update);
}

void CSMForeign::RegionMap::regionsInserted (const QModelIndex& parent, int start, int end)
{
    std::vector<std::string> update;

    const RegionCollection& regions = mData.getForeignRegions();

    for (int i=start; i<=end; ++i)
    {
        const CSMWorld::Record<Region>& region = regions.getRecord (i);

        if (!region.isDeleted())
        {
            update.push_back (region.get().mId);

            addRegion (region.get().mId, region.get().mMapColor);
        }
    }

    updateRegions (update);
}

void CSMForeign::RegionMap::regionsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // Note: At this point an additional check could be inserted to see if there is any change to the
    // columns we are interested in. If not we can exit the function here and avoid all updating.

    std::vector<std::string> update;

    const RegionCollection& regions = mData.getForeignRegions();

    for (int i=topLeft.row(); i<=bottomRight.column(); ++i)
    {
        const CSMWorld::Record<Region>& region = regions.getRecord (i);

        update.push_back (region.get().mId);

        if (!region.isDeleted())
            addRegion (region.get().mId, region.get().mMapColor);
        else
            removeRegion (region.get().mId);
    }

    updateRegions (update);
}

void CSMForeign::RegionMap::cellsAboutToBeRemoved (const QModelIndex& parent, int start, int end)
{
    const CellCollection& cells = mData.getForeignCells();

    for (int i=start; i<=end; ++i)
    {
        const CSMWorld::Record<Cell>& cell = cells.getRecord (i);

        const Cell& cell2 = cell.get();

        if (cell2.mWorld == "Tamriel") // FIXME: make this configurable
            removeCell (getIndex (cell2));
    }
}

void CSMForeign::RegionMap::cellsInserted (const QModelIndex& parent, int start, int end)
{
    addCells (start, end);
}

void CSMForeign::RegionMap::cellsChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight)
{
    // Note: At this point an additional check could be inserted to see if there is any change to the
    // columns we are interested in. If not we can exit the function here and avoid all updating.

    addCells (topLeft.row(), bottomRight.row());
}
