#include "landscapecollection.hpp"

#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>

#include "../world/idcollection.hpp"
#include "../world/cell.hpp"
#include "../world/record.hpp"

CSMForeign::LandscapeCollection::LandscapeCollection (const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells)
  : mCells (cells)
{
}

CSMForeign::LandscapeCollection::~LandscapeCollection ()
{
}

int CSMForeign::LandscapeCollection::load (ESM4::Reader& reader, bool base)
{
    std::string id;
    // HACK // FIXME
    if (reader.grp().type == ESM4::Grp_CellTemporaryChild)
    {
        std::ostringstream stream;
        stream << "#" << std::floor((float)reader.currCell().grid.x/2)
               << " " << std::floor((float)reader.currCell().grid.y/2);
        id = stream.str();
    }

    CSMForeign::Landscape record;
    //std::cout << "new Landscape " << std::hex << &record << std::endl; // FIXME

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
