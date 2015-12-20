#include "navmeshcollection.hpp"

#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>

#include "../world/idcollection.hpp"
#include "../world/cell.hpp"
#include "../world/record.hpp"

CSMForeign::NavMeshCollection::NavMeshCollection (const CSMWorld::IdCollection<CSMWorld::Cell, CSMWorld::IdAccessor<CSMWorld::Cell> >& cells)
  : mCells (cells)
{
}

CSMForeign::NavMeshCollection::~NavMeshCollection ()
{
}

int CSMForeign::NavMeshCollection::load (ESM4::Reader& reader, bool base)
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

    CSMForeign::NavMesh record;
    //std::cout << "new NavMesh " << std::hex << &record << std::endl; // FIXME

    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::NavMesh>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::NavMeshCollection::loadRecord (CSMForeign::NavMesh& record, ESM4::Reader& reader)
{
    record.load(reader, mCells);
}

int CSMForeign::NavMeshCollection::load (const CSMForeign::NavMesh& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::NavMesh>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMesh> > record2(new CSMWorld::Record<CSMForeign::NavMesh>);
        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::NavMesh> > record2(
                new CSMWorld::Record<CSMForeign::NavMesh>(CSMWorld::Collection<CSMForeign::NavMesh,
                    CSMWorld::IdAccessor<CSMForeign::NavMesh> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

#if 0
void CSMForeign::NavMeshCollection::addNestedRow (int row, int col, int position)
{
}

void CSMForeign::NavMeshCollection::removeNestedRows (int row, int column, int subRow)
{
}

QVariant CSMForeign::NavMeshCollection::getNestedData (int row,
        int column, int subRow, int subColumn) const
{
}

void CSMForeign::NavMeshCollection::setNestedData (int row,
        int column, const QVariant& data, int subRow, int subColumn)
{
}

NestedTableWrapperBase* CSMForeign::NavMeshCollection::nestedTable (int row, int column) const
{
}

void CSMForeign::NavMeshCollection::setNestedTable (int row,
        int column, const NestedTableWrapperBase& nestedTable)
{
}

int CSMForeign::NavMeshCollection::getNestedRowsCount (int row, int column) const
{
}

int CSMForeign::NavMeshCollection::getNestedColumnsCount (int row, int column) const
{
}

NestableColumn *CSMForeign::NavMeshCollection::getNestableColumn (int column)
{
}
#endif
