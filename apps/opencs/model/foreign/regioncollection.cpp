#include "regioncollection.hpp"

#include <libs/platform/strings.h>

#include <extern/esm4/reader.hpp>

#include "../world/record.hpp"

#include "worldcollection.hpp"

CSMForeign::RegionCollection::RegionCollection ()
{
}

CSMForeign::RegionCollection::~RegionCollection ()
{
}

int CSMForeign::RegionCollection::load (ESM4::Reader& reader, bool base)
{
    Region record;

    std::string id;
    //id = std::to_string(reader.hdr().record.id); // use formId converted to string instead
    char buf[8+1];
    int res = snprintf(buf, 8+1, "%08x", reader.hdr().record.id);
    if (res > 0 && res < 100)
        id.assign(buf);
    else
        throw std::runtime_error("Region Collection possible buffer overflow on formId");

    // FIXME; should be using the formId as the lookup key
    int index = this->searchId(id);

    if (index == -1)
        CSMWorld::IdAccessor<Region>().getId(record) = id;
    else
    {
        record = this->getRecord(index).get();
    }

    loadRecord(record, reader);

    return load(record, base, index);
}

void CSMForeign::RegionCollection::loadRecord (Region& record, ESM4::Reader& reader)
{
    record.load(reader);
}

int CSMForeign::RegionCollection::load (const Region& record, bool base, int index)
{
    if (index == -2)
        index = this->searchId(CSMWorld::IdAccessor<Region>().getId(record));

    if (index == -1)
    {
        // new record
        std::unique_ptr<CSMWorld::Record<Region> > record2(new CSMWorld::Record<Region>);

        record2->mState = base ? CSMWorld::RecordBase::State_BaseOnly : CSMWorld::RecordBase::State_ModifiedOnly;
        (base ? record2->mBase : record2->mModified) = record;

        index = this->getSize();
        this->appendRecord(std::move(record2));
    }
    else
    {
        // old record
        std::unique_ptr<CSMWorld::Record<Region> > record2(new CSMWorld::Record<Region>(getRecord(index)));

        if (base)
            record2->mBase = record;
        else
            record2->setModified(record);

        this->setRecord(index, std::move(record2));
    }

    return index;
}

void CSMForeign::RegionCollection::updateWorldNames (const WorldCollection& worlds)
{
    for (unsigned int i = 0; i < getRecords().size(); ++i)
    {
        std::unique_ptr<CSMWorld::Record<Region> > record(new CSMWorld::Record<Region>(getRecord(i)));
        Region& region = record->get();
        region.mWorld = worlds.getIdString(region.mWorldId);
        setRecord(i, std::move(record));
    }
}
