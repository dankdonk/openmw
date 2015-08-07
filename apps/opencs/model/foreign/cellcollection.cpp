#include "cellcollection.hpp"

#include <iostream> // FIXME

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

CSMForeign::CellCollection::CellCollection ()
{
}

CSMForeign::CellCollection::~CellCollection ()
{
}

int CSMForeign::CellCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::Cell record;

    std::string id;
    // HACK // FIXME
    //if (reader.grp().type == ESM4::Grp_CellTemporaryChild)
    {
        std::ostringstream stream;
        stream << "#" << reader.currCell().grid.x << " " << reader.currCell().grid.y;
        //stream << "#" << std::floor((float)reader.currCell().grid.x/2)
               //<< " " << std::floor((float)reader.currCell().grid.y/2);
        id = stream.str();
        std::cout << "loading Cell " << id << std::endl; // FIXME
    }
    int index = this->searchId (id);

    if (index==-1)
        CSMWorld::IdAccessor<CSMForeign::Cell>().getId (record) = id;
    else
        record = this->getRecord (index).get();

    loadRecord (record, reader);

    return load (record, base, index);
}

void CSMForeign::CellCollection::loadRecord (CSMForeign::Cell& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::CellCollection::load (const CSMForeign::Cell& record, bool base, int index)
{
    if (index==-2)
        index = this->searchId (CSMWorld::IdAccessor<CSMForeign::Cell>().getId (record));

    //if (index==-1)
    {
        // new record
        CSMWorld::Record<CSMForeign::Cell> record2;
        record2.mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2.mBase : record2.mModified) = record;

        index = this->getSize();
        this->appendRecord (record2);
    }
#if 0
    else
    {
        // old record
        CSMWorld::Record<CSMForeign::Cell> record2
            = CSMWorld::Collection<CSMForeign::Cell, CSMWorld::IdAccessor<CSMForeign::Cell> >::getRecord (index);

        if (base)
            record2.mBase = record;
        else
            record2.setModified (record);

        this->setRecord (index, record2);
    }
#endif

    return index;
}

#if 0
void CSMForeign::CellCollection::addNestedRow (int row, int col, int position)
{
}

void CSMForeign::CellCollection::removeNestedRows (int row, int column, int subRow)
{
}

QVariant CSMForeign::CellCollection::getNestedData (int row,
        int column, int subRow, int subColumn) const
{
}

void CSMForeign::CellCollection::setNestedData (int row,
        int column, const QVariant& data, int subRow, int subColumn)
{
}

NestedTableWrapperBase* CSMForeign::CellCollection::nestedTable (int row, int column) const
{
}

void CSMForeign::CellCollection::setNestedTable (int row,
        int column, const NestedTableWrapperBase& nestedTable)
{
}

int CSMForeign::CellCollection::getNestedRowsCount (int row, int column) const
{
}

int CSMForeign::CellCollection::getNestedColumnsCount (int row, int column) const
{
}

NestableColumn *CSMForeign::CellCollection::getNestableColumn (int column)
{
}
#endif
