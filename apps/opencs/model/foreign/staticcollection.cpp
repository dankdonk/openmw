#include "staticcollection.hpp"

#include "../world/record.hpp"

#include "worldcollection.hpp"

CSMForeign::StaticCollection::StaticCollection ()
{
}

CSMForeign::StaticCollection::~StaticCollection ()
{
}

void CSMForeign::StaticCollection::updateWorldNames (const WorldCollection& worlds)
{
    using CSMWorld::Record;

    for (unsigned int i = 0; i < getRecords().size(); ++i)
    {
        std::unique_ptr<Record<Static> > record(new Record<Static>(getRecord(i)));
        Static& stat = record->get();
        //stat.mWorld = worlds.getIdString(stat.mWorldId);
        setRecord(i, std::move(record));
    }
}
