#include "landscapecollection.hpp"

#include <iostream> // FIXME
#ifdef NDEBUG // FIXME for debugging only
#undef NDEBUG
#endif

#include <libs/platform/strings.h>

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

    std::string id;
    ESM4::formIdToString(reader.hdr().record.id, id);

    record.mName = id; // default is formId

    // check if parent cell left some bread crumbs
    if (reader.hasCellGrid())
    {
        // LAND records should only occur in a exterior cell.
        assert(reader.grp(3).type == ESM4::Grp_ExteriorCell &&
               reader.grp(2).type == ESM4::Grp_ExteriorSubCell &&
               reader.grp(1).type == ESM4::Grp_CellChild &&
               reader.grp(0).type == ESM4::Grp_CellTemporaryChild &&
               "LAND record found in an unexpected group heirarchy");

        std::uint32_t worldId = reader.currWorld();
        std::map<std::uint32_t, CoordinateIndex>::iterator lb = mPositionIndex.lower_bound(worldId);

        if (lb != mPositionIndex.end() && !(mPositionIndex.key_comp()(worldId, lb->first)))
        {
            std::pair<CoordinateIndex::iterator, bool> res = lb->second.insert(
                { std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), id });

            assert(res.second && "existing LAND record found for the given coordinates");
        }
        else
            mPositionIndex.insert(lb, std::map<std::uint32_t, CoordinateIndex>::value_type(worldId,
                { {std::pair<int, int>(reader.currCellGrid().grid.x, reader.currCellGrid().grid.y), id } }));

        std::ostringstream stream;
        // If using Morrowind cell size, need to divide by 2
        //stream << "#" << std::floor((float)reader.currCellGrid().grid.x/2)
               //<< " " << std::floor((float)reader.currCellGrid().grid.y/2);
        stream << "#" << reader.currCellGrid().grid.x << " " << reader.currCellGrid().grid.y;
        record.mName = stream.str(); // overwrite mName
#if 0
        std::string padding = "";
        padding.insert(0, reader.stackSize()*2, ' ');
        std::cout << padding << "LAND: formId " << std::hex << reader.hdr().record.id << std::endl;
        std::cout << padding << "LAND X " << std::dec << reader.currCellGrid().grid.x
            << ", Y " << reader.currCellGrid().grid.y << std::endl;
#endif
    }

    // FIXME; should be using the formId as the lookup key
    int index = CSMWorld::Collection<Landscape, CSMWorld::IdAccessor<Landscape> >::searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<CSMForeign::Landscape>().getId(record) = id;
    else
        record = this->getRecord(index).get();

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::LandscapeCollection::loadRecord (Landscape& record, ESM4::Reader& reader)
{
    record.load(reader, mCells);
}

int CSMForeign::LandscapeCollection::load (const Landscape& record, bool base, int index)
{
    if (index == -2)
        index = CSMWorld::Collection<Landscape, CSMWorld::IdAccessor<Landscape> >::searchId(
            CSMWorld::IdAccessor<Landscape>().getId(record));

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
        std::unique_ptr<CSMWorld::Record<Landscape> > record2(
                new CSMWorld::Record<Landscape>(CSMWorld::Collection<Landscape, CSMWorld::IdAccessor<Landscape> >::getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

int CSMForeign::LandscapeCollection::searchId(int x, int y, std::uint32_t worldId) const
{
    std::map<std::uint32_t, CoordinateIndex>::const_iterator iter = mPositionIndex.find(worldId);
    if (iter == mPositionIndex.end())
        return -1; // can't find world

    CoordinateIndex::const_iterator it = iter->second.find(std::make_pair(x, y));
    if (it == iter->second.end())
        return -1; // cann't find coordinate

    return CSMWorld::Collection<Landscape, CSMWorld::IdAccessor<Landscape> >::searchId(it->second);
}
