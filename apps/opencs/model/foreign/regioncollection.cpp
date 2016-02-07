#include "regioncollection.hpp"

#include "../world/record.hpp"

#include "worldcollection.hpp"

CSMForeign::RegionCollection::RegionCollection ()
{
}

CSMForeign::RegionCollection::~RegionCollection ()
{
}

void CSMForeign::RegionCollection::updateWorldNames (const WorldCollection& worlds)
{
    using CSMWorld::Record;

    for (unsigned int i = 0; i < getRecords().size(); ++i)
    {
        std::unique_ptr<Record<Region> > record(new Record<Region>(getRecord(i)));
        Region& region = record->get();
        region.mWorld = worlds.getIdString(region.mWorldId);
        setRecord(i, std::move(record));
    }
}
