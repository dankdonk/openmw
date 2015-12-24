#include "landscapecollection.hpp"

#include <iostream> // FIXME

#ifdef NDEBUG // FIXME for debugging only
#undef NDEBUG
#endif

#include <extern/esm4/reader.hpp>

#include "../world/idcollection.hpp"
#include "../world/cell.hpp"
#include "../world/record.hpp"

CSMForeign::LandscapeCollection::LandscapeCollection (const CellCollection& cells)
  : mCells (cells)
{
}

CSMForeign::LandscapeCollection::~LandscapeCollection ()
{
}

// Can't reliably use cell grid as the id for LAND, since some cells can be "empty" or do not
// have an XCLC sub-record. e.g. OblivionMQKvatchBridge, TheFrostFireGlade and CheydinhalOblivion
int CSMForeign::LandscapeCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::Landscape record;
    //std::cout << "new Landscape " << std::hex << &record << std::endl; // FIXME

    std::string id;

    if (reader.hasCellGrid())
    {
        // LAND records should only occur in a exterior cell.  Ensure this because we will make use
        // of reader.currCellGrid() later.
        assert(reader.grp(3).type == ESM4::Grp_ExteriorCell &&
               reader.grp(2).type == ESM4::Grp_ExteriorSubCell &&
               reader.grp(1).type == ESM4::Grp_CellChild &&
               reader.grp(0).type == ESM4::Grp_CellTemporaryChild &&
               "LAND record found in an unexpected group heirarchy");

        std::ostringstream stream;
        // If using Morrowind cell size, need to divide by 2
        //stream << "#" << std::floor((float)reader.currCellGrid().grid.x/2)
               //<< " " << std::floor((float)reader.currCellGrid().grid.y/2);
        stream << "#" << reader.currCellGrid().grid.x << " " << reader.currCellGrid().grid.y;
        id = stream.str();
#if 0
        std::string padding = "";
        padding.insert(0, reader.stackSize()*2, ' ');
        std::cout << padding << "LAND: formId " << std::hex << reader.hdr().record.id << std::endl; // FIXME
        std::cout << padding << "LAND X " << std::dec << reader.currCellGrid().grid.x << ", Y " << reader.currCellGrid().grid.y << std::endl;
#endif
    }
    else
        id = std::to_string(reader.hdr().record.id); // use formId instead

    record.mName = id; // FIXME: temporary, note id overwritten below

    id = std::to_string(reader.hdr().record.id); // use formId instead

    // FIXME; should be using the formId as the lookup key
    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::Landscape>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::LandscapeCollection::loadRecord (CSMForeign::Landscape& record,
    ESM4::Reader& reader)
{
    record.load(reader, mCells);
}

int CSMForeign::LandscapeCollection::load (const CSMForeign::Landscape& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::Landscape>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::Landscape> > record2(
                new CSMWorld::Record<CSMForeign::Landscape>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::Landscape> > record2(
                new CSMWorld::Record<CSMForeign::Landscape>(
                    CSMWorld::Collection<CSMForeign::Landscape, CSMWorld::IdAccessor<CSMForeign::Landscape> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

#if 0
void CSMForeign::LandscapeCollection::addNestedRow(int row, int col, int position)
{
}

void CSMForeign::LandscapeCollection::removeNestedRows(int row, int column, int subRow)
{
}

QVariant CSMForeign::LandscapeCollection::getNestedData(int row,
        int column, int subRow, int subColumn) const
{
}

void CSMForeign::LandscapeCollection::setNestedData(int row,
        int column, const QVariant& data, int subRow, int subColumn)
{
}

NestedTableWrapperBase* CSMForeign::LandscapeCollection::nestedTable(int row, int column) const
{
}

void CSMForeign::LandscapeCollection::setNestedTable(int row,
        int column, const NestedTableWrapperBase& nestedTable)
{
}

int CSMForeign::LandscapeCollection::getNestedRowsCount(int row, int column) const
{
}

int CSMForeign::LandscapeCollection::getNestedColumnsCount(int row, int column) const
{
}

NestableColumn *CSMForeign::LandscapeCollection::getNestableColumn(int column)
{
}
#endif
