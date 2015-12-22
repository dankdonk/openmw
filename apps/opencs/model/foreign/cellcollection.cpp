#include "cellcollection.hpp"

#include <stdexcept>
#include <cassert>
#include <iostream> // FIXME

#ifdef NDEBUG // FIXME: debugging only
#undef NDEBUG
#endif

#include <extern/esm4/reader.hpp>

#include <libs/platform/strings.h>

#include "../world/record.hpp"

CSMForeign::CellCollection::CellCollection ()
{
}

CSMForeign::CellCollection::~CellCollection ()
{
}

// http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format/CELL
//
// The block and subblock groups for an interior cell are determined by the last two decimal
// digits of the lower 3 bytes of the cell form ID (the modindex is not included in the
// calculation). For example, for form ID 0x000CF2=3314, the block is 4 and the subblock is 1.
//
// The block and subblock groups for an exterior cell are determined by the X-Y coordinates of
// the cell. Each block contains 16 subblocks (4x4) and each subblock contains 64 cells (8x8).
// So each block contains 1024 cells (32x32).

int CSMForeign::CellCollection::load (ESM4::Reader& reader, bool base)
{
    CSMForeign::Cell record;

    loadRecord(record, reader);

    std::string id;

    // reader.currCell() is set during the load (sub record XCLC for an exterior cell)
    if (reader.hasCellGrid())
    {
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_WorldChild) && "Unexpected group while loading cell");

        std::string padding = "";
        padding.insert(0, reader.stackSize()*2, ' ');
        std::cout << padding << "CELL: formId " << std::dec << reader.hdr().record.id << std::endl; // FIXME
#if 0
        std::cout << padding << "CELL X " << std::dec << reader.currCell().grid.x << ", Y " << reader.currCell().grid.y << std::endl;
        std::cout << padding << "CELL type " << std::hex << reader.grp().type << std::endl;

        std::ostringstream stream;
        stream << "#" << reader.currCell().grid.x << " " << reader.currCell().grid.y;
        //stream << "#" << std::floor((float)reader.currCell().grid.x/2)
               //<< " " << std::floor((float)reader.currCell().grid.y/2);
        id = stream.str();
#endif
        char buf[100];
        int res = snprintf(buf, 100, "#%d %d", reader.currCell().grid.x, reader.currCell().grid.y);
        if (res > 0 && res < 100)
            id.assign(buf);
        else
            throw std::runtime_error("Cell Collection possible buffer overflow");
    }
    else
    {
        assert((reader.grp().type == ESM4::Grp_ExteriorSubCell ||
                reader.grp().type == ESM4::Grp_InteriorSubCell) && "Unexpected group while loading cell");

        std::string padding = "";
        padding.insert(0, reader.stackSize()*2, ' ');
        std::cout << padding << "CELL: formId " << std::dec << reader.hdr().record.id << std::endl; // FIXME
#if 0
        std::cout << padding << "CELL type " << std::hex << reader.grp().type << std::endl;
#endif
        if (!record.mName.empty())
        {
            id = record.mName;
            //if (id == "Arena" || id == "Arena ")
                //std::cout << "formId " << std::to_string(reader.hdr().record.id) << std::endl;
        }
        else
            id = std::to_string(reader.hdr().record.id); // use formId instead
    }

    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::Cell>().getId(record) = id;
    else
        record = this->getRecord(index).get(); // FIXME: record (just loaded) is being overwritten

    return load(record, base, index);
}

void CSMForeign::CellCollection::loadRecord (CSMForeign::Cell& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::CellCollection::load (const CSMForeign::Cell& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<CSMForeign::Cell>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<CSMForeign::Cell> > record2(new CSMWorld::Record<CSMForeign::Cell>);
        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<CSMForeign::Cell> > record2(
                new CSMWorld::Record<CSMForeign::Cell>(
                    CSMWorld::Collection<CSMForeign::Cell, CSMWorld::IdAccessor<CSMForeign::Cell> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

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
